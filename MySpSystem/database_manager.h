#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QString>
#include <QDateTime>
#include <QDebug>

// ── 角色枚举 ──
enum class EmployeeRole { Boss = 0, SalesCashier = 1, InventoryCashier = 2 };

// ── 会员等级 ──
enum class MemberLevel { Normal = 0, Silver = 1, Gold = 2 };

// ── 班次类型 ──
namespace ShiftType {
    const QString Morning = QStringLiteral("早上");
    const QString Afternoon = QStringLiteral("下午");
    const QString Night = QStringLiteral("晚上");
    const QString Inventory = QStringLiteral("库存");
}

// ── 支付方式 ──
namespace PayMethod {
    const QString Cash = QStringLiteral("cash");
    const QString Balance = QStringLiteral("balance");
    const QString Mixed = QStringLiteral("mixed");
}

// ── 数据库单例 ──
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    bool initialize(const QString& dbPath = "mysp.db");
    void seedDefaultData();

    QSqlDatabase& db();

    // 通用查询辅助
    QSqlQuery exec(const QString& sql, const QVariantList& params = {});
    QVariant execScalar(const QString& sql, const QVariantList& params = {});

    // ── 折扣率 ──
    static double memberDiscountRate(MemberLevel level);
    static double levelUpThreshold(MemberLevel level);

private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void createTables();

    QSqlDatabase m_db;
    bool m_initialized = false;
};
