#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QComboBox>
#include <QDateEdit>

// ── 收银员库存界面 (EmpUIk) ──
class CashierInventoryWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CashierInventoryWindow(QWidget* parent = nullptr);

private slots:
    void onNavChanged(int row);
    // 库存录入
    void onSearchProduct();
    void onProductSelected();
    void onSaveBatch();
    // 交接
    void onShiftHandover();

private:
    void setupUI();
    void setupInventoryPage(QWidget* page);
    void setupShiftPage(QWidget* page);

    QListWidget* m_navList = nullptr;
    QStackedWidget* m_stack = nullptr;

    // 库存录入控件
    QLineEdit* m_productSearch = nullptr;
    QTableWidget* m_productTable = nullptr;
    QLineEdit* m_batchNo = nullptr;
    QComboBox* m_supplierCombo = nullptr;
    QDateEdit* m_purchaseDate = nullptr;
    QDateEdit* m_expiryDate = nullptr;
    QLineEdit* m_costPrice = nullptr;
    QLineEdit* m_qty = nullptr;
    QLabel* m_invStatus = nullptr;
    QLabel* m_selectedProductLabel = nullptr;
    int m_selectedProductId = 0;

    // 交接
    QLineEdit* m_shiftEmpPhone = nullptr;
    QLabel* m_shiftStatus = nullptr;
};
