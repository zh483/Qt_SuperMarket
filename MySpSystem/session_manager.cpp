#include "session_manager.h"

SessionManager::SessionManager(QObject* parent) : QObject(parent) {}

SessionManager& SessionManager::instance()
{
    static SessionManager inst;
    return inst;
}

void SessionManager::login(int empId, const QString& name, EmployeeRole role)
{
    m_empId = empId;
    m_empName = name;
    m_role = role;
    m_loggedIn = true;
    emit loggedIn();
}

void SessionManager::logout()
{
    m_empId = 0;
    m_empName.clear();
    m_role = EmployeeRole::SalesCashier;
    m_loggedIn = false;
    emit loggedOut();
}

bool SessionManager::isLoggedIn() const { return m_loggedIn; }
int SessionManager::empId() const { return m_empId; }
QString SessionManager::empName() const { return m_empName; }
EmployeeRole SessionManager::role() const { return m_role; }

QString SessionManager::roleName() const
{
    switch (m_role) {
    case EmployeeRole::Boss:            return QStringLiteral("老板");
    case EmployeeRole::SalesCashier:    return QStringLiteral("员工");
    case EmployeeRole::InventoryCashier:return QStringLiteral("员工");
    }
    return QStringLiteral("未知");
}
