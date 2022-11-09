#include "mainwindow.h"

#include <QApplication>
#include "RegistryHelper.h"
#include <QMessageBox>
#include "Log/LogHelper.h"

#define ElevocPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Elevoc"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    bool start = false;
    if (RegistryHelper::valueExists(ElevocPath, L"EnableLog"))
    {
        start = RegistryHelper::readDWORDValue(ElevocPath, L"EnableLog") == 1 ? true : false;
    }
    else
    {
        if (RegistryHelper::valueExists(ElevocPath, L"EnableLog_Up"))
        {
            start = RegistryHelper::readDWORDValue(ElevocPath, L"EnableLog_Up") == 1 ? true : false;
        }
    }

    if(!start)
    {
        QMessageBox::warning(nullptr,u8"日志开关",u8"APO 日志未生效，请打开日志配置！");
        return 0;
    }

    LogHelper::EnableLog(NGX_LOG_TRACE);

    MainWindow w;
    w.show();
    return a.exec();
}
