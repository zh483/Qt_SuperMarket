QT       += core gui widgets charts sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = MySpSystem
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    MySpSystem.cpp \
    MySpSystem/database_manager.cpp \
    MySpSystem/session_manager.cpp \
    MySpSystem/login_window.cpp \
    MySpSystem/cashier_sales_window.cpp \
    MySpSystem/cashier_inventory_window.cpp \
    MySpSystem/boss_window.cpp

HEADERS += \
    MySpSystem.h \
    MySpSystem/database_manager.h \
    MySpSystem/session_manager.h \
    MySpSystem/login_window.h \
    MySpSystem/cashier_sales_window.h \
    MySpSystem/cashier_inventory_window.h \
    MySpSystem/boss_window.h

FORMS += \
    MySpSystem.ui

RESOURCES += \
    MySpSystem.qrc

INCLUDEPATH += $$PWD/MySpSystem
