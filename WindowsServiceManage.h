#ifndef WINDOWSSERVICEMANAGE_H
#define WINDOWSSERVICEMANAGE_H

#include <Windows.h>
#include<string>
#include<QDebug>

class WindowsServiceManage
{
public:
    WindowsServiceManage();

    bool Start_Server(const std::string& strServiceName);

    bool Stop_Server(const std::string& strServiceName);

    DWORD Query_Server_Status(const std::string& strServiceName);

    bool SerivceInstall(const std::string& strServiceName);
};

#endif // WINDOWSSERVICEMANAGE_H
