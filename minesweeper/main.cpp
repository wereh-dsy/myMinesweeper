#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);//qt框架
    MainWindow w;//主窗口对象
    w.show();//显示窗口
    return QCoreApplication::exec();//进入qt事件循环
}
