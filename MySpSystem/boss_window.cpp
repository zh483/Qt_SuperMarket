#include "boss_window.h"
#include "database_manager.h"
#include "session_manager.h"
#include "login_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDate>
#include <QScrollArea>
#include <QSplitter>
#include <QMouseEvent>

#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

// ────────────────────────────────────────────
BossWindow::BossWindow(QWidget* parent) : QWidget(parent)
{
    setupUI();
    auto& s = SessionManager::instance();
    setWindowTitle(QString("超市管理系统 - 老板 - %1").arg(s.empName()));
    resize(1100, 750);
}

// ────────────────────────────────────────────
// 整体布局
// ────────────────────────────────────────────
void BossWindow::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 顶部栏
    auto* header = new QHBoxLayout;
    header->setContentsMargins(12, 8, 12, 8);
    m_headerLabel = new QLabel;
    auto& s = SessionManager::instance();
    m_headerLabel->setText(QString("👤 %1（%2）").arg(s.empName(), s.roleName()));
    m_headerLabel->setStyleSheet("font-size: 14px;");
    header->addWidget(m_headerLabel);
    header->addStretch();
    auto* logoutBtn = new QPushButton("退出登录");
    logoutBtn->setStyleSheet(
        "QPushButton { color: #ff4d4f; border: 1px solid #ff4d4f; padding: 4px 12px; border-radius: 3px; }"
        "QPushButton:hover { background: #fff1f0; }");
    header->addWidget(logoutBtn);
    root->addLayout(header);

    // 下方：侧边栏 + 内容
    auto* body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);

    m_navList = new QListWidget;
    m_navList->setFixedWidth(150);
    m_navList->setStyleSheet(
        "QListWidget { border: none; border-right: 1px solid #e8e8e8; background: #fafafa; font-size: 13px; }"
        "QListWidget::item { padding: 10px 12px; }"
        "QListWidget::item:selected { background: #1890ff; color: white; }");
    // 分组
    m_navList->addItem("📊 销售额总览");
    m_navList->addItem("📈 商品销售排行");
    QListWidgetItem* skip1=new QListWidgetItem("─── 员工管理 ───");
    skip1->setFlags(skip1->flags() & ~Qt::ItemIsSelectable);
    m_navList->addItem(skip1);
    m_navList->addItem("  录入员工");
    m_navList->addItem("  辞退员工");
    m_navList->addItem("  员工工时排序");
    QListWidgetItem* skip2 = new QListWidgetItem("─── 出勤管理 ───");
    skip2->setFlags(skip2->flags() & ~Qt::ItemIsSelectable);
    m_navList->addItem(skip2);
    m_navList->addItem("  排班");
    m_navList->addItem("  值班状况");
    m_navList->addItem("  出勤统计");
    QListWidgetItem* skip3 = new QListWidgetItem("─── 商品管理 ───");
    skip3->setFlags(skip3->flags() & ~Qt::ItemIsSelectable);
    m_navList->addItem(skip3);
    m_navList->addItem("  录入商品");
    m_navList->addItem("  下架商品");
    m_navList->addItem("  查询商品");
    QListWidgetItem* skip4 = new QListWidgetItem("─── 库存管理 ───");
    skip4->setFlags(skip4->flags() & ~Qt::ItemIsSelectable);
    m_navList->addItem(skip4);
    m_navList->addItem("  季度盘点");
    m_navList->addItem("  每日归零");

    // 事件过滤器：阻止点击分隔符行
    m_navList->viewport()->installEventFilter(this);

    body->addWidget(m_navList);
    m_stack = new QStackedWidget;
    m_stack->addWidget(createDashboardPage());       // 0 - 销售额总览
    m_stack->addWidget(createRankingPage());          // 1 - 商品销售排行
    m_stack->addWidget(new QWidget);                  // 2 - 分隔符 skip
    m_stack->addWidget(createAddEmpPage());           // 3 - 录入员工
    m_stack->addWidget(createFireEmpPage());          // 4 - 辞退员工
    m_stack->addWidget(createEmpRankPage());          // 5 - 员工工时排序
    m_stack->addWidget(new QWidget);                  // 6 - skip
    m_stack->addWidget(createSchedulePage());         // 7 - 排班
    m_stack->addWidget(createDutyPage());             // 8 - 值班状况
    m_stack->addWidget(createAttendancePage());       // 9 - 出勤统计
    m_stack->addWidget((new QWidget));                  // 10 - skip
    m_stack->addWidget(createAddProductPage());       // 11 - 录入商品
    m_stack->addWidget(createDeactivateProductPage());// 12 - 下架商品
    m_stack->addWidget(createQueryProductPage());     // 13 - 查询商品
    m_stack->addWidget(new QWidget);                  // 14 - skip
    m_stack->addWidget(createInventoryCheckPage());   // 15 - 季度盘点
    m_stack->addWidget(createDailyResetPage());       // 16 - 每日归零

    body->addWidget(m_stack, 1);
    root->addLayout(body, 1);

    connect(m_navList, &QListWidget::currentRowChanged, this, &BossWindow::onNavChanged);
    connect(logoutBtn, &QPushButton::clicked, this, &BossWindow::onLogout);
}

bool BossWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_navList->viewport() && (event->type() == QEvent::MouseButtonPress|| event->type()==QEvent::MouseButtonDblClick ||event->type() == QEvent::MouseMove)) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        QListWidgetItem* item = m_navList->itemAt(mouseEvent->pos());
        if (item && item->text().startsWith("───")) {
            return true;  // 吃掉事件，分隔符行不可点击
        }
    }
    return QWidget::eventFilter(obj, event);
}

void BossWindow::refreshScheduleEmployeeLists()
{
    if (!m_scheduleTable)return;
    auto& db= DatabaseManager::instance();
    QSqlQuery eq = db.exec("SELECT id, name FROM employee WHERE is_active = 1 AND role != 0 ORDER BY id");
    QStringList empNames;
    QVariantList empIDs;
    empNames << "--休息--"; empIDs << 0;
    while (eq.next())
    {
        empNames << eq.value(1).toString();
        empIDs << eq.value(0).toInt();
    }
    for (int r = 0; r < m_scheduleTable->rowCount(); r++) {
        for (int c = 0; c < m_scheduleTable->columnCount(); c++)
        {
            auto* combo = qobject_cast<QComboBox*>(m_scheduleTable->cellWidget(r, c));
            int curentId = combo->currentData().toInt();
            combo->blockSignals(true);
            combo->clear();
            combo->addItems(empNames);
            for (int i = 0; i < empIDs.size(); i++)
                combo->setItemData(i, empIDs[i]);
            int index = empIDs.indexOf(curentId);
            if (index > 0) {
                combo->setCurrentIndex(index);
            }
            else
            {
                combo->setCurrentIndex(0);
            }
            combo->blockSignals(false);
        }
    }
}

void BossWindow::onNavChanged(int row)
{
    m_stack->setCurrentIndex(row);
    // 根据页面刷新数据
    switch (row) {
    case 0: refreshDashboard(); break;
    case 1: refreshProductRanking(); break;
    case 4: refreshEmployeeList(m_fireEmpTable); break;
    case 5: refreshEmployeeRanking(); break;
    case 7: refreshSchedule(); break;
    case 8: refreshDutyStatus(); break;
    case 9: refreshAttendanceStats(); break;
    case 12: refreshProductList(m_deactProdTable); break;
    case 15: refreshInventoryCheck(); break;
    case 16: {
        auto& db = DatabaseManager::instance();
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        QVariant sales = db.execScalar(
            "SELECT total_sales FROM daily_counter WHERE date = ?", { today });
        m_resetInfo->setText(QString("今日销售额: ¥%1\n订单数: %2")
            .arg(sales.toDouble(), 0, 'f', 2)
            .arg(db.execScalar("SELECT order_count FROM daily_counter WHERE date = ?", { today }).toInt()));
        break;
    }
    }
}

void BossWindow::onLogout()
{
    SessionManager::instance().logout();
    auto* login = new LoginWindow;
    login->show();
    this->close();
}

// ────────────────────────────────────────────
//  销售额总览（含柱状图）
// ────────────────────────────────────────────
QWidget* BossWindow::createDashboardPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("销售额总览");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    // 统计卡片行
    auto* cards = new QHBoxLayout;
    auto makeCard = [](const QString& label, QLabel*& val) -> QWidget* {
        auto* w = new QWidget;
        w->setStyleSheet("background: #f0f5ff; border-radius: 8px; padding: 12px;");
        auto* l = new QVBoxLayout(w);
        auto* lb = new QLabel(label);
        lb->setStyleSheet("color: #666; font-size: 12px;");
        l->addWidget(lb);
        val = new QLabel("¥0.00");
        val->setStyleSheet("font-size: 22px; font-weight: bold; color: #1890ff;");
        l->addWidget(val);
        return w;
    };
    cards->addWidget(makeCard("今日销售额", m_todaySalesLabel));
    cards->addWidget(makeCard("今日订单数", m_todayOrdersLabel));
    cards->addWidget(makeCard("本月销售额", m_monthSalesLabel));
    cards->addWidget(makeCard("本月利润", m_monthProfitLabel));
    root->addLayout(cards);

    // 柱状图
    m_chartView = new QChartView;
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(350);
    root->addWidget(m_chartView, 1);

    auto* refreshBtn = new QPushButton("刷新数据");
    refreshBtn->setStyleSheet(
        "QPushButton { background: #1890ff; color: white; padding: 6px 16px; border-radius: 3px; }");
    root->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &BossWindow::refreshDashboard);

    return page;
}

void BossWindow::refreshDashboard()
{
    auto& db = DatabaseManager::instance();
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString month = QDate::currentDate().toString("yyyy-MM");

    // 今日统计
    QSqlQuery tq = db.exec("SELECT total_sales, order_count FROM daily_counter WHERE date = ?", { today });
    double todaySales = 0; int todayOrders = 0;
    if (tq.next()) { todaySales = tq.value(0).toDouble(); todayOrders = tq.value(1).toInt(); }
    m_todaySalesLabel->setText(QString("¥%1").arg(todaySales, 0, 'f', 2));
    m_todayOrdersLabel->setText(QString::number(todayOrders));

    // 本月统计
    QSqlQuery mq = db.exec(
        "SELECT COALESCE(SUM(total_sales), 0), COALESCE(SUM(order_count), 0) "
        "FROM daily_counter WHERE date LIKE ?", { month + "%" });
    double monthSales = 0; int monthOrders = 0;
    if (mq.next()) { monthSales = mq.value(0).toDouble(); monthOrders = mq.value(1).toInt(); }
    m_monthSalesLabel->setText(QString("¥%1").arg(monthSales, 0, 'f', 2));

    // 本月利润
    QSqlQuery profitQ = db.exec(
        "SELECT COALESCE(SUM((oi.unit_price - b.cost_price) * oi.qty), 0) "
        "FROM order_item oi "
        "JOIN batch b ON oi.batch_id = b.id "
        "JOIN \"order\" o ON oi.order_id = o.id "
        "WHERE o.create_time LIKE ?", { month + "%" });
    double monthProfit = 0;
    if (profitQ.next()) monthProfit = profitQ.value(0).toDouble();
    m_monthProfitLabel->setText(QString("¥%1").arg(monthProfit, 0, 'f', 2));

    // ── 柱状图：近6个月销售额 & 利润 ──
    auto* chart = new QChart;
    chart->setTitle("近6个月销售趋势");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* salesSet = new QBarSet("销售额");
    auto* profitSet = new QBarSet("利润");
    QStringList categories;

    for (int i = 5; i >= 0; i--) {
        QDate d = QDate::currentDate().addMonths(-i);
        QString m = d.toString("yyyy-MM");
        categories << d.toString("M月");

        double s = db.execScalar(
            "SELECT COALESCE(SUM(total_sales), 0) FROM daily_counter WHERE date LIKE ?",
            { m + "%" }).toDouble();
        double p = db.execScalar(
            "SELECT COALESCE(SUM((oi.unit_price - b.cost_price) * oi.qty), 0) "
            "FROM order_item oi JOIN batch b ON oi.batch_id = b.id "
            "JOIN \"order\" o ON oi.order_id = o.id "
            "WHERE o.create_time LIKE ?", { m + "%" }).toDouble();
        *salesSet << s;
        *profitSet << p;
    }

    auto* series = new QBarSeries;
    series->append(salesSet);
    series->append(profitSet);
    chart->addSeries(series);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setTitleText("金额 (元)");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 替换旧图表
    auto* oldChart = m_chartView->chart();
    m_chartView->setChart(chart);
    delete oldChart;
}

// ────────────────────────────────────────────
//  商品销售排行
// ────────────────────────────────────────────
QWidget* BossWindow::createRankingPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("商品销售排行");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("从:"));
    m_rankDateFrom = new QDateEdit(QDate::currentDate().addMonths(-1));
    m_rankDateFrom->setCalendarPopup(true);
    filterRow->addWidget(m_rankDateFrom);
    filterRow->addWidget(new QLabel("至:"));
    m_rankDateTo = new QDateEdit(QDate::currentDate());
    m_rankDateTo->setCalendarPopup(true);
    filterRow->addWidget(m_rankDateTo);
    auto* queryBtn = new QPushButton("查询");
    queryBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 4px 12px; border-radius: 3px; }");
    filterRow->addWidget(queryBtn);
    filterRow->addStretch();
    root->addLayout(filterRow);

    m_rankingTable = new QTableWidget(0, 5);
    m_rankingTable->setHorizontalHeaderLabels({ "排名", "商品", "销量", "销售额", "利润" });
    m_rankingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_rankingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rankingTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_rankingTable, 1);

    connect(queryBtn, &QPushButton::clicked, this, &BossWindow::refreshProductRanking);
    return page;
}

void BossWindow::refreshProductRanking()
{
    auto& db = DatabaseManager::instance();
    QString from = m_rankDateFrom->date().toString("yyyy-MM-dd");
    QString to = m_rankDateTo->date().toString("yyyy-MM-dd");

    QSqlQuery q = db.exec(R"(
        SELECT p.name,
               COALESCE(SUM(oi.qty), 0) AS total_qty,
               COALESCE(SUM(oi.unit_price * oi.qty), 0) AS total_sales,
               COALESCE(SUM((oi.unit_price - b.cost_price) * oi.qty), 0) AS total_profit
        FROM order_item oi
        JOIN product p ON oi.product_id = p.id
        JOIN batch b ON oi.batch_id = b.id
        JOIN "order" o ON oi.order_id = o.id
        WHERE o.create_time BETWEEN ? AND ?
        GROUP BY p.id
        ORDER BY total_sales DESC
    )", { from + " 00:00:00", to + " 23:59:59" });

    m_rankingTable->setRowCount(0);
    int rank = 0;
    while (q.next()) {
        m_rankingTable->insertRow(rank);
        m_rankingTable->setItem(rank, 0, new QTableWidgetItem(QString::number(rank + 1)));
        m_rankingTable->setItem(rank, 1, new QTableWidgetItem(q.value(0).toString()));
        m_rankingTable->setItem(rank, 2, new QTableWidgetItem(q.value(1).toString()));
        m_rankingTable->setItem(rank, 3, new QTableWidgetItem(
            QString("¥%1").arg(q.value(2).toDouble(), 0, 'f', 2)));
        m_rankingTable->setItem(rank, 4, new QTableWidgetItem(
            QString("¥%1").arg(q.value(3).toDouble(), 0, 'f', 2)));
        rank++;
    }
}

// ────────────────────────────────────────────
//  录入员工
// ────────────────────────────────────────────
QWidget* BossWindow::createAddEmpPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("录入员工");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* form = new QFormLayout;
    m_empName = new QLineEdit; m_empName->setPlaceholderText("员工姓名");
    form->addRow("姓名:", m_empName);
    m_empPhone = new QLineEdit; m_empPhone->setPlaceholderText("手机号（登录用）");
    form->addRow("手机号:", m_empPhone);
    m_empRole = new QComboBox;
    m_empRole->addItems({ "员工" });
    m_empRole->setCurrentIndex(0);
    form->addRow("角色:", m_empRole);
    m_empPassword = new QLineEdit; m_empPassword->setPlaceholderText("默认 123456");
    m_empPassword->setText("123456");
    form->addRow("密码:", m_empPassword);
    root->addLayout(form);

    auto* addBtn = new QPushButton("确认录入");
    addBtn->setStyleSheet("QPushButton { background: #52c41a; color: white; padding: 8px 16px; border-radius: 4px; }");
    root->addWidget(addBtn);

    m_addEmpStatus = new QLabel;
    root->addWidget(m_addEmpStatus);
    root->addStretch();

    connect(addBtn, &QPushButton::clicked, this, &BossWindow::onAddEmployee);
    return page;
}

void BossWindow::onAddEmployee()
{
    QString name = m_empName->text().trimmed();
    QString phone = m_empPhone->text().trimmed();
    QString password = m_empPassword->text();
    int role = m_empRole->currentIndex(); //

    if (name.isEmpty() || phone.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写姓名和手机号");
        return;
    }

    // 角色映射: combobox顺序: 0=员工
    int dbRole = (role == 0) ? 1 : 0;

    auto& db = DatabaseManager::instance();
    QSqlQuery exist = db.exec("SELECT id FROM employee WHERE phone = ?", { phone });
    if (exist.next()) {
        QMessageBox::warning(this, "手机号重复", "该手机号已被注册！");
        return;
    }

    db.exec("INSERT INTO employee (name, phone, role, password) VALUES (?,?,?,?)",
            { name, phone, dbRole, password.isEmpty() ? "123456" : password });

    m_addEmpStatus->setStyleSheet("color: green;");
    m_addEmpStatus->setText(QString("员工 %1 录入成功！").arg(name));
    m_empName->clear(); m_empPhone->clear();
    refreshScheduleEmployeeLists();
}

// ────────────────────────────────────────────
//  辞退员工
// ────────────────────────────────────────────
QWidget* BossWindow::createFireEmpPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("辞退员工");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    m_fireEmpTable = new QTableWidget(0, 5);
    m_fireEmpTable->setHorizontalHeaderLabels({ "ID", "姓名", "手机号", "角色", "状态" });
    m_fireEmpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fireEmpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fireEmpTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_fireEmpTable, 1);

    auto* fireBtn = new QPushButton("辞退选中员工（软删除）");
    fireBtn->setStyleSheet("QPushButton { background: #ff4d4f; color: white; padding: 8px 16px; border-radius: 4px; }");
    root->addWidget(fireBtn);

    connect(fireBtn, &QPushButton::clicked, this, &BossWindow::onFireEmployee);
    
    return page;
}

void BossWindow::refreshEmployeeList(QTableWidget* table)
{
    auto& db = DatabaseManager::instance();
    QSqlQuery q = db.exec(
        "SELECT id, name, phone, role, is_active FROM employee ORDER BY id");
    table->setRowCount(0);
    int row = 0;
    while (q.next()) {
        int role = q.value(3).toInt();
        if(role==0)continue;
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        table->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        table->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        //int role = q.value(3).toInt();
        QString roleName = (role == 0) ? "老板" : "员工";
        table->setItem(row, 3, new QTableWidgetItem(roleName));
        bool active = q.value(4).toBool();
        auto* statusItem = new QTableWidgetItem(active ? "在职" : "已离职");
        if (!active) statusItem->setForeground(Qt::red);
        table->setItem(row, 4, statusItem);
        row++;
    }
}

void BossWindow::onFireEmployee()
{
    int selRow = m_fireEmpTable->currentRow();
    if (selRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要辞退的员工");
        return;
    }
    int empId = m_fireEmpTable->item(selRow, 0)->text().toInt();
    QString name = m_fireEmpTable->item(selRow, 1)->text();
    bool active = m_fireEmpTable->item(selRow, 4)->text() == "在职";
    if (!active) {
        QMessageBox::warning(this, "提示", "该员工已离职");
        return;
    }

    auto ret = QMessageBox::question(this, "确认辞退",
        QString("确认辞退 %1 吗？\n（历史数据将保留）").arg(name));
    if (ret != QMessageBox::Yes) return;

    DatabaseManager::instance().exec(
        "UPDATE employee SET is_active = 0 WHERE id = ?", { empId });
    refreshEmployeeList(m_fireEmpTable);
    refreshScheduleEmployeeLists();
}

// ────────────────────────────────────────────
//  员工工时排序
// ────────────────────────────────────────────
QWidget* BossWindow::createEmpRankPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("员工工时排序（按总工作时长）");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    m_empRankTable = new QTableWidget(0, 5);
    m_empRankTable->setHorizontalHeaderLabels({ "排名", "姓名", "角色", "总打卡次数", "最近打卡" });
    m_empRankTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_empRankTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_empRankTable, 1);

    auto* refreshBtn = new QPushButton("刷新");
    refreshBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 6px 16px; border-radius: 3px; }");
    root->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &BossWindow::refreshEmployeeRanking);

    return page;
}

void BossWindow::refreshEmployeeRanking()
{
    auto& db = DatabaseManager::instance();
    QSqlQuery q = db.exec(R"(
        SELECT e.name,
               CASE e.role WHEN 0 THEN '老板' WHEN 1 THEN '员工' ELSE '为啥改不了' END,
               COUNT(sl.id) AS shift_count,
               MAX(sl.shift_time) AS last_shift
        FROM employee e
        LEFT JOIN shift_log sl ON sl.to_emp_id = e.id
        WHERE e.is_active = 1 AND e.role != 0
        GROUP BY e.id
        ORDER BY shift_count DESC
    )");

    m_empRankTable->setRowCount(0);
    int rank = 0;

    while (q.next()) {
        m_empRankTable->insertRow(rank);
        m_empRankTable->setItem(rank, 0, new QTableWidgetItem(QString::number(rank + 1)));
        m_empRankTable->setItem(rank, 1, new QTableWidgetItem(q.value(0).toString()));
        m_empRankTable->setItem(rank, 2, new QTableWidgetItem(q.value(1).toString()));
        m_empRankTable->setItem(rank, 3, new QTableWidgetItem(q.value(2).toString()));
        m_empRankTable->setItem(rank, 4, new QTableWidgetItem(q.value(3).toString()));
        rank++;
    }
}

// ────────────────────────────────────────────
//  排班（4行×7列表格）
// ────────────────────────────────────────────
QWidget* BossWindow::createSchedulePage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("每周排班");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* row = new QHBoxLayout;
    row->addWidget(new QLabel("选择周一日期:"));
    m_weekStart = new QDateEdit(QDate::currentDate());
    m_weekStart->setCalendarPopup(true);
    // 自动对齐到本周一
    int dow = QDate::currentDate().dayOfWeek(); // 1=Monday
    m_weekStart->setDate(QDate::currentDate().addDays(-(dow - 1)));
    row->addWidget(m_weekStart);
    row->addStretch();
    root->addLayout(row);

    // 4 行 × 7 列的排班表
    m_scheduleTable = new QTableWidget(4, 7);
    QStringList rowHeaders = { "早上(销售)", "下午(销售)", "晚上(销售)", "库存" };
    m_scheduleTable->setVerticalHeaderLabels(rowHeaders);
    QStringList dayHeaders = { "周一", "周二", "周三", "周四", "周五", "周六", "周日" };
    m_scheduleTable->setHorizontalHeaderLabels(dayHeaders);

    // 每个格子放一个 ComboBox
    auto& db = DatabaseManager::instance();
    QSqlQuery eq = db.exec("SELECT id, name FROM employee WHERE is_active = 1 AND role != 0 ORDER BY id");
    QStringList empNames;
    QVariantList empIds;
    empNames << "--休息--"; empIds << 0;
    while (eq.next()) {
        empNames << eq.value(1).toString();
        empIds << eq.value(0).toInt();
    }

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 7; c++) {
            auto* combo = new QComboBox;
            combo->addItems(empNames);
            // 把员工 ID 存到 UserRole
            for (int i = 0; i < empIds.size(); i++)
                combo->setItemData(i, empIds[i]);
            m_scheduleTable->setCellWidget(r, c, combo);
        }
    }

    m_scheduleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_scheduleTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    root->addWidget(m_scheduleTable, 1);

    auto* saveBtn = new QPushButton("保存本周排班");
    saveBtn->setStyleSheet("QPushButton { background: #52c41a; color: white; padding: 8px 16px; border-radius: 4px; font-size: 14px; }");
    root->addWidget(saveBtn);

    m_scheduleStatus = new QLabel;
    root->addWidget(m_scheduleStatus);

    connect(saveBtn, &QPushButton::clicked, this, &BossWindow::onSaveSchedule);
    connect(m_weekStart, &QDateEdit::dateChanged, this, &BossWindow::refreshSchedule);
    return page;
}

void BossWindow::refreshSchedule()
{
    // 从数据库加载当前周的排班，填充到 ComboBox
    auto& db = DatabaseManager::instance();
    QDate monday = m_weekStart->date();
    QString weekStart = monday.toString("yyyy-MM-dd");

    // 先清空
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 7; c++) {
            auto* combo = qobject_cast<QComboBox*>(m_scheduleTable->cellWidget(r, c));
            if (combo) combo->setCurrentIndex(0);
        }

    QStringList shiftTypes = { ShiftType::Morning, ShiftType::Afternoon,
                               ShiftType::Night, ShiftType::Inventory };
    QSqlQuery q = db.exec(
        "SELECT date, shift_type, emp_id FROM schedule WHERE week_start = ?",
        { weekStart });

    while (q.next()) {
        QString date = q.value(0).toString();
        QString shiftType = q.value(1).toString();
        int empId = q.value(2).toInt();

        QDate d = QDate::fromString(date, "yyyy-MM-dd");
        int col = monday.daysTo(d);
        if (col < 0 || col > 6) continue;

        int row = shiftTypes.indexOf(shiftType);
        if (row < 0) continue;

        auto* combo = qobject_cast<QComboBox*>(m_scheduleTable->cellWidget(row, col));
        if (combo) {
            for (int i = 0; i < combo->count(); i++) {
                if (combo->itemData(i).toInt() == empId) {
                    combo->setCurrentIndex(i);
                    break;
                }
            }
        }
    }
}

void BossWindow::onSaveSchedule()
{
    auto& db = DatabaseManager::instance();
    QDate monday = m_weekStart->date();
    QString weekStart = monday.toString("yyyy-MM-dd");

    QStringList shiftTypes = { ShiftType::Morning, ShiftType::Afternoon,
                               ShiftType::Night, ShiftType::Inventory };

    db.db().transaction();

    // 删除本周旧排班
    db.exec("DELETE FROM schedule WHERE week_start = ?", { weekStart });

    // 插入新排班
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 7; c++) {
            auto* combo = qobject_cast<QComboBox*>(m_scheduleTable->cellWidget(r, c));
            if (!combo) continue;
            int empId = combo->currentData().toInt();
            if (empId == 0) continue;  // 休息

            QDate date = monday.addDays(c);
            db.exec("INSERT INTO schedule (emp_id, date, shift_type, week_start) VALUES (?,?,?,?)",
                    { empId, date.toString("yyyy-MM-dd"), shiftTypes[r], weekStart });
        }
    }

    db.db().commit();
    m_scheduleStatus->setStyleSheet("color: green;");
    m_scheduleStatus->setText(QString("排班保存成功！（%1 ~ %2）")
        .arg(monday.toString("MM-dd"), monday.addDays(6).toString("MM-dd")));
}

// ────────────────────────────────────────────
//  值班状况
// ────────────────────────────────────────────
QWidget* BossWindow::createDutyPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("值班状况");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* row = new QHBoxLayout;
    row->addWidget(new QLabel("日期:"));
    m_dutyDate = new QDateEdit(QDate::currentDate());
    m_dutyDate->setCalendarPopup(true);
    row->addWidget(m_dutyDate);
    auto* queryBtn = new QPushButton("查询");
    queryBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 4px 12px; border-radius: 3px; }");
    row->addWidget(queryBtn);
    row->addStretch();
    root->addLayout(row);

    m_dutyTable = new QTableWidget(0, 5);
    m_dutyTable->setHorizontalHeaderLabels({ "员工", "角色", "排班时段", "打卡时间", "状态" });
    m_dutyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dutyTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_dutyTable, 1);

    connect(queryBtn, &QPushButton::clicked, this, &BossWindow::refreshDutyStatus);
    return page;
}

void BossWindow::refreshDutyStatus()
{
    auto& db = DatabaseManager::instance();
    QString date = m_dutyDate->date().toString("yyyy-MM-dd");

    QSqlQuery q = db.exec(R"(
        SELECT e.name,
               CASE e.role WHEN 0 THEN '老板' WHEN 1 THEN '收银员' END,
               s.shift_type,
               sl.shift_time
        FROM schedule s
        JOIN employee e ON s.emp_id = e.id
        LEFT JOIN shift_log sl ON sl.to_emp_id = e.id AND date(sl.shift_time) = ?
             AND sl.shift_type = s.shift_type
        WHERE s.date = ?
        UNION ALL
        -- 加班（无排班但有打卡）
        SELECT e.name,
               CASE e.role WHEN 0 THEN '老板' WHEN 1 THEN '收银员(销售)' ELSE '收银员(库存)' END,
               '加班',
               sl.shift_time
        FROM shift_log sl
        JOIN employee e ON sl.to_emp_id = e.id
        WHERE date(sl.shift_time) = ?
          AND e.id NOT IN (SELECT emp_id FROM schedule WHERE date = ?)
    )", { date, date, date, date });

    m_dutyTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_dutyTable->insertRow(row);
        m_dutyTable->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        m_dutyTable->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        m_dutyTable->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        m_dutyTable->setItem(row, 3, new QTableWidgetItem(q.value(3).toString()));

        QString status;
        QString shiftType = q.value(2).toString();
        bool punched = !q.value(3).isNull();
        if (shiftType == "加班") {
            status = punched ? "✅ 已打卡(加班)" : "⚠️ 加班";
        } else if (punched) {
            status = "✅ 已打卡";
        } else {
            status = "❌ 缺勤";
        }
        m_dutyTable->setItem(row, 4, new QTableWidgetItem(status));
        row++;
    }

    // 检查排了班但没打卡的（上面 LEFT JOIN 已包含）
    if (row == 0) {
        QMessageBox::information(this, "提示", "当天无值班记录");
    }
}

// ────────────────────────────────────────────
//  出勤统计
// ────────────────────────────────────────────
QWidget* BossWindow::createAttendancePage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("出勤统计（奖惩）");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("从:"));
    m_attDateFrom = new QDateEdit(QDate::currentDate().addDays(-30));
    m_attDateFrom->setCalendarPopup(true);
    filterRow->addWidget(m_attDateFrom);
    filterRow->addWidget(new QLabel("至:"));
    m_attDateTo = new QDateEdit(QDate::currentDate());
    m_attDateTo->setCalendarPopup(true);
    filterRow->addWidget(m_attDateTo);
    auto* queryBtn = new QPushButton("查询");
    queryBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 4px 12px; border-radius: 3px; }");
    filterRow->addWidget(queryBtn);
    filterRow->addStretch();
    root->addLayout(filterRow);

    m_attTable = new QTableWidget(0, 6);
    m_attTable->setHorizontalHeaderLabels({ "员工", "排班天数", "打卡天数", "缺勤天数", "全勤奖励", "惩罚" });
    m_attTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_attTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_attTable, 1);

    connect(queryBtn, &QPushButton::clicked, this, &BossWindow::refreshAttendanceStats);
    return page;
}

void BossWindow::refreshAttendanceStats()
{
    auto& db = DatabaseManager::instance();
    QString from = m_attDateFrom->date().toString("yyyy-MM-dd");
    QString to = m_attDateTo->date().toString("yyyy-MM-dd");

    QSqlQuery q = db.exec(R"(
        SELECT e.name,
               COUNT(DISTINCT s.date) AS schedule_days,
               COUNT(DISTINCT date(sl.shift_time)) AS punch_days,
               CASE 
                    WHEN COUNT(DISTINCT s.date) - COUNT(DISTINCT DATE(sl.shift_time)) < 0 THEN 0
                    ELSE COUNT(DISTINCT s.date) - COUNT(DISTINCT DATE(sl.shift_time))
               END AS absent_days
        FROM employee e
        LEFT JOIN schedule s ON s.emp_id = e.id AND s.date BETWEEN ? AND ?
        LEFT JOIN shift_log sl ON sl.to_emp_id = e.id AND date(sl.shift_time) BETWEEN ? AND ?
        WHERE e.is_active = 1 AND e.role != 0
        GROUP BY e.id
        ORDER BY absent_days DESC
    )", { from, to, from, to });

    m_attTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_attTable->insertRow(row);
        m_attTable->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        int scheduleDays = q.value(1).toInt();
        int punchDays = q.value(2).toInt();
        int absentDays = q.value(3).toInt();
        m_attTable->setItem(row, 1, new QTableWidgetItem(QString::number(scheduleDays)));
        m_attTable->setItem(row, 2, new QTableWidgetItem(QString::number(punchDays)));
        auto* absentItem = new QTableWidgetItem(QString::number(absentDays));
        if (absentDays > 0) absentItem->setForeground(Qt::red);
        m_attTable->setItem(row, 3, absentItem);

        // 全勤奖励：0缺勤 → 奖金标记
        QString reward = (absentDays == 0 && scheduleDays > 0) ? "🏆 全勤" : "-";
        m_attTable->setItem(row, 4, new QTableWidgetItem(reward));

        // 惩罚：缺勤 > 0
        QString penalty = (absentDays > 0) ? QString("⚠️ 缺勤%1天").arg(absentDays) : "-";
        m_attTable->setItem(row, 5, new QTableWidgetItem(penalty));
        row++;
    }
}

// ────────────────────────────────────────────
//  录入商品
// ────────────────────────────────────────────
QWidget* BossWindow::createAddProductPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("录入商品");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* form = new QFormLayout;
    m_prodName = new QLineEdit; m_prodName->setPlaceholderText("商品名称");
    form->addRow("名称:", m_prodName);
    m_prodCategory = new QLineEdit; m_prodCategory->setPlaceholderText("如：零食、饮料");
    form->addRow("分类:", m_prodCategory);
    m_prodPrice = new QLineEdit; m_prodPrice->setPlaceholderText("零售价");
    form->addRow("售价:", m_prodPrice);
    root->addLayout(form);

    auto* addBtn = new QPushButton("确认录入");
    addBtn->setStyleSheet("QPushButton { background: #52c41a; color: white; padding: 8px 16px; border-radius: 4px; }");
    root->addWidget(addBtn);

    m_addProdStatus = new QLabel;
    root->addWidget(m_addProdStatus);
    root->addStretch();

    connect(addBtn, &QPushButton::clicked, this, &BossWindow::onAddProduct);
    return page;
}

void BossWindow::onAddProduct()
{
    QString name = m_prodName->text().trimmed();
    QString category = m_prodCategory->text().trimmed();
    double price = m_prodPrice->text().toDouble();

    if (name.isEmpty() || price <= 0) {
        QMessageBox::warning(this, "提示", "请填写商品名称和正确的售价");
        return;
    }

    DatabaseManager::instance().exec(
        "INSERT INTO product (name, category, price) VALUES (?,?,?)",
        { name, category, price });
    m_addProdStatus->setStyleSheet("color: green;");
    m_addProdStatus->setText(QString("商品 %1 录入成功！（售价 ¥%2）").arg(name).arg(price, 0, 'f', 2));
    m_prodName->clear(); m_prodCategory->clear(); m_prodPrice->clear();
}

// ────────────────────────────────────────────
//  下架商品
// ────────────────────────────────────────────
QWidget* BossWindow::createDeactivateProductPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("下架商品（软删除）");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    m_deactProdTable = new QTableWidget(0, 5);
    m_deactProdTable->setHorizontalHeaderLabels({ "ID", "名称", "分类", "售价", "状态" });
    m_deactProdTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deactProdTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deactProdTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_deactProdTable, 1);

    auto* deactBtn = new QPushButton("下架选中商品");
    deactBtn->setStyleSheet("QPushButton { background: #ff4d4f; color: white; padding: 8px 16px; border-radius: 4px; }");
    root->addWidget(deactBtn);

    connect(deactBtn, &QPushButton::clicked, this, &BossWindow::onDeactivateProduct);
    return page;
}

void BossWindow::refreshProductList(QTableWidget* table)
{
    auto& db = DatabaseManager::instance();
    QSqlQuery q = db.exec("SELECT id, name, category, price, is_active FROM product ORDER BY id");
    table->setRowCount(0);
    int row = 0;
    while (q.next()) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        table->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        table->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        table->setItem(row, 3, new QTableWidgetItem(
            QString("¥%1").arg(q.value(3).toDouble(), 0, 'f', 2)));
        bool active = q.value(4).toBool();
        auto* st = new QTableWidgetItem(active ? "在售" : "已下架");
        if (!active) st->setForeground(Qt::red);
        table->setItem(row, 4, st);
        row++;
    }
}

void BossWindow::onDeactivateProduct()
{
    int selRow = m_deactProdTable->currentRow();
    if (selRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要下架的商品");
        return;
    }
    int prodId = m_deactProdTable->item(selRow, 0)->text().toInt();
    QString name = m_deactProdTable->item(selRow, 1)->text();
    bool active = m_deactProdTable->item(selRow, 4)->text() == "在售";
    if (!active) {
        QMessageBox::warning(this, "提示", "该商品已下架");
        return;
    }

    auto ret = QMessageBox::question(this, "确认下架",
        QString("确认下架 %1 吗？\n（历史销售数据保留）").arg(name));
    if (ret != QMessageBox::Yes) return;

    DatabaseManager::instance().exec("UPDATE product SET is_active = 0 WHERE id = ?", { prodId });
    refreshProductList(m_deactProdTable);
}

// ────────────────────────────────────────────
//  查询商品
// ────────────────────────────────────────────
QWidget* BossWindow::createQueryProductPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("查询商品");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* row = new QHBoxLayout;
    m_queryProd = new QLineEdit;
    m_queryProd->setPlaceholderText("输入商品名称或ID...");
    auto* queryBtn = new QPushButton("查询");
    queryBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 4px 12px; border-radius: 3px; }");
    row->addWidget(m_queryProd);
    row->addWidget(queryBtn);
    root->addLayout(row);

    m_queryProdTable = new QTableWidget(0, 7);
    m_queryProdTable->setHorizontalHeaderLabels({ "ID", "名称", "分类", "售价", "库存", "批次", "状态" });
    m_queryProdTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_queryProdTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_queryProdTable, 1);

    connect(queryBtn, &QPushButton::clicked, this, &BossWindow::onQueryProduct);
    connect(m_queryProd, &QLineEdit::returnPressed, this, &BossWindow::onQueryProduct);
    return page;
}

void BossWindow::onQueryProduct()
{
    QString kw = m_queryProd->text().trimmed();
    auto& db = DatabaseManager::instance();
    QString sql = R"(
        SELECT p.id, p.name, p.category, p.price,
               COALESCE(SUM(b.remain_qty), 0) AS stock,
               p.is_active
        FROM product p
        LEFT JOIN batch b ON b.product_id = p.id AND b.remain_qty > 0
    )";
    QVariantList params;
    if (!kw.isEmpty()) {
        sql += " WHERE p.name LIKE ? OR CAST(p.id AS TEXT) = ?";
        params << ("%" + kw + "%") << kw;
    }
    sql += " GROUP BY p.id ORDER BY p.id";

    QSqlQuery q = db.exec(sql, params);
    m_queryProdTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_queryProdTable->insertRow(row);
        m_queryProdTable->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        m_queryProdTable->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        m_queryProdTable->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        m_queryProdTable->setItem(row, 3, new QTableWidgetItem(
            QString("¥%1").arg(q.value(3).toDouble(), 0, 'f', 2)));
        int stock = q.value(4).toInt();
        auto* stockItem = new QTableWidgetItem(QString::number(stock));
        if (stock == 0) stockItem->setForeground(Qt::red);
        m_queryProdTable->setItem(row, 4, stockItem);

        // 查询批次
        QString batches;
        QSqlQuery bq = db.exec(
            "SELECT batch_no, remain_qty FROM batch WHERE product_id = ? AND remain_qty > 0 "
            "ORDER BY expiry_date ASC", { q.value(0).toInt() });
        while (bq.next())
            batches += QString("%1(%2) ").arg(bq.value(0).toString()).arg(bq.value(1).toInt());
        m_queryProdTable->setItem(row, 5, new QTableWidgetItem(batches.trimmed()));

        bool active = q.value(5).toBool();
        auto* st = new QTableWidgetItem(active ? "在售" : "已下架");
        if (!active) st->setForeground(Qt::red);
        m_queryProdTable->setItem(row, 6, st);
        row++;
    }
}

// ────────────────────────────────────────────
//  库存盘点
// ────────────────────────────────────────────
QWidget* BossWindow::createInventoryCheckPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("季度盘点");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    m_invCheckTable = new QTableWidget(0, 7);
    m_invCheckTable->setHorizontalHeaderLabels({ "商品ID", "商品名", "批次号", "系统库存", "实际盘点", "差异", "保质期" });
    m_invCheckTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_invCheckTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_invCheckTable, 1);

    auto* btnRow = new QHBoxLayout;
    auto* refreshBtn = new QPushButton("加载当前库存");
    refreshBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 6px 16px; border-radius: 3px; }");
    btnRow->addWidget(refreshBtn);
    auto* saveBtn = new QPushButton("保存盘点结果");
    saveBtn->setStyleSheet("QPushButton { background: #52c41a; color: white; padding: 6px 16px; border-radius: 3px; }");
    btnRow->addWidget(saveBtn);
    btnRow->addStretch();
    root->addLayout(btnRow);

    m_invCheckStatus = new QLabel;
    root->addWidget(m_invCheckStatus);

    connect(refreshBtn, &QPushButton::clicked, this, &BossWindow::refreshInventoryCheck);
    connect(saveBtn, &QPushButton::clicked, this, &BossWindow::onSaveInventoryCheck);
    return page;
}

void BossWindow::refreshInventoryCheck()
{
    auto& db = DatabaseManager::instance();
    QSqlQuery q = db.exec(R"(
        SELECT b.product_id, p.name, b.batch_no, b.remain_qty, b.expiry_date, b.id
        FROM batch b
        JOIN product p ON b.product_id = p.id
        WHERE p.is_active = 1
        ORDER BY b.product_id, b.expiry_date
    )");
    disconnect(m_invCheckTable, &QTableWidget::cellChanged, this, nullptr);
    m_invCheckTable->blockSignals(true);
    m_invCheckTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_invCheckTable->insertRow(row);
        m_invCheckTable->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        m_invCheckTable->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        m_invCheckTable->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        int sysQty = q.value(3).toInt();
        auto* sysItem = new QTableWidgetItem(QString::number(sysQty));
        sysItem->setData(Qt::UserRole, q.value(5).toInt()); // batch_id
        sysItem->setData(Qt::UserRole + 1, sysQty);          // system qty
        m_invCheckTable->setItem(row, 3, sysItem);
        // 实际盘点列：默认等于系统库存，可编辑
        m_invCheckTable->setItem(row, 4, new QTableWidgetItem(QString::number(sysQty)));
        m_invCheckTable->setItem(row, 5, new QTableWidgetItem("0"));
        m_invCheckTable->setItem(row, 6, new QTableWidgetItem(q.value(4).toString()));
        for (int i = 0; i < 7; i++) {
            if (i == 4)continue;
            auto* it = m_invCheckTable->item(row, i);
            if (it)it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        }
        row++;
    }
    m_invCheckTable->blockSignals(false);
    // 每当用户编辑"实际盘点"列，重新计算差异
    connect(m_invCheckTable, &QTableWidget::cellChanged, this, [this](int r, int c) {
        if (c == 4 && r < m_invCheckTable->rowCount()) {
            int sysQty = m_invCheckTable->item(r, 3)->data(Qt::UserRole + 1).toInt();
            int actualQty = m_invCheckTable->item(r, 4)->text().toInt();
            int diff = actualQty - sysQty;
            auto* diffItem = m_invCheckTable->item(r, 5);
            diffItem->setText(QString::number(diff));
            diffItem->setForeground(diff == 0 ? QColor("#333") : diff > 0 ? Qt::green : Qt::red);
        }
    });
}

void BossWindow::onSaveInventoryCheck()
{
    auto& db = DatabaseManager::instance();
    db.db().transaction();

    int saved = 0;
    for (int r = 0; r < m_invCheckTable->rowCount(); r++) {
        int batchId = m_invCheckTable->item(r, 3)->data(Qt::UserRole).toInt();
        int productId = m_invCheckTable->item(r, 0)->text().toInt();
        int sysQty = m_invCheckTable->item(r, 3)->data(Qt::UserRole + 1).toInt();
        int actualQty = m_invCheckTable->item(r, 4)->text().toInt();
        int diff = actualQty - sysQty;

        if (diff != 0) {
            // 记录差异
            db.exec("INSERT INTO inventory_check (batch_id, product_id, system_qty, actual_qty, diff) "
                    "VALUES (?,?,?,?,?)",
                    { batchId, productId, sysQty, actualQty, diff });
            // 直接覆盖库存
            db.exec("UPDATE batch SET remain_qty = ? WHERE id = ?",
                    { actualQty, batchId });
            saved++;
        }
    }

    db.db().commit();
    m_invCheckStatus->setStyleSheet("color: green;");
    m_invCheckStatus->setText(QString("盘点完成！共发现 %1 项差异，库存已更新。").arg(saved));
}

// ────────────────────────────────────────────
//  每日归零
// ────────────────────────────────────────────
QWidget* BossWindow::createDailyResetPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("每日归零");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* info = new QLabel("每日关店时将当天销售额计数器归零。\n订单数据永久保留，不受归零影响。");
    info->setWordWrap(true);
    info->setStyleSheet("color: #666;");
    root->addWidget(info);

    m_resetInfo = new QLabel("今日销售额: ¥0.00\n订单数: 0");
    m_resetInfo->setStyleSheet("font-size: 16px; padding: 12px; background: #fafafa; border-radius: 6px;");
    root->addWidget(m_resetInfo);

    auto* resetBtn = new QPushButton("🔴 每日归零（关店结算）");
    resetBtn->setStyleSheet(
        "QPushButton { background: #ff4d4f; color: white; padding: 12px; "
        "font-size: 16px; font-weight: bold; border-radius: 6px; }"
        "QPushButton:hover { background: #ff7875; }");
    root->addWidget(resetBtn);

    m_resetStatus = new QLabel;
    root->addWidget(m_resetStatus);

    root->addStretch();

    connect(resetBtn, &QPushButton::clicked, this, &BossWindow::onDailyReset);
    return page;
}

void BossWindow::onDailyReset()
{
    auto ret = QMessageBox::question(this, "确认归零",
        "确定要将今日销售额计数器归零吗？\n"
        "这模拟关店结算操作，订单数据不受影响。\n\n"
        "归零后老板可以看到今日的销售总结。");
    if (ret != QMessageBox::Yes) return;

    auto& db = DatabaseManager::instance();
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    // 确保今日有记录，归零只是标记确认
    db.exec("INSERT INTO daily_counter (date, total_sales, order_count) VALUES (?,0,0) "
            "ON CONFLICT(date) DO NOTHING", { today });

    // 把今日未结算订单标记为已结算
    db.exec("UPDATE \"order\" SET settled = 1 WHERE date(create_time) = ? AND settled = 0",
            { today });

    QVariant sales = db.execScalar("SELECT total_sales FROM daily_counter WHERE date = ?", { today });
    QVariant orders = db.execScalar("SELECT order_count FROM daily_counter WHERE date = ?", { today });

    m_resetStatus->setStyleSheet("color: green; font-size: 14px; font-weight: bold;");
    m_resetStatus->setText(QString("✅ 归零完成！\n今日共 %2 笔订单，销售额 ¥%1\n明日计数将从零开始。")
        .arg(sales.toDouble(), 0, 'f', 2).arg(orders.toInt()));
}
