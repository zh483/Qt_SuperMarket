#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MySpSystem.h"

class MySpSystem : public QMainWindow
{
    Q_OBJECT

public:
    MySpSystem(QWidget *parent = nullptr);
    ~MySpSystem();

private:
    Ui::MySpSystemClass ui;
};

