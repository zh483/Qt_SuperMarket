#pragma once

#include <QObject>
#include <QString>
#include "database_manager.h"

// ── 当前登录会话 ──
class SessionManager : public QObject
{
    Q_OBJECT

public:
    static SessionManager& instance();

    void login(int empId, const QString& name, EmployeeRole role);
    void logout();

    bool isLoggedIn() const;
    int empId() const;
    QString empName() const;
    EmployeeRole role() const;
    QString roleName() const;

signals:
    void loggedIn();
    void loggedOut();

private:
    SessionManager(QObject* parent = nullptr);
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    int m_empId = 0;
    QString m_empName;
    EmployeeRole m_role = EmployeeRole::SalesCashier;
    bool m_loggedIn = false;
};
