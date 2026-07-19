#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>

// ── 登录窗口 ──
class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);

private slots:
    void onLogin();

private:
    void setupUI();

    QLineEdit* m_phoneEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
};
