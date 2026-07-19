#include "cashier_sales_window.h"
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
#include <QDate>
#include <QTimer>

// ────────────────────────────────────────────
// 构造函数
// ────────────────────────────────────────────
CashierSalesWindow::CashierSalesWindow(QWidget* parent) : QWidget(parent)
{
    setupUI();
    auto& s = SessionManager::instance();
    setWindowTitle(QString("超市收银系统 - %1 - %2").arg(s.empName(), s.roleName()));
    resize(960, 680);
}

// ────────────────────────────────────────────
// 整体布局：左侧导航 + 右侧页面栈
// ────────────────────────────────────────────
void CashierSalesWindow::setupUI()
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    // 左侧导航
    m_navList = new QListWidget;
    m_navList->setFixedWidth(120);
    m_navList->addItem("卖东西");
    m_navList->addItem("交接员工");
    m_navList->addItem("新增会员");
    m_navList->addItem("会员充值");
    m_navList->addItem("查询会员");
    m_navList->setCurrentRow(0);
    m_navList->setStyleSheet(
        "QListWidget { border: none; background: #f5f5f5; font-size: 14px; }"
        "QListWidget::item { padding: 12px 8px; }"
        "QListWidget::item:selected { background: #1890ff; color: white; }");
    root->addWidget(m_navList);

    // 右侧页面栈
    m_stack = new QStackedWidget;

    auto* sellPage = new QWidget;
    setupSellPage(sellPage);
    m_stack->addWidget(sellPage);  // 0

    auto* shiftPage = new QWidget;
    setupShiftPage(shiftPage);
    m_stack->addWidget(shiftPage);  // 1

    auto* memberPage = new QWidget;
    setupMemberPage(memberPage);
    m_stack->addWidget(memberPage);  // 2

    auto* topUpPage = new QWidget;
    setupTopUpPage(topUpPage);
    m_stack->addWidget(topUpPage);  // 3

    auto* queryPage = new QWidget;
    setupQueryMemberPage(queryPage);
    m_stack->addWidget(queryPage);  // 4

    root->addWidget(m_stack, 1);

    connect(m_navList, &QListWidget::currentRowChanged, this, &CashierSalesWindow::onNavChanged);
}

// ────────────────────────────────────────────
// 导航切换
// ────────────────────────────────────────────
void CashierSalesWindow::onNavChanged(int row)
{
    m_stack->setCurrentIndex(row);
}

// ────────────────────────────────────────────
// 卖东西页面
// ────────────────────────────────────────────
void CashierSalesWindow::setupSellPage(QWidget* page)
{
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // ── 顶部：商品搜索 + 添加 ──
    auto* topRow = new QHBoxLayout;
    m_productSearch = new QLineEdit;
    m_productSearch->setPlaceholderText("输入商品名称或ID搜索...");
    auto* searchBtn = new QPushButton("搜索");
    searchBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 4px 12px; border-radius: 3px; }");
    auto* addBtn = new QPushButton("加入购物车 →");
    addBtn->setStyleSheet("QPushButton { background: #52c41a; color: white; padding: 4px 12px; border-radius: 3px; font-weight: bold; }");
    topRow->addWidget(m_productSearch, 1);
    topRow->addWidget(searchBtn);
    topRow->addWidget(addBtn);
    root->addLayout(topRow);

    // ── 搜索结果表格 ──
    m_searchResultTable = new QTableWidget(0, 5);
    m_searchResultTable->setHorizontalHeaderLabels({ "ID", "商品名", "分类", "单价", "库存" });
    m_searchResultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_searchResultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_searchResultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_searchResultTable->horizontalHeader()->setStretchLastSection(true);
    m_searchResultTable->setMaximumHeight(180);
    root->addWidget(m_searchResultTable);

    // ── 购物车表格 ──
    auto* cartLabel = new QLabel("🛒 购物车");
    cartLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    root->addWidget(cartLabel);

    m_cartTable = new QTableWidget(0, 5);
    m_cartTable->setHorizontalHeaderLabels({ "商品名", "批次", "单价", "数量", "小计" });
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_cartTable, 1);

    auto* cartBtns = new QHBoxLayout;
    auto* removeBtn = new QPushButton("移除选中商品");
    removeBtn->setStyleSheet("QPushButton { color: #ff4d4f; }");
    cartBtns->addWidget(removeBtn);
    cartBtns->addStretch();
    root->addLayout(cartBtns);

    // ── 会员栏 ──
    auto* memberRow = new QHBoxLayout;
    memberRow->addWidget(new QLabel("会员手机号:"));
    m_memberPhoneEdit = new QLineEdit;
    m_memberPhoneEdit->setPlaceholderText("输入会员手机号（非会员可留空）");
    m_memberPhoneEdit->setMaximumWidth(200);
    auto* lookupBtn = new QPushButton("查询会员");
    auto* clearMemberBtn = new QPushButton("清除会员");
    m_memberInfoLabel = new QLabel;
    m_memberInfoLabel->setStyleSheet("color: #1890ff;");
    memberRow->addWidget(m_memberPhoneEdit);
    memberRow->addWidget(lookupBtn);
    memberRow->addWidget(clearMemberBtn);
    memberRow->addWidget(m_memberInfoLabel);
    memberRow->addStretch();
    root->addLayout(memberRow);

    // ── 金额汇总 ──
    auto* sumRow = new QHBoxLayout;
    m_totalLabel = new QLabel("商品合计: ¥0.00");
    m_discountLabel = new QLabel("折扣: -¥0.00");
    m_finalLabel = new QLabel("应付: ¥0.00");
    m_finalLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #ff4d4f;");
    sumRow->addWidget(m_totalLabel);
    sumRow->addWidget(m_discountLabel);
    sumRow->addWidget(m_finalLabel);
    sumRow->addStretch();
    root->addLayout(sumRow);

    // ── 结账按钮 ──
    m_checkoutBtn = new QPushButton("确认付款");
    m_checkoutBtn->setStyleSheet(
        "QPushButton { background: #ff4d4f; color: white; padding: 10px; "
        "font-size: 16px; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background: #ff7875; }");
    root->addWidget(m_checkoutBtn);

    // ── 连接信号 ──
    connect(searchBtn, &QPushButton::clicked, this, &CashierSalesWindow::onSearchProduct);
    connect(m_productSearch, &QLineEdit::returnPressed, this, &CashierSalesWindow::onSearchProduct);
    connect(addBtn, &QPushButton::clicked, this, &CashierSalesWindow::onAddToCart);
    connect(removeBtn, &QPushButton::clicked, this, &CashierSalesWindow::onRemoveFromCart);
    connect(lookupBtn, &QPushButton::clicked, this, &CashierSalesWindow::onLookupMember);
    connect(clearMemberBtn, &QPushButton::clicked, this, &CashierSalesWindow::onClearMember);
    connect(m_checkoutBtn, &QPushButton::clicked, this, &CashierSalesWindow::onCheckout);
}

// ────────────────────────────────────────────
// 搜索商品
// ────────────────────────────────────────────
void CashierSalesWindow::onSearchProduct()
{
    QString kw = m_productSearch->text().trimmed();
    if (kw.isEmpty()) {
        // 显示全部有库存的在售商品
        refreshProductSearch("");
        return;
    }
    refreshProductSearch(kw);
}

void CashierSalesWindow::refreshProductSearch(const QString& keyword)
{
    auto& db = DatabaseManager::instance();
    QString sql = R"(
        SELECT DISTINCT p.id, p.name, p.category, p.price,
               (SELECT COALESCE(SUM(b.remain_qty), 0) FROM batch b
                WHERE b.product_id = p.id AND b.remain_qty > 0 AND b.expiry_date > date('now','localtime'))
        FROM product p
        WHERE p.is_active = 1
    )";
    QVariantList params;
    if (!keyword.isEmpty()) {
        sql += " AND (p.name LIKE ? OR CAST(p.id AS TEXT) = ?)";
        params << ("%" + keyword + "%") << keyword;
    }
    sql += " ORDER BY p.id";

    QSqlQuery q = db.exec(sql, params);
    m_searchResultTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_searchResultTable->insertRow(row);
        m_searchResultTable->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        m_searchResultTable->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        m_searchResultTable->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        m_searchResultTable->setItem(row, 3, new QTableWidgetItem(
            QString("¥%1").arg(q.value(3).toDouble(), 0, 'f', 2)));
        int stock = q.value(4).toInt();
        auto* stockItem = new QTableWidgetItem(QString::number(stock));
        if (stock <= 0) stockItem->setForeground(Qt::red);
        m_searchResultTable->setItem(row, 4, stockItem);
        row++;
    }
}

// ────────────────────────────────────────────
// 加入购物车
// ────────────────────────────────────────────
void CashierSalesWindow::onAddToCart()
{
    int selRow = m_searchResultTable->currentRow();
    if (selRow < 0) {
        QMessageBox::warning(this, "提示", "请先在搜索结果中选择一个商品");
        return;
    }

    int productId = m_searchResultTable->item(selRow, 0)->text().toInt();

    // 检查库存
    auto& db = DatabaseManager::instance();
    auto batchQ = db.exec(
        "SELECT id, batch_no, remain_qty, expiry_date FROM batch "
        "WHERE product_id = ? AND remain_qty > 0 AND expiry_date > date('now','localtime') "
        "ORDER BY expiry_date ASC LIMIT 1",
        { productId });

    if (!batchQ.next()) {
        QMessageBox::warning(this, "库存不足", "该商品暂无可用库存或已过期！");
        return;
    }

    int batchId = batchQ.value(0).toInt();
    QString batchNo = batchQ.value(1).toString();

    // 检查购物车是否已有同批次商品（累加数量）
    for (int r = 0; r < m_cartTable->rowCount(); r++) {
        if (m_cartTable->item(r, 1)->data(Qt::UserRole).toInt() == batchId) {
            int curQty = m_cartTable->item(r, 3)->text().toInt();
            m_cartTable->item(r, 3)->setText(QString::number(curQty + 1));
            recalcTotal();
            return;
        }
    }

    // 新加一行
    int row = m_cartTable->rowCount();
    m_cartTable->insertRow(row);

    QString name = m_searchResultTable->item(selRow, 1)->text();
    double price = m_searchResultTable->item(selRow, 3)->text().mid(1).toDouble();

    auto* nameItem = new QTableWidgetItem(name);
    nameItem->setData(Qt::UserRole, productId);
    m_cartTable->setItem(row, 0, nameItem);

    auto* batchItem = new QTableWidgetItem(batchNo);
    batchItem->setData(Qt::UserRole, batchId);
    m_cartTable->setItem(row, 1, batchItem);

    m_cartTable->setItem(row, 2, new QTableWidgetItem(QString("¥%1").arg(price, 0, 'f', 2)));
    m_cartTable->setItem(row, 3, new QTableWidgetItem("1"));
    m_cartTable->setItem(row, 4, new QTableWidgetItem(QString("¥%1").arg(price, 0, 'f', 2)));

    recalcTotal();
}

// ────────────────────────────────────────────
// 从购物车移除
// ────────────────────────────────────────────
void CashierSalesWindow::onRemoveFromCart()
{
    int selRow = m_cartTable->currentRow();
    if (selRow < 0) {
        QMessageBox::warning(this, "提示", "请先在购物车中选择要移除的商品");
        return;
    }
    m_cartTable->removeRow(selRow);
    recalcTotal();
}

// ────────────────────────────────────────────
// 查找会员
// ────────────────────────────────────────────
void CashierSalesWindow::onLookupMember()
{
    QString phone = m_memberPhoneEdit->text().trimmed();
    if (phone.isEmpty()) return;

    auto& db = DatabaseManager::instance();
    QSqlQuery q = db.exec(
        "SELECT id, name, level, balance, total_spent FROM member WHERE phone = ? AND is_active = 1",
        { phone });

    if (!q.next()) {
        m_memberInfoLabel->setStyleSheet("color: red;");
        m_memberInfoLabel->setText("会员不存在或已停用");
        m_currentMemberId = 0;
        return;
    }

    m_currentMemberId = q.value(0).toInt();
    QString name = q.value(1).toString();
    m_currentMemberLevel = q.value(2).toInt();
    double balance = q.value(3).toDouble();

    QString levelName;
    switch (m_currentMemberLevel) {
    case 0: levelName = "普通"; break;
    case 1: levelName = "银卡"; break;
    case 2: levelName = "金卡"; break;
    }

    m_currentMemberDiscount = DatabaseManager::memberDiscountRate(
        static_cast<MemberLevel>(m_currentMemberLevel));

    m_memberInfoLabel->setStyleSheet("color: green;");
    m_memberInfoLabel->setText(QString("%1 | %2会员 | 折扣%3折 | 余额¥%4")
        .arg(name, levelName)
        .arg(m_currentMemberDiscount * 100, 0, 'f', 0)
        .arg(balance, 0, 'f', 2));

    recalcTotal();
}

void CashierSalesWindow::onClearMember()
{
    m_currentMemberId = 0;
    m_currentMemberLevel = 0;
    m_currentMemberDiscount = 1.0;
    m_memberPhoneEdit->clear();
    m_memberInfoLabel->clear();
    recalcTotal();
}

// ────────────────────────────────────────────
// 重新计算总额
// ────────────────────────────────────────────
void CashierSalesWindow::recalcTotal()
{
    double subtotal = 0.0;
    for (int r = 0; r < m_cartTable->rowCount(); r++) {
        double price = m_cartTable->item(r, 2)->text().mid(1).toDouble();
        int qty = m_cartTable->item(r, 3)->text().toInt();
        double lineTotal = price * qty;
        m_cartTable->item(r, 4)->setText(QString("¥%1").arg(lineTotal, 0, 'f', 2));
        subtotal += lineTotal;
    }

    // 商品层面折扣（暂时不从 item 读取，后续可扩展）
    double afterProductDiscount = subtotal;

    // 会员折上折
    double finalDiscount = subtotal * (1.0 - m_currentMemberDiscount);
    double finalTotal = subtotal * m_currentMemberDiscount;

    m_totalLabel->setText(QString("商品合计: ¥%1").arg(subtotal, 0, 'f', 2));
    m_discountLabel->setText(QString("会员折扣(-%1%): -¥%2")
        .arg((1.0 - m_currentMemberDiscount) * 100, 0, 'f', 0)
        .arg(finalDiscount, 0, 'f', 2));
    m_finalLabel->setText(QString("应付: ¥%1").arg(finalTotal, 0, 'f', 2));
}

// ────────────────────────────────────────────
// 确认付款
// ────────────────────────────────────────────
void CashierSalesWindow::onCheckout()
{
    if (m_cartTable->rowCount() == 0) {
        QMessageBox::warning(this, "提示", "购物车为空！");
        return;
    }

    double total = m_finalLabel->text().mid(QString("应付: ¥").length()).toDouble();
    double subtotal = m_totalLabel->text().mid(QString("商品合计: ¥").length()).toDouble();

    auto& db = DatabaseManager::instance();
    auto& sess = SessionManager::instance();

    // 支付方式
    QString payMethod = PayMethod::Cash;
    double cashAmount = 0;
    double changeAmount = 0;

    if (m_currentMemberId > 0) {
        // 有会员：检查余额
        double balance = db.execScalar(
            "SELECT balance FROM member WHERE id = ?", { m_currentMemberId }).toDouble();

        if (balance >= total) {
            // 余额足够
            QMessageBox mb;
            mb.setWindowTitle("确认付款");
            mb.setText(QString("应付 ¥%1\n会员余额 ¥%2\n是否从余额扣款？")
                .arg(total, 0, 'f', 2).arg(balance, 0, 'f', 2));
            mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            mb.setDefaultButton(QMessageBox::Yes);
            if (mb.exec() == QMessageBox::Yes) {
                payMethod = PayMethod::Balance;
            } else {
                payMethod = PayMethod::Cash;
            }
        } else {
            // 余额不够 → 混合支付
            double shortage = total - balance;
            QMessageBox mb;
            mb.setWindowTitle("余额不足");
            mb.setText(QString("应付 ¥%1\n会员余额 ¥%2\n差额 ¥%3\n\n是否用余额支付 ¥%2 + 现金补差额 ¥%3？")
                .arg(total, 0, 'f', 2).arg(balance, 0, 'f', 2).arg(shortage, 0, 'f', 2));
            mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            if (mb.exec() == QMessageBox::Yes) {
                payMethod = PayMethod::Mixed;
                cashAmount = shortage;
            } else {
                payMethod = PayMethod::Cash;
            }
        }
    }

    if (payMethod == PayMethod::Cash || payMethod == PayMethod::Mixed) {
        // 如果是现金或混合，需要输入实收金额
        // 简化：假设现金刚好够
        if (payMethod == PayMethod::Cash) {
            cashAmount = total;
        }
    }

    // ── 开始事务 ──
    db.db().transaction();

    // 1. 创建订单
    QSqlQuery oq = db.exec(
        "INSERT INTO \"order\" (emp_id, member_id, total, pay_method, cash_amount, change_amount) "
        "VALUES (?,?,?,?,?,?)",
        { sess.empId(),
          m_currentMemberId > 0 ? m_currentMemberId : QVariant(),
          total, payMethod, cashAmount, changeAmount });
    int orderId = oq.lastInsertId().toInt();

    // 2. 插入订单明细 + 扣减库存
    for (int r = 0; r < m_cartTable->rowCount(); r++) {
        int productId = m_cartTable->item(r, 0)->data(Qt::UserRole).toInt();
        int batchId = m_cartTable->item(r, 1)->data(Qt::UserRole).toInt();
        double unitPrice = m_cartTable->item(r, 2)->text().mid(1).toDouble();
        int qty = m_cartTable->item(r, 3)->text().toInt();

        db.exec("INSERT INTO order_item (order_id, batch_id, product_id, qty, unit_price) "
                "VALUES (?,?,?,?,?)",
                { orderId, batchId, productId, qty, unitPrice });

        // 扣减批次库存
        db.exec("UPDATE batch SET remain_qty = remain_qty - ? WHERE id = ? AND remain_qty >= ?",
                { qty, batchId, qty });
    }

    // 3. 如果是会员支付（余额或混合），扣减余额
    if (payMethod == PayMethod::Balance || payMethod == PayMethod::Mixed) {
        double deductAmount = (payMethod == PayMethod::Mixed) ?
            db.execScalar("SELECT balance FROM member WHERE id = ?", { m_currentMemberId }).toDouble()
            : total;
        db.exec("UPDATE member SET balance = balance - ? WHERE id = ?",
                { deductAmount, m_currentMemberId });
    }

    // 4. 会员累计消费 + 自动升级（按折扣前金额）
    if (m_currentMemberId > 0) {
        db.exec("UPDATE member SET total_spent = total_spent + ? WHERE id = ?",
                { subtotal, m_currentMemberId });
        double totalSpent = db.execScalar(
            "SELECT total_spent FROM member WHERE id = ?", { m_currentMemberId }).toDouble();

        MemberLevel newLevel = MemberLevel::Normal;
        if (totalSpent >= DatabaseManager::levelUpThreshold(MemberLevel::Gold))
            newLevel = MemberLevel::Gold;
        else if (totalSpent >= DatabaseManager::levelUpThreshold(MemberLevel::Silver))
            newLevel = MemberLevel::Silver;

        db.exec("UPDATE member SET level = ? WHERE id = ?",
                { static_cast<int>(newLevel), m_currentMemberId });
    }

    // 5. 更新每日计数器
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    db.exec("INSERT INTO daily_counter (date, total_sales, order_count) VALUES (?,?,1) "
            "ON CONFLICT(date) DO UPDATE SET total_sales = total_sales + ?, order_count = order_count + 1",
            { today, total, total });

    db.db().commit();

    // ── 显示结果 ──
    QString levelUpMsg;
    if (m_currentMemberId > 0) {
        int newLevel = db.execScalar("SELECT level FROM member WHERE id = ?",
                                     { m_currentMemberId }).toInt();
        if (newLevel > m_currentMemberLevel) {
            QString ln;
            switch (newLevel) { case 1: ln = "银卡"; break; case 2: ln = "金卡"; break; }
            levelUpMsg = QString("\n🎉 会员升级为 %1！").arg(ln);
        }
    }

    QMessageBox::information(this, "付款成功",
        QString("订单 #%1\n应付: ¥%2\n支付方式: %3\n找零: ¥%4%5")
        .arg(orderId).arg(total, 0, 'f', 2)
        .arg(payMethod == PayMethod::Balance ? "会员余额" :
             payMethod == PayMethod::Mixed ? "余额+现金" : "现金")
        .arg(changeAmount, 0, 'f', 2)
        .arg(levelUpMsg));

    // 清空购物车
    m_cartTable->setRowCount(0);
    onClearMember();
    recalcTotal();

    // 交接打卡（销售后自动打卡）
    db.exec("INSERT OR IGNORE INTO shift_log (from_emp_id, to_emp_id, shift_time) "
            "SELECT 0, ?, datetime('now','localtime') "
            "WHERE NOT EXISTS (SELECT 1 FROM shift_log WHERE to_emp_id = ? "
            "AND date(shift_time) = date('now','localtime'))",
            { sess.empId(), sess.empId() });
}

// ────────────────────────────────────────────
// 交接页面
// ────────────────────────────────────────────
void CashierSalesWindow::setupShiftPage(QWidget* page)
{
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("员工交接");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* form = new QFormLayout;
    m_shiftEmpPhone = new QLineEdit;
    m_shiftEmpPhone->setPlaceholderText("输入接班员工手机号");
    form->addRow("接班员工手机号:", m_shiftEmpPhone);
    root->addLayout(form);

    auto* shiftBtn = new QPushButton("确认交接");
    shiftBtn->setStyleSheet(
        "QPushButton { background: #1890ff; color: white; padding: 8px 16px; "
        "border-radius: 4px; font-size: 14px; }");
    root->addWidget(shiftBtn);

    m_shiftStatus = new QLabel;
    root->addWidget(m_shiftStatus);

    root->addStretch();

    connect(shiftBtn, &QPushButton::clicked, this, &CashierSalesWindow::onShiftHandover);
}

void CashierSalesWindow::onShiftHandover()
{
    QString phone = m_shiftEmpPhone->text().trimmed();
    if (phone.isEmpty()) {
        m_shiftStatus->setText("请输入接班员工手机号");
        return;
    }

    auto& db = DatabaseManager::instance();
    auto& sess = SessionManager::instance();

    QSqlQuery q = db.exec("SELECT id, name, is_active FROM employee WHERE phone = ?",
                          { phone });
    if (!q.next()) {
        m_shiftStatus->setStyleSheet("color: red;");
        m_shiftStatus->setText("未找到该员工");
        return;
    }
    if (q.value(2).toInt() == 0) {
        m_shiftStatus->setStyleSheet("color: red;");
        m_shiftStatus->setText("该员工已离职");
        return;
    }

    int toEmpId = q.value(0).toInt();
    QString toName = q.value(1).toString();

    db.exec("INSERT INTO shift_log (from_emp_id, to_emp_id) VALUES (?,?)",
            { sess.empId(), toEmpId });

    m_shiftStatus->setStyleSheet("color: green;");
    m_shiftStatus->setText(QString("交接成功！%1 → %2 | %3")
        .arg(sess.empName(), toName,
             QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    m_shiftEmpPhone->clear();
}

// ────────────────────────────────────────────
// 新增会员页面
// ────────────────────────────────────────────
void CashierSalesWindow::setupMemberPage(QWidget* page)
{
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("新增会员");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* form = new QFormLayout;
    m_newMemberName = new QLineEdit;
    m_newMemberName->setPlaceholderText("请输入姓名");
    form->addRow("姓名:", m_newMemberName);

    m_newMemberPhone = new QLineEdit;
    m_newMemberPhone->setPlaceholderText("请输入手机号");
    form->addRow("手机号:", m_newMemberPhone);
    root->addLayout(form);

    auto* addBtn = new QPushButton("确认新增");
    addBtn->setStyleSheet(
        "QPushButton { background: #52c41a; color: white; padding: 8px 16px; "
        "border-radius: 4px; font-size: 14px; }");
    root->addWidget(addBtn);

    m_addMemberStatus = new QLabel;
    root->addWidget(m_addMemberStatus);

    root->addStretch();

    connect(addBtn, &QPushButton::clicked, this, &CashierSalesWindow::onAddMember);
}

void CashierSalesWindow::onAddMember()
{
    QString name = m_newMemberName->text().trimmed();
    QString phone = m_newMemberPhone->text().trimmed();

    if (name.isEmpty() || phone.isEmpty()) {
        m_addMemberStatus->setStyleSheet("color: red;");
        m_addMemberStatus->setText("请填写完整信息");
        return;
    }

    auto& db = DatabaseManager::instance();
    QSqlQuery exist = db.exec("SELECT id FROM member WHERE phone = ?", { phone });
    if (exist.next()) {
        m_addMemberStatus->setStyleSheet("color: red;");
        m_addMemberStatus->setText("该手机号已注册！");
        return;
    }

    db.exec("INSERT INTO member (name, phone) VALUES (?,?)", { name, phone });
    m_addMemberStatus->setStyleSheet("color: green;");
    m_addMemberStatus->setText(QString("会员 %1 注册成功！（手机号: %2）").arg(name, phone));
    m_newMemberName->clear();
    m_newMemberPhone->clear();
}

// ────────────────────────────────────────────
// 会员充值页面
// ────────────────────────────────────────────
void CashierSalesWindow::setupTopUpPage(QWidget* page)
{
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("会员充值");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* form = new QFormLayout;
    m_topUpPhone = new QLineEdit;
    m_topUpPhone->setPlaceholderText("输入会员手机号");
    form->addRow("手机号:", m_topUpPhone);

    m_topUpAmount = new QLineEdit;
    m_topUpAmount->setPlaceholderText("充值金额");
    form->addRow("金额:", m_topUpAmount);
    root->addLayout(form);

    auto* topUpBtn = new QPushButton("确认充值");
    topUpBtn->setStyleSheet(
        "QPushButton { background: #faad14; color: white; padding: 8px 16px; "
        "border-radius: 4px; font-size: 14px; }");
    root->addWidget(topUpBtn);

    m_topUpStatus = new QLabel;
    root->addWidget(m_topUpStatus);

    root->addStretch();

    connect(topUpBtn, &QPushButton::clicked, this, &CashierSalesWindow::onTopUp);
}

void CashierSalesWindow::onTopUp()
{
    QString phone = m_topUpPhone->text().trimmed();
    double amount = m_topUpAmount->text().toDouble();

    if (phone.isEmpty() || amount <= 0) {
        m_topUpStatus->setStyleSheet("color: red;");
        m_topUpStatus->setText("请输入正确的手机号和金额");
        return;
    }

    auto& db = DatabaseManager::instance();
    auto& sess = SessionManager::instance();

    QSqlQuery q = db.exec("SELECT id, name FROM member WHERE phone = ? AND is_active = 1",
                          { phone });
    if (!q.next()) {
        m_topUpStatus->setStyleSheet("color: red;");
        m_topUpStatus->setText("会员不存在或已停用");
        return;
    }

    int memberId = q.value(0).toInt();
    QString name = q.value(1).toString();

    db.db().transaction();
    db.exec("UPDATE member SET balance = balance + ?, total_spent = total_spent + ? WHERE id = ?",
            { amount, amount, memberId });
    db.exec("INSERT INTO topup_log (member_id, amount, emp_id) VALUES (?,?,?)",
            { memberId, amount, sess.empId() });

    // 自动升级
    double totalSpent = db.execScalar(
        "SELECT total_spent FROM member WHERE id = ?", { memberId }).toDouble();
    MemberLevel newLevel = MemberLevel::Normal;
    if (totalSpent >= DatabaseManager::levelUpThreshold(MemberLevel::Gold))
        newLevel = MemberLevel::Gold;
    else if (totalSpent >= DatabaseManager::levelUpThreshold(MemberLevel::Silver))
        newLevel = MemberLevel::Silver;
    db.exec("UPDATE member SET level = ? WHERE id = ?",
            { static_cast<int>(newLevel), memberId });
    db.db().commit();

    double newBalance = db.execScalar("SELECT balance FROM member WHERE id = ?",
                                      { memberId }).toDouble();
    m_topUpStatus->setStyleSheet("color: green;");
    m_topUpStatus->setText(QString("充值成功！%1 余额: ¥%2，累计消费: ¥%3")
        .arg(name).arg(newBalance, 0, 'f', 2).arg(totalSpent, 0, 'f', 2));
    m_topUpAmount->clear();
}

// ────────────────────────────────────────────
// 查询会员页面
// ────────────────────────────────────────────
void CashierSalesWindow::setupQueryMemberPage(QWidget* page)
{
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("查询会员");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    auto* row = new QHBoxLayout;
    m_queryPhone = new QLineEdit;
    m_queryPhone->setPlaceholderText("输入会员手机号");
    auto* queryBtn = new QPushButton("查询");
    queryBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 6px 16px; border-radius: 3px; }");
    row->addWidget(m_queryPhone);
    row->addWidget(queryBtn);
    root->addLayout(row);

    m_queryResult = new QLabel;
    m_queryResult->setWordWrap(true);
    m_queryResult->setStyleSheet("font-size: 14px; line-height: 1.8;");
    root->addWidget(m_queryResult);

    root->addStretch();

    connect(queryBtn, &QPushButton::clicked, this, &CashierSalesWindow::onQueryMember);
    connect(m_queryPhone, &QLineEdit::returnPressed, this, &CashierSalesWindow::onQueryMember);
}

void CashierSalesWindow::onQueryMember()
{
    QString phone = m_queryPhone->text().trimmed();
    if (phone.isEmpty()) {
        m_queryResult->setText("请输入手机号");
        return;
    }

    auto& db = DatabaseManager::instance();
    QSqlQuery q = db.exec(
        "SELECT name, phone, level, balance, total_spent, is_active, created_at "
        "FROM member WHERE phone = ?", { phone });

    if (!q.next()) {
        m_queryResult->setStyleSheet("color: red; font-size: 14px;");
        m_queryResult->setText("会员不存在");
        return;
    }

    QString name = q.value(0).toString();
    int level = q.value(2).toInt();
    double balance = q.value(3).toDouble();
    double totalSpent = q.value(4).toDouble();
    bool active = q.value(5).toBool();
    QString createdAt = q.value(6).toString();

    QString levelName;
    switch (level) { case 0: levelName = "普通"; break; case 1: levelName = "银卡"; break; case 2: levelName = "金卡"; break; }

    double discount = DatabaseManager::memberDiscountRate(static_cast<MemberLevel>(level));

    m_queryResult->setStyleSheet("color: #333; font-size: 14px; line-height: 1.8;");
    m_queryResult->setText(QString(
        "姓名: %1\n"
        "手机号: %2\n"
        "等级: %3（%4折）\n"
        "余额: ¥%5\n"
        "累计消费: ¥%6\n"
        "状态: %7\n"
        "注册日期: %8")
        .arg(name, phone, levelName)
        .arg(discount * 100, 0, 'f', 0)
        .arg(balance, 0, 'f', 2)
        .arg(totalSpent, 0, 'f', 2)
        .arg(active ? "正常" : "已停用")
        .arg(createdAt));
}
