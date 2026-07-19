#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QSpinbox>

// ── 收银员销售界面 (EmpUI) ──
class CashierSalesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CashierSalesWindow(QWidget* parent = nullptr);

private slots:
    // 卖东西
    void onSearchProduct();
    void onAddToCart();
    void onRemoveFromCart();
    void recalcTotal();
    // 会员
    void onLookupMember();
    void onClearMember();
    // 支付
    void onCheckout();
    // 侧边导航
    void onNavChanged(int row);
    // 交接
    void onShiftHandover();
    // 新增会员
    void onAddMember();
    // 会员充值
    void onTopUp();
    // 查会员
    void onQueryMember();
    // 退出登录
    void onLogout();

private:
    void setupUI();
    void setupSellPage(QWidget* page);
    void setupShiftPage(QWidget* page);
    void setupMemberPage(QWidget* page);
    void setupTopUpPage(QWidget* page);
    void setupQueryMemberPage(QWidget* page);
    void refreshProductSearch(const QString& keyword);

    // ── 卖东西控件 ──
    QLineEdit* m_productSearch = nullptr;
    QTableWidget* m_searchResultTable = nullptr;
    QTableWidget* m_cartTable = nullptr;
    QLabel* m_totalLabel = nullptr;
    QLabel* m_discountLabel = nullptr;
    QLabel* m_finalLabel = nullptr;
    QLineEdit* m_memberPhoneEdit = nullptr;
    QLabel* m_memberInfoLabel = nullptr;
    QPushButton* m_checkoutBtn = nullptr;
	QSpinBox* m_currentQuantitySpinBox = nullptr;

    // 当前会员（0 = 无会员）
    int m_currentMemberId = 0;
    int m_currentMemberLevel = 0;
    double m_currentMemberDiscount = 1.0;

    // ── 导航 ──
    QListWidget* m_navList = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_headerLabel = nullptr;

    // ── 交接 ──
    QLineEdit* m_shiftEmpPhone = nullptr;
    QLabel* m_shiftStatus = nullptr;

    // ── 新增会员 ──
    QLineEdit* m_newMemberName = nullptr;
    QLineEdit* m_newMemberPhone = nullptr;
    QLabel* m_addMemberStatus = nullptr;

    // ── 充值 ──
    QLineEdit* m_topUpPhone = nullptr;
    QLineEdit* m_topUpAmount = nullptr;
    QLabel* m_topUpStatus = nullptr;

    // ── 查会员 ──
    QLineEdit* m_queryPhone = nullptr;
    QLabel* m_queryResult = nullptr;
};
