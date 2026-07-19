#include "login_window.h"
#include "database_manager.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei', 'SimHei', sans-serif; }");

    // 初始化数据库
    if (!DatabaseManager::instance().initialize("mysp.db")) {
        return -1;
    }
    //DatabaseManager::instance().seedDefaultData();
    //DatabaseManager::instance().seedDefaultData();

    LoginWindow login;
    login.show();
    return app.exec();
}
