#pragma once

#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QDateEdit>
#include <QTextEdit>

// 前置声明 Qt Charts (Qt 6 中这些类在全局命名空间)
QT_BEGIN_NAMESPACE
class QChartView;
class QChart;
QT_END_NAMESPACE

// ── 老板界面 (BossUI) ──
class BossWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BossWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void refreshScheduleEmployeeLists();
private slots:
    void onNavChanged(int row);
    void onLogout();

    // 销售额总览
    void refreshDashboard();
    // 商品销售排行
    void refreshProductRanking();
    // 录入员工
    void onAddEmployee();
    // 辞退员工
    void onFireEmployee();
    void refreshEmployeeList(QTableWidget* table);
    // 员工排序
    void refreshEmployeeRanking();
    // 排班
    void refreshSchedule();
    void onSaveSchedule();
    // 值班状况
    void refreshDutyStatus();
    // 出勤统计
    void refreshAttendanceStats();
    // 录入商品
    void onAddProduct();
    // 删除商品（下架）
    void onDeactivateProduct();
    void refreshProductList(QTableWidget* table);
    // 查询商品
    void onQueryProduct();
    // 库存盘点
    void refreshInventoryCheck();
    void onSaveInventoryCheck();
    // 每日归零
    void onDailyReset();

private:
    void setupUI();
    QWidget* createDashboardPage();
    QWidget* createRankingPage();
    QWidget* createAddEmpPage();
    QWidget* createFireEmpPage();
    QWidget* createEmpRankPage();
    QWidget* createSchedulePage();
    QWidget* createDutyPage();
    QWidget* createAttendancePage();
    QWidget* createAddProductPage();
    QWidget* createDeactivateProductPage();
    QWidget* createQueryProductPage();
    QWidget* createInventoryCheckPage();
    QWidget* createDailyResetPage();

    QListWidget* m_navList = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_headerLabel = nullptr;

    // ── 各页面控件指针 ──
    // Dashboard
    QChartView* m_chartView = nullptr;
    QLabel* m_todaySalesLabel = nullptr;
    QLabel* m_todayOrdersLabel = nullptr;
    QLabel* m_monthSalesLabel = nullptr;
    QLabel* m_monthProfitLabel = nullptr;

    // 商品销售排行
    QTableWidget* m_rankingTable = nullptr;
    QDateEdit* m_rankDateFrom = nullptr;
    QDateEdit* m_rankDateTo = nullptr;

    // 录入员工
    QLineEdit* m_empName = nullptr;
    QLineEdit* m_empPhone = nullptr;
    QComboBox* m_empRole = nullptr;
    QLineEdit* m_empPassword = nullptr;
    QLabel* m_addEmpStatus = nullptr;

    // 辞退员工
    QTableWidget* m_fireEmpTable = nullptr;

    // 员工排序
    QTableWidget* m_empRankTable = nullptr;

    // 排班
    QTableWidget* m_scheduleTable = nullptr; // 4 rows × 7 cols
    QDateEdit* m_weekStart = nullptr;
    QLabel* m_scheduleStatus = nullptr;

    // 值班状况
    QDateEdit* m_dutyDate = nullptr;
    QTableWidget* m_dutyTable = nullptr;

    // 出勤统计
    QDateEdit* m_attDateFrom = nullptr;
    QDateEdit* m_attDateTo = nullptr;
    QTableWidget* m_attTable = nullptr;

    // 录入商品
    QLineEdit* m_prodName = nullptr;
    QLineEdit* m_prodCategory = nullptr;
    QLineEdit* m_prodPrice = nullptr;
    QLabel* m_addProdStatus = nullptr;

    // 删除商品
    QTableWidget* m_deactProdTable = nullptr;

    // 查询商品
    QLineEdit* m_queryProd = nullptr;
    QTableWidget* m_queryProdTable = nullptr;

    // 库存盘点
    QTableWidget* m_invCheckTable = nullptr;
    QLabel* m_invCheckStatus = nullptr;

    // 每日归零
    QLabel* m_resetStatus = nullptr;
    QLabel* m_resetInfo = nullptr;
};
