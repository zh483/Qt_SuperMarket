# 超市收银管理系统（MySpSystem）

> **适用场景**：小型超市/便利店的日常收银、会员管理、库存管理和员工排班。
>
> **技术栈**：C++17 + Qt 6 (Widgets + Charts) + SQLite

---

## 目录

1. [快速概览](#快速概览)
2. [角色与权限](#角色与权限)
3. [数据库设计（核心）](#数据库设计核心)
4. [代码架构](#代码架构)
5. [核心业务逻辑](#核心业务逻辑)
6. [如何运行](#如何运行)
7. [演示数据](#演示数据)

---

## 快速概览

```
┌─────────────────────────────────────────────────────┐
│                    登录窗口                          │
│         手机号 + 密码 → 自动识别角色                  │
└────────┬──────────────┬──────────────┬──────────────┘
         │              │              │
    老板(Boss)    收银员(销售)    收银员(库存)
         │              │              │
    ┌────┴────┐   ┌────┴────┐   ┌────┴────┐
    │销售额总览│   │  卖东西  │   │ 库存录入 │
    │销售排行  │   │ 交接员工 │   │ 交接员工 │
    │员工管理  │   │ 新增会员 │   └─────────┘
    │排班管理  │   │ 会员充值 │
    │出勤统计  │   │ 查询会员 │
    │商品管理  │   └─────────┘
    │库存盘点  │
    │每日归零  │
    └─────────┘
```

整个系统只有 **3 个角色**、**12 张数据库表**，运行在一个 **SQLite 文件**（`mysp.db`）上。

---

## 角色与权限

| 角色 | 枚举值 | 能做什么 |
|------|--------|---------|
| **老板** (Boss) | `0` | 看销售报表/图表、管员工（增删排班）、管商品（上下架）、库存盘点、每日关店结算 |
| **收银员-销售** (SalesCashier) | `1` | 卖东西（搜索商品→加购物车→结账）、会员注册/充值/查询、交接班 |
| **收银员-库存** (InventoryCashier) | `2` | 商品入库（选择商品→填写批次→保存）、交接班 |

---

## 数据库设计（核心）

系统使用 **SQLite**，共 **12 张表**。下面是每张表的完整字段说明。

### 1. employee（员工表）

存储所有员工信息，包括老板和收银员。

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `name` | TEXT | 员工姓名 |
| `phone` | TEXT | 手机号（登录账号，唯一标识） |
| `role` | INTEGER | 角色：0=老板，1=销售收银员，2=库存收银员 |
| `password` | TEXT | 登录密码，默认 `123456` |
| `is_active` | INTEGER | 是否在职：1=在职，0=已离职（软删除） |
| `created_at` | TEXT | 创建时间 |

**关键点**：离职员工设为 `is_active=0`，历史数据不删除。

---

### 2. member（会员表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `phone` | TEXT UNIQUE | 手机号（唯一，用于识别会员） |
| `name` | TEXT | 会员姓名 |
| `level` | INTEGER | 等级：0=普通(98折)，1=银卡(95折)，2=金卡(90折) |
| `balance` | REAL | 账户余额（可用于支付） |
| `total_spent` | REAL | 累计消费金额（含充值，用于自动升级判断） |
| `is_active` | INTEGER | 是否有效：1=正常，0=已停用 |
| `created_at` | TEXT | 注册时间 |

**会员升级规则**：
- 普通 → 银卡：累计消费 ≥ ¥5,000
- 银卡 → 金卡：累计消费 ≥ ¥20,000
- 每次付款和充值后**自动检测并升级**

---

### 3. product（商品表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `name` | TEXT | 商品名称 |
| `category` | TEXT | 分类（如：零食、饮料、乳制品） |
| `price` | REAL | 零售价 |
| `discount` | REAL | 商品自身折扣，默认 1.0（不打折） |
| `is_active` | INTEGER | 是否在售：1=在售，0=已下架（软删除） |
| `created_at` | TEXT | 创建时间 |

---

### 4. supplier（供应商表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `name` | TEXT | 供应商名称 |
| `contact` | TEXT | 联系方式 |

**注**：录入批次时如果输入新供应商名，会自动添加到这个表。

---

### 5. batch（批次库存表）

这是库存管理的核心表，记录每批商品的进货和剩余。

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `product_id` | INTEGER FK | 关联 `product.id` |
| `supplier_id` | INTEGER | 关联 `supplier.id` |
| `batch_no` | TEXT UNIQUE | 批次号（如 `B20260718-001`），**唯一** |
| `purchase_date` | TEXT | 进货日期 |
| `cost_price` | REAL | 进货单价（用于计算利润） |
| `qty` | INTEGER | 入库数量 |
| `remain_qty` | INTEGER | 剩余数量（每卖一件减 1） |
| `expiry_date` | TEXT | 保质期截止日期 |

**关键规则**：
- 卖东西时优先取**最早过期**的批次（FEFO 原则）
- 过期商品不会出现在搜索结果中
- `remain_qty` 会随销售实时扣减

---

### 6. schedule（排班表）

老板安排员工每周哪天上哪个班次。

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `emp_id` | INTEGER FK | 员工 ID |
| `date` | TEXT | 日期（如 `2026-07-18`） |
| `shift_type` | TEXT | 班次：`早上`/`下午`/`晚上`/`库存` |
| `week_start` | TEXT | 所属周的周一日期（用于批量保存和删除） |

**排班表结构**：4 行（早上/下午/晚上/库存）× 7 列（周一~周日），每个格子选一个员工。

---

### 7. shift_log（交接/打卡记录表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `from_emp_id` | INTEGER | 交班人 ID（0 = 系统自动打卡） |
| `to_emp_id` | INTEGER | 接班/打卡人 ID |
| `shift_time` | TEXT | 时间，默认当前时间 |

**用途**：
1. 员工 A 主动交接给员工 B → `from_emp_id=A, to_emp_id=B`
2. 收银员完成一笔销售后自动打卡 → `from_emp_id=0, to_emp_id=当前员工`
3. 老板查看出勤统计时用它来计算打卡天数

---

### 8. order（订单表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键（即订单号） |
| `emp_id` | INTEGER FK | 收银员 ID |
| `member_id` | INTEGER | 会员 ID（0 = 非会员） |
| `total` | REAL | 最终实付金额（已折后） |
| `pay_method` | TEXT | 支付方式：`cash`/`balance`/`mixed` |
| `create_time` | TEXT | 下单时间 |
| `settled` | INTEGER | 是否已结算：0=未结，1=已结（每日归零用） |
| `cash_amount` | REAL | 现金部分金额 |
| `change_amount` | REAL | 找零金额 |

**支付方式说明**：
| 方式 | 含义 |
|------|------|
| `cash` | 纯现金支付 |
| `balance` | 从会员余额全额扣款 |
| `mixed` | 余额不够，差额用现金补足 |

---

### 9. order_item（订单明细表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `order_id` | INTEGER FK | 关联 `order.id` |
| `batch_id` | INTEGER FK | 关联 `batch.id`（出库批次） |
| `product_id` | INTEGER FK | 关联 `product.id` |
| `qty` | INTEGER | 购买数量 |
| `unit_price` | REAL | 销售单价 |
| `discount` | REAL | 该行折扣，默认 1.0 |

**关系**：一个订单（order）包含多个明细行（order_item），类似超市小票的每一行。

---

### 10. topup_log（充值记录表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `member_id` | INTEGER FK | 会员 ID |
| `amount` | REAL | 充值金额 |
| `emp_id` | INTEGER FK | 操作员工 ID |
| `time` | TEXT | 充值时间 |

**逻辑**：充值金额会同时累加到 `member.balance` 和 `member.total_spent`（充值也算累计消费）。

---

### 11. inventory_check（库存盘点记录表）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `batch_id` | INTEGER FK | 批次 ID |
| `product_id` | INTEGER FK | 商品 ID |
| `system_qty` | INTEGER | 系统记录的库存数量 |
| `actual_qty` | INTEGER | 实际盘点的数量 |
| `diff` | INTEGER | 差异（实际 - 系统） |
| `check_date` | TEXT | 盘点日期 |

**作用**：老板做季度盘点，输入实际库存数，系统自动计算差异并更新库存。

---

### 12. daily_counter（每日销售计数器）

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER PK | 自增主键 |
| `date` | TEXT UNIQUE | 日期（唯一） |
| `total_sales` | REAL | 当日销售总额 |
| `order_count` | INTEGER | 当日订单数 |

**作用**：每完成一笔订单就累加，老板仪表盘读取这个表展示今日/本月数据。

---

### 表关系速览图

```
employee ──1:N──→ shift_log
employee ──1:N──→ order
employee ──1:N──→ schedule
member   ──1:N──→ order
member   ──1:N──→ topup_log
product  ──1:N──→ batch
product  ──1:N──→ order_item
supplier ──1:N──→ batch
batch    ──1:N──→ order_item
order    ──1:N──→ order_item
batch    ──1:N──→ inventory_check
```

---

## 代码架构

### 文件结构

```
MySpSystem/
├── main.cpp                          # 入口：初始化数据库 → 显示登录窗口
├── MySpSystem.pro                    # Qt 项目文件
├── MySpSystem.h / .cpp               # Qt Designer 生成的窗口（未使用）
├── MySpSystem.ui                     # Qt Designer 界面文件
├── MySpSystem.qrc                    # Qt 资源文件
├── mysp.db                           # SQLite 数据库文件
│
├── MySpSystem/                       # 业务代码
│   ├── database_manager.h / .cpp     # 🔑 数据库层（单例）
│   ├── session_manager.h / .cpp      # 🔑 当前登录会话（单例）
│   ├── login_window.h / .cpp         # 登录界面
│   ├── cashier_sales_window.h / .cpp # 收银员-销售界面
│   ├── cashier_inventory_window.h / .cpp # 收银员-库存界面
│   └── boss_window.h / .cpp          # 老板界面
```

### 设计模式

| 模式 | 用在哪里 | 为什么这样设计 |
|------|---------|---------------|
| **单例模式** | `DatabaseManager`、`SessionManager` | 全局只需要一个数据库连接和一个登录状态 |
| **MVC（简化版）** | 整体架构 | View=窗口类，Model=数据库表，Controller=窗口内的槽函数 |
| **枚举+命名空间** | `EmployeeRole`、`MemberLevel`、`ShiftType`、`PayMethod` | 避免魔法数字，代码可读 |

### 核心类说明

#### 1. DatabaseManager（数据库管理器）

```cpp
// 使用方式（全局唯一实例）
DatabaseManager::instance().initialize("mysp.db");  // 启动时调用一次
DatabaseManager::instance().exec("SELECT ...", {参数});  // 执行 SQL
DatabaseManager::instance().execScalar("SELECT COUNT(*)...");  // 查单个值
```

- **单例模式**：整个程序只有一个数据库连接
- **初始化时**：自动创建所有 12 张表（`CREATE TABLE IF NOT EXISTS`）
- **`exec()` 方法**：封装了 `prepare → bindValue → exec`，用 `?` 占位防 SQL 注入
- **外键**：开启 `PRAGMA foreign_keys = ON`
- **WAL 模式**：开启 `PRAGMA journal_mode = WAL`，提升并发读写性能

#### 2. SessionManager（会话管理器）

```cpp
// 登录时
SessionManager::instance().login(empId, "张三", EmployeeRole::SalesCashier);
// 任意地方获取当前用户信息
int id = SessionManager::instance().empId();
QString name = SessionManager::instance().empName();
EmployeeRole role = SessionManager::instance().role();
```

- 存储当前登录用户的信息
- 发出 `loggedIn()` / `loggedOut()` 信号（可扩展）

#### 3. 窗口类（View 层）

| 类名 | 对应角色 | 功能模块 |
|------|---------|---------|
| `LoginWindow` | 所有人 | 手机号+密码验证，根据角色跳转 |
| `CashierSalesWindow` | 销售收银员 | 5 个页面：卖东西、交接、新增会员、充值、查会员 |
| `CashierInventoryWindow` | 库存收银员 | 2 个页面：库存录入、交接 |
| `BossWindow` | 老板 | 14 个页面：仪表盘、排行、员工CRUD、排班、出勤、商品CRUD、盘点、归零 |

所有窗口采用 **左侧导航列表 + 右侧 QStackedWidget** 的布局模式。

---

## 核心业务逻辑

### 1. 登录流程

```
输入手机号 + 密码
    │
    ▼
查询 employee 表（phone + is_active=1）
    │
    ├─ 查不到 → "账号不存在或已停用"
    ├─ 密码错 → "密码错误"
    └─ 正确 →
        │
        ├─ 角色=老板 → 打开 BossWindow
        └─ 角色=收银员 →
            查询 schedule 表：今天有排班吗？
            ├─ 有 → 按排班类型进入对应窗口
            └─ 无 → 弹窗选加班类型 → 写 shift_log → 进对应窗口
```

### 2. 卖东西流程（最核心）

```
搜索商品（支持名称模糊 / ID 精确）
    │
    ▼
显示搜索结果（商品名、分类、单价、库存）
    │
    ▼
选中商品 → "加入购物车"
    │  自动取最早过期的可用批次（FEFO）
    │  同批次再次加入只累加数量
    ▼
购物车（可移除、可修改数量）
    │
    ▼
可选：输入会员手机号 → 查询会员
    │  显示会员等级、折扣率、余额
    ▼
实时计算：商品合计 → 会员折扣（折上折）→ 应付金额
    │
    ▼
"确认付款" →
    ├─ 有会员 + 余额够 → 弹窗：是否从余额扣？
    ├─ 有会员 + 余额不够 → 弹窗：混合支付？
    └─ 无会员/选现金 → 现金支付
    │
    ▼
数据库事务（一条订单 = 4 步原子操作）：
    1. INSERT order        — 创建订单
    2. INSERT order_item   — 订单明细
    3. UPDATE batch        — 扣减库存
    4. UPDATE member       — 扣余额 / 累计消费 / 判断升级
    5. UPSERT daily_counter— 累加今日销售
    │
    ▼
提交事务 → 显示订单号 → 清空购物车
```

### 3. 会员自动升级

```cpp
// 每次付款后自动检测
if (totalSpent >= 20000) → 升级金卡（9折）
else if (totalSpent >= 5000) → 升级银卡（95折）
else → 普通会员（98折）
```

### 4. 库存入库流程

```
搜索/选择商品 → 填写批次信息 → 保存
    │
    批次信息：批次号（唯一）、供应商、进货日期、
             保质期、进价、数量
    │
    ▼
INSERT batch（remain_qty 初始 = qty）
如果供应商不存在 → 自动新增
```

### 5. 排班逻辑

- 老板在 4×7 的表格中选择每个格子的员工
- 保存时：先 `DELETE` 本周旧排班，再 `INSERT` 新排班（事务）
- 每个格子对应的班次类型：
  | 行 | 类型 | 说明 |
  |----|------|------|
  | 第1行 | 早上 | 早班销售 |
  | 第2行 | 下午 | 下午销售 |
  | 第3行 | 晚上 | 晚班销售 |
  | 第4行 | 库存 | 库存管理 |

---

## 如何运行

### 环境要求

- **Qt 6.x**（需要 Widgets、Charts、Sql 模块）
- **C++17** 编译器（MSVC 或 MinGW）
- **SQLite**（Qt 自带，无需额外安装）

### 步骤

```bash
# 1. 用 Qt Creator 打开 MySpSystem.pro

# 2. 配置 Kit（选择 Qt 6.x + 合适的编译器）

# 3. 点击 Run（绿色三角）

# 4. 首次运行会自动创建 mysp.db 并写入演示数据
```

### 如果 Qt Creator 看不到源文件

在 Qt Creator 中：
1. 关闭项目
2. 删除 `MySpSystem.vcxproj.user`（Visual Studio 遗留文件会干扰）
3. 重新用 **File → Open File or Project** 打开 `.pro` 文件

### 演示账号

| 角色 | 手机号 | 密码 |
|------|--------|------|
| 老板 | 13800000000 | admin |
| 销售收银员 | 13800000001 | 123456 |
| 库存收银员 | 13800000002 | 123456 |

---

## 演示数据

首次运行时自动写入以下数据：

- **8 种商品**：乐事薯片、康师傅方便面、可口可乐、蒙牛牛奶、洽洽瓜子、农夫山泉、奥利奥、旺仔牛奶
- **3 个供应商**：宏发食品批发、利源商贸、天润乳业
- **3 个员工**：老板 + 张三（销售）+ 李四（库存）
- **8 个批次的库存**：每个商品初始库存 100 件

---

## 关键技术点总结

| 技术点 | 实现方式 |
|--------|---------|
| 数据库 | `QSqlDatabase` + SQLite，单例管理 |
| 防 SQL 注入 | 所有 SQL 使用 `?` 占位符 + `prepare/bindValue` |
| 事务 | `db.transaction()` / `db.commit()` 保证订单原子性 |
| UI 布局 | 左侧 QListWidget 导航 + 右侧 QStackedWidget 页面栈 |
| 图表 | Qt Charts 的 `QBarSeries` + `QBarCategoryAxis`（近6月趋势） |
| 数据展示 | QTableWidget 动态加载 SQL 查询结果 |
| 密码 | 明文存储（演示项目，生产环境应使用哈希） |
| 软删除 | `is_active` 字段标记，不真正删除数据 |
