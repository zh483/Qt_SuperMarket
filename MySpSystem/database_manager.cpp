#include "database_manager.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen())
        m_db.close();
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    if (m_initialized) return true;

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qCritical() << "数据库打开失败:" << m_db.lastError().text();
        return false;
    }

    // 启用外键约束
    exec("PRAGMA foreign_keys = ON");
    exec("PRAGMA journal_mode = WAL");

    createTables();
    m_initialized = true;
    return true;
}

QSqlDatabase& DatabaseManager::db()
{
    return m_db;
}

void DatabaseManager::createTables()
{
    exec(R"(
        CREATE TABLE IF NOT EXISTS employee (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            phone TEXT DEFAULT '',
            role INTEGER NOT NULL DEFAULT 1,
            password TEXT NOT NULL DEFAULT '123456',
            is_active INTEGER DEFAULT 1,
            created_at TEXT DEFAULT (datetime('now','localtime'))
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS member (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            phone TEXT NOT NULL UNIQUE,
            name TEXT NOT NULL,
            level INTEGER DEFAULT 0,
            balance REAL DEFAULT 0.0,
            total_spent REAL DEFAULT 0.0,
            is_active INTEGER DEFAULT 1,
            created_at TEXT DEFAULT (datetime('now','localtime'))
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS product (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            category TEXT DEFAULT '',
            price REAL NOT NULL,
            discount REAL DEFAULT 1.0,
            is_active INTEGER DEFAULT 1,
            created_at TEXT DEFAULT (datetime('now','localtime'))
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS supplier (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            contact TEXT DEFAULT ''
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS batch (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id INTEGER NOT NULL,
            supplier_id INTEGER DEFAULT 0,
            batch_no TEXT NOT NULL UNIQUE,
            purchase_date TEXT NOT NULL,
            cost_price REAL NOT NULL,
            qty INTEGER NOT NULL,
            remain_qty INTEGER NOT NULL,
            expiry_date TEXT NOT NULL,
            FOREIGN KEY (product_id) REFERENCES product(id)
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            emp_id INTEGER NOT NULL,
            date TEXT NOT NULL,
            shift_type TEXT NOT NULL,
            week_start TEXT NOT NULL,
            FOREIGN KEY (emp_id) REFERENCES employee(id)
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS shift_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_emp_id INTEGER DEFAULT 0,
            to_emp_id INTEGER NOT NULL,
            shift_time TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (to_emp_id) REFERENCES employee(id)
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS "order" (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            emp_id INTEGER NOT NULL,
            member_id INTEGER DEFAULT 0,
            total REAL NOT NULL,
            pay_method TEXT NOT NULL,
            create_time TEXT DEFAULT (datetime('now','localtime')),
            settled INTEGER DEFAULT 0,
            cash_amount REAL DEFAULT 0,
            change_amount REAL DEFAULT 0,
            FOREIGN KEY (emp_id) REFERENCES employee(id)
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS order_item (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER NOT NULL,
            batch_id INTEGER NOT NULL,
            product_id INTEGER NOT NULL,
            qty INTEGER NOT NULL,
            unit_price REAL NOT NULL,
            discount REAL DEFAULT 1.0,
            FOREIGN KEY (order_id) REFERENCES "order"(id),
            FOREIGN KEY (batch_id) REFERENCES batch(id),
            FOREIGN KEY (product_id) REFERENCES product(id)
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS topup_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            member_id INTEGER NOT NULL,
            amount REAL NOT NULL,
            emp_id INTEGER NOT NULL,
            time TEXT DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (member_id) REFERENCES member(id),
            FOREIGN KEY (emp_id) REFERENCES employee(id)
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS inventory_check (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            batch_id INTEGER NOT NULL,
            product_id INTEGER NOT NULL,
            system_qty INTEGER NOT NULL,
            actual_qty INTEGER NOT NULL,
            diff INTEGER NOT NULL,
            check_date TEXT NOT NULL DEFAULT (date('now','localtime')),
            FOREIGN KEY (batch_id) REFERENCES batch(id),
            FOREIGN KEY (product_id) REFERENCES product(id)
        )
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS daily_counter (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL UNIQUE,
            total_sales REAL DEFAULT 0.0,
            order_count INTEGER DEFAULT 0
        )
    )");
}

void DatabaseManager::seedDefaultData()
{
    // 老板账号: admin / admin
    QSqlQuery q = exec(
        "SELECT COUNT(*) FROM employee WHERE role = 0 AND is_active = 1");
    if (q.next() && q.value(0).toInt() == 0) {
        exec("INSERT INTO employee (name, phone, role, password) VALUES (?,?,?,?)",
             { "老板", "13800000000", 0, "admin" });
    }

    // 演示收银员
    q = exec("SELECT COUNT(*) FROM employee WHERE role = 1 AND is_active = 1");
    if (q.next() && q.value(0).toInt() == 0) {
        exec("INSERT INTO employee (name, phone, role, password) VALUES (?,?,?,?)",
             { "张三", "13800000001", 1, "123456" });
    }

    // 演示库存员
    q = exec("SELECT COUNT(*) FROM employee WHERE role = 2 AND is_active = 1");
    if (q.next() && q.value(0).toInt() == 0) {
        exec("INSERT INTO employee (name, phone, role, password) VALUES (?,?,?,?)",
             { "李四", "13800000002", 2, "123456" });
    }

    // 演示商品
    q = exec("SELECT COUNT(*) FROM product WHERE is_active = 1");
    if (q.next() && q.value(0).toInt() == 0) {
        exec("INSERT INTO product (name, category, price) VALUES "
             "('乐事薯片原味75g', '零食', 6.5),"
             "('康师傅红烧牛肉面', '方便面', 4.5),"
             "('可口可乐330ml', '饮料', 3.0),"
             "('蒙牛纯牛奶250ml', '乳制品', 5.0),"
             "('洽洽瓜子200g', '零食', 8.0),"
             "('农夫山泉550ml', '饮料', 2.0),"
             "('奥利奥饼干原味', '零食', 9.0),"
             "('旺仔牛奶245ml', '乳制品', 5.5)");
    }

    // 演示供应商
    q = exec("SELECT COUNT(*) FROM supplier");
    if (q.next() && q.value(0).toInt() == 0) {
        exec("INSERT INTO supplier (name, contact) VALUES "
             "('宏发食品批发', '13900000001'),"
             "('利源商贸', '13900000002'),"
             "('天润乳业', '13900000003')");
    }

    // 演示批次库存（给每个商品入一批）
    q = exec("SELECT COUNT(*) FROM batch");
    if (q.next() && q.value(0).toInt() == 0) {
        auto products = exec("SELECT id FROM product WHERE is_active = 1");
        QVariantList ids;
        while (products.next()) ids.append(products.value(0).toInt());
        for (int i = 0; i < ids.size(); i++) {
            int pid = ids[i].toInt();
            int sid = (i % 3) + 1;
            QString batchNo = QString("B202607%1-001").arg(i + 1, 2, 10, QChar('0'));
            exec("INSERT INTO batch (product_id, supplier_id, batch_no, purchase_date,"
                 " cost_price, qty, remain_qty, expiry_date) VALUES (?,?,?,?,?,?,?,?)",
                 { pid, sid, batchNo, "2026-07-15", 3.0 + i * 0.5, 100, 100, "2027-07-15" });
        }
    }
}

QSqlQuery DatabaseManager::exec(const QString& sql, const QVariantList& params)
{
    QSqlQuery query(m_db);
    query.prepare(sql);
    for (const QVariant& p : params)
        query.addBindValue(p);
    if (!query.exec()) {
        qWarning() << "SQL错误:" << query.lastError().text()
                    << "\n  SQL:" << sql << "\n  Params:" << params;
    }
    return query;
}

QVariant DatabaseManager::execScalar(const QString& sql, const QVariantList& params)
{
    QSqlQuery q = exec(sql, params);
    if (q.next()) return q.value(0);
    return QVariant();
}

double DatabaseManager::memberDiscountRate(MemberLevel level)
{
    switch (level) {
    case MemberLevel::Normal: return 0.98;
    case MemberLevel::Silver: return 0.95;
    case MemberLevel::Gold:   return 0.90;
    }
    return 1.0;
}

double DatabaseManager::levelUpThreshold(MemberLevel level)
{
    switch (level) {
    case MemberLevel::Normal: return 5000.0;   // 累计 5000 → 银卡
    case MemberLevel::Silver: return 20000.0;  // 累计 20000 → 金卡
    case MemberLevel::Gold:   return 1e9;      // 封顶
    }
    return 1e9;
}
