#include "login_window.h"
#include "database_manager.h"
#include "session_manager.h"
#include "cashier_sales_window.h"
#include "cashier_inventory_window.h"
#include "boss_window.h"
#include <QApplication>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDate>

LoginWindow::LoginWindow(QWidget* parent) : QWidget(parent)
{
    setupUI();
    setWindowTitle("超市收银管理系统 - 登录");
    setFixedSize(400, 280);
}

void LoginWindow::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 30, 40, 30);
    root->setSpacing(12);

    // 标题
    auto* title = new QLabel("超市收银管理系统");
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    root->addSpacing(10);

    // 表单
    auto* form = new QFormLayout;
    form->setSpacing(8);

    m_phoneEdit = new QLineEdit;
    m_phoneEdit->setPlaceholderText("请输入手机号");
    form->addRow("手机号:", m_phoneEdit);

    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow("密  码:", m_passwordEdit);

    root->addLayout(form);

    root->addSpacing(8);

    // 登录按钮
    m_loginBtn = new QPushButton("登  录");
    m_loginBtn->setStyleSheet(
        "QPushButton { background: #1890ff; color: white; padding: 8px; "
        "border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #40a9ff; }");
    root->addWidget(m_loginBtn);

    // 状态
    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: red;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    root->addStretch();

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
}

void LoginWindow::onLogin()
{
    QString phone = m_phoneEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (phone.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("请输入手机号和密码");
        return;
    }

    auto& db = DatabaseManager::instance();
    QSqlQuery q = db.exec(
        "SELECT id, name, role, password FROM employee WHERE phone = ? AND is_active = 1",
        { phone });

    if (!q.next()) {
        m_statusLabel->setText("账号不存在或已停用");
        return;
    }

    int empId = q.value(0).toInt();
    QString name = q.value(1).toString();
    int roleInt = q.value(2).toInt();
    QString dbPassword = q.value(3).toString();

    if (password != dbPassword) {
        m_statusLabel->setText("密码错误");
        return;
    }

    auto role = static_cast<EmployeeRole>(roleInt);
    auto& sess = SessionManager::instance();
    sess.login(empId, name, role);

    m_statusLabel->setStyleSheet("color: green;");
    m_statusLabel->setText("登录成功，正在跳转...");

    QWidget* nextWindow = nullptr;

    if (role == EmployeeRole::Boss) {
        nextWindow = new BossWindow;
    } else if (role == EmployeeRole::SalesCashier || role == EmployeeRole::InventoryCashier) {
        // 检查今天是否有排班
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        QSqlQuery sq = db.exec(
            "SELECT shift_type FROM schedule WHERE emp_id = ? AND date = ?",
            { empId, today });

        if (sq.next()) {
            // 有排班 → 按班次类型进对应 UI
            QString shiftType = sq.value(0).toString();
            if (shiftType == ShiftType::Inventory) {
                nextWindow = new CashierInventoryWindow;
            } else {
                nextWindow = new CashierSalesWindow;
            }
        } else {
            // 无排班 → 加班 → 员工自选
            QMessageBox mb;
            mb.setWindowTitle("加班选择");
            mb.setText("今天没有您的排班，将记为加班。\n请选择工作类型：");
            mb.setIcon(QMessageBox::Question);
            auto* salesBtn = mb.addButton("销售收银", QMessageBox::AcceptRole);
            auto* invBtn = mb.addButton("库存管理", QMessageBox::AcceptRole);
            mb.exec();

            // 记录加班
            QString chosenShift = (mb.clickedButton() == invBtn)
                ? ShiftType::Inventory : ShiftType::Morning;
            db.exec("INSERT INTO shift_log (to_emp_id, shift_time) VALUES (?, datetime('now','localtime'))",
                    { empId });

            if (mb.clickedButton() == invBtn) {
                nextWindow = new CashierInventoryWindow;
            } else {
                nextWindow = new CashierSalesWindow;
            }
        }
    }

    if (nextWindow) {
        nextWindow->show();
        this->close();
    }
}
