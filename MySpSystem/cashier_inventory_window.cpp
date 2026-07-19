#include "cashier_inventory_window.h"
#include "database_manager.h"
#include "session_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDate>
#include <QDateTime>

#include "login_window.h"

// ────────────────────────────────────────────
CashierInventoryWindow::CashierInventoryWindow(QWidget* parent) : QWidget(parent)
{
    setupUI();
    auto& s = SessionManager::instance();
    setWindowTitle(QString("超市收银系统 - %1 - %2").arg(s.empName(), s.roleName()));
    resize(800, 600);
}

void CashierInventoryWindow::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 顶部栏
    auto* topBar = new QHBoxLayout;
    topBar->setContentsMargins(12, 8, 12, 8);
    m_headerLabel = new QLabel;
    auto& s = SessionManager::instance();
    m_headerLabel->setText(QString("👤 %1（%2）").arg(s.empName(), s.roleName()));
    m_headerLabel->setStyleSheet("font-size: 14px;");
    topBar->addWidget(m_headerLabel);
    topBar->addStretch();
    auto* logoutBtn = new QPushButton("退出登录");
    logoutBtn->setStyleSheet(
        "QPushButton { color: #ff4d4f; border: 1px solid #ff4d4f; padding: 4px 12px; border-radius: 3px; }"
        "QPushButton:hover { background: #fff1f0; }");
    topBar->addWidget(logoutBtn);
    root->addLayout(topBar);

    // 下方：左侧导航 + 右侧页面
    auto* body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);

    m_navList = new QListWidget;
    m_navList->setFixedWidth(120);
    m_navList->addItem("库存录入");
    m_navList->addItem("交接员工");
    m_navList->setCurrentRow(0);
    m_navList->setStyleSheet(
        "QListWidget { border: none; background: #f5f5f5; font-size: 14px; }"
        "QListWidget::item { padding: 12px 8px; }"
        "QListWidget::item:selected { background: #1890ff; color: white; }");
    body->addWidget(m_navList);

    m_stack = new QStackedWidget;

    auto* invPage = new QWidget;
    setupInventoryPage(invPage);
    m_stack->addWidget(invPage);

    auto* shiftPage = new QWidget;
    setupShiftPage(shiftPage);
    m_stack->addWidget(shiftPage);

    body->addWidget(m_stack, 1);
    root->addLayout(body, 1);
    connect(m_navList, &QListWidget::currentRowChanged, this, &CashierInventoryWindow::onNavChanged);
    connect(logoutBtn, &QPushButton::clicked, this, &CashierInventoryWindow::onLogout);
}

void CashierInventoryWindow::onNavChanged(int row)
{
    m_stack->setCurrentIndex(row);
}

void CashierInventoryWindow::onLogout()
{
    SessionManager::instance().logout();
    auto* login = new LoginWindow;
    login->show();
    this->close();
}

// ────────────────────────────────────────────
// 库存录入页面
// ────────────────────────────────────────────
void CashierInventoryWindow::setupInventoryPage(QWidget* page)
{
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* title = new QLabel("库存录入");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    // 搜索商品
    auto* searchRow = new QHBoxLayout;
    m_productSearch = new QLineEdit;
    m_productSearch->setPlaceholderText("搜索商品名称或ID...");
    auto* searchBtn = new QPushButton("搜索");
    searchBtn->setStyleSheet("QPushButton { background: #1890ff; color: white; padding: 4px 12px; border-radius: 3px; }");
    searchRow->addWidget(m_productSearch, 1);
    searchRow->addWidget(searchBtn);
    root->addLayout(searchRow);

    m_productTable = new QTableWidget(0, 4);
    m_productTable->setHorizontalHeaderLabels({ "ID", "商品名", "分类", "售价" });
    m_productTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_productTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_productTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_productTable->setMaximumHeight(160);
    root->addWidget(m_productTable);

    m_selectedProductLabel = new QLabel("未选择商品");
    m_selectedProductLabel->setStyleSheet("color: #1890ff; font-weight: bold;");
    root->addWidget(m_selectedProductLabel);

    // 批次信息表单
    auto* formGroup = new QGroupBox("批次信息");
    auto* form = new QFormLayout(formGroup);

    m_batchNo = new QLineEdit;
    m_batchNo->setPlaceholderText("例如 B20260718-001");
    form->addRow("批次号:", m_batchNo);

    m_supplierCombo = new QComboBox;
    m_supplierCombo->setEditable(true);
    m_supplierCombo->setPlaceholderText("选择或输入供应商");
    form->addRow("供应商:", m_supplierCombo);

    m_purchaseDate = new QDateEdit(QDate::currentDate());
    m_purchaseDate->setCalendarPopup(true);
    form->addRow("进货日期:", m_purchaseDate);

    m_expiryDate = new QDateEdit(QDate::currentDate().addYears(1));
    m_expiryDate->setCalendarPopup(true);
    form->addRow("保质期至:", m_expiryDate);

    m_costPrice = new QLineEdit;
    m_costPrice->setPlaceholderText("进货单价");
    form->addRow("进价:", m_costPrice);

    m_qty = new QLineEdit;
    m_qty->setPlaceholderText("入库数量");
    form->addRow("数量:", m_qty);

    root->addWidget(formGroup);

    auto* saveBtn = new QPushButton("保存入库");
    saveBtn->setStyleSheet(
        "QPushButton { background: #52c41a; color: white; padding: 10px; "
        "font-size: 14px; font-weight: bold; border-radius: 4px; }");
    root->addWidget(saveBtn);

    m_invStatus = new QLabel;
    root->addWidget(m_invStatus);

    root->addStretch();

    // 加载供应商列表
    auto& db = DatabaseManager::instance();
    QSqlQuery sq = db.exec("SELECT name FROM supplier ORDER BY id");
    while (sq.next()) m_supplierCombo->addItem(sq.value(0).toString());

    connect(searchBtn, &QPushButton::clicked, this, &CashierInventoryWindow::onSearchProduct);
    connect(m_productSearch, &QLineEdit::returnPressed, this, &CashierInventoryWindow::onSearchProduct);
    connect(m_productTable, &QTableWidget::itemSelectionChanged, this, &CashierInventoryWindow::onProductSelected);
    connect(saveBtn, &QPushButton::clicked, this, &CashierInventoryWindow::onSaveBatch);
}

void CashierInventoryWindow::onSearchProduct()
{
    QString kw = m_productSearch->text().trimmed();
    auto& db = DatabaseManager::instance();
    QString sql = "SELECT id, name, category, price FROM product WHERE is_active = 1";
    QVariantList params;
    if (!kw.isEmpty()) {
        sql += " AND (name LIKE ? OR CAST(id AS TEXT) = ?)";
        params << ("%" + kw + "%") << kw;
    }
    sql += " ORDER BY id";

    QSqlQuery q = db.exec(sql, params);
    m_productTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_productTable->insertRow(row);
        m_productTable->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        m_productTable->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        m_productTable->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        m_productTable->setItem(row, 3, new QTableWidgetItem(
            QString("¥%1").arg(q.value(3).toDouble(), 0, 'f', 2)));
        row++;
    }
}

void CashierInventoryWindow::onProductSelected()
{
    int selRow = m_productTable->currentRow();
    if (selRow < 0) return;
    m_selectedProductId = m_productTable->item(selRow, 0)->text().toInt();
    QString name = m_productTable->item(selRow, 1)->text();
    m_selectedProductLabel->setText(
        QString("已选择商品: [%1] %2").arg(m_selectedProductId).arg(name));
}

void CashierInventoryWindow::onSaveBatch()
{
    if (m_selectedProductId == 0) {
        QMessageBox::warning(this, "提示", "请先选择商品！");
        return;
    }
    QString batchNo = m_batchNo->text().trimmed();
    QString supplierName = m_supplierCombo->currentText().trimmed();
    double costPrice = m_costPrice->text().toDouble();
    int qty = m_qty->text().toInt();

    if (batchNo.isEmpty() || supplierName.isEmpty() || costPrice <= 0 || qty <= 0) {
        QMessageBox::warning(this, "提示", "请填写完整的批次信息！");
        return;
    }

    auto& db = DatabaseManager::instance();

    // 检查批次号唯一性
    QSqlQuery exist = db.exec("SELECT id FROM batch WHERE batch_no = ?", { batchNo });
    if (exist.next()) {
        QMessageBox::warning(this, "批次号重复", "该批次号已存在！");
        return;
    }

    // 确保供应商存在
    QSqlQuery sq = db.exec("SELECT id FROM supplier WHERE name = ?", { supplierName });
    int supplierId;
    if (sq.next()) {
        supplierId = sq.value(0).toInt();
    } else {
        QSqlQuery ins = db.exec("INSERT INTO supplier (name) VALUES (?)", { supplierName });
        supplierId = ins.lastInsertId().toInt();
        m_supplierCombo->addItem(supplierName);
    }

    db.exec("INSERT INTO batch (product_id, supplier_id, batch_no, purchase_date, "
            "cost_price, qty, remain_qty, expiry_date) VALUES (?,?,?,?,?,?,?,?)",
            { m_selectedProductId, supplierId, batchNo,
              m_purchaseDate->date().toString("yyyy-MM-dd"),
              costPrice, qty, qty,
              m_expiryDate->date().toString("yyyy-MM-dd") });

    m_invStatus->setStyleSheet("color: green; font-weight: bold;");
    m_invStatus->setText(QString("入库成功！批次 %1，数量 %2，进价 ¥%3")
        .arg(batchNo).arg(qty).arg(costPrice, 0, 'f', 2));

    // 清空表单（保留商品选择）
    m_batchNo->clear();
    m_costPrice->clear();
    m_qty->clear();
}

// ────────────────────────────────────────────
// 交接页面（与销售版相同逻辑）
// ────────────────────────────────────────────
void CashierInventoryWindow::setupShiftPage(QWidget* page)
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

    connect(shiftBtn, &QPushButton::clicked, this, &CashierInventoryWindow::onShiftHandover);
}

void CashierInventoryWindow::onShiftHandover()
{
    QString phone = m_shiftEmpPhone->text().trimmed();
    if (phone.isEmpty()) {
        m_shiftStatus->setText("请输入接班员工手机号");
        return;
    }

    auto& db = DatabaseManager::instance();
    auto& sess = SessionManager::instance();

    QSqlQuery q = db.exec("SELECT id, name, is_active , FROM employee WHERE phone = ?", { phone });
    if (!q.next()) {
        m_shiftStatus->setStyleSheet("color: red;");
        m_shiftStatus->setText("未找到该员工");
        return;
    }
    if (q.value(0) == sess.empId()) {
        m_shiftStatus->setStyleSheet("color: red;");
        m_shiftStatus->setText("不能交接自己");
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
