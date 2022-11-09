#include "WindowsServiceManage.h"
#include "log/LogHelper.h"

WindowsServiceManage::WindowsServiceManage()
{

}

 bool WindowsServiceManage::Start_Server(const std::string& strServiceName)
{
    bool bResult = false;
    if (strServiceName.empty())
    {
        return bResult;
    }

    SC_HANDLE sc_Manager = ::OpenSCManagerA(NULL, NULL, GENERIC_EXECUTE);
    if (sc_Manager)
    {
        SC_HANDLE sc_service = ::OpenServiceA(sc_Manager, strServiceName.c_str(), SERVICE_ALL_ACCESS);
        if (sc_service)
        {
            SERVICE_STATUS_PROCESS service_status;
            ZeroMemory(&service_status, sizeof(SERVICE_STATUS_PROCESS));
            DWORD dwpcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
            if (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
                (LPBYTE)&service_status,
                dwpcbBytesNeeded,
                &dwpcbBytesNeeded))
            {
                if (service_status.dwCurrentState == SERVICE_STOPPED)
                {
                    if (!::StartService(sc_service, NULL, NULL))
                    {
                        //LOG_ERROR("Start_Server  StartService GetLastError = %d",GetLastError());
                        ::CloseServiceHandle(sc_service);
                        ::CloseServiceHandle(sc_Manager);
                        return bResult;
                    }
                    while (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
                        (LPBYTE)&service_status,
                        dwpcbBytesNeeded,
                        &dwpcbBytesNeeded))
                    {
                        Sleep(service_status.dwWaitHint);
                        if (service_status.dwCurrentState == SERVICE_RUNNING)
                        {
                            bResult = true;
                            break;
                        }
                    }
                }
            }
            ::CloseServiceHandle(sc_service);
        }
        ::CloseServiceHandle(sc_Manager);
    }
    return bResult;
}
/*
@根据服务名停止服务
*/
 bool WindowsServiceManage::Stop_Server(const std::string& strServiceName)
{
    bool bResult = false;
    if (strServiceName.empty())
    {
        return bResult;
    }

    //const char*pAdmin = "Administrator";
    SC_HANDLE sc_Manager = ::OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    //LOG_INFO("OpenSCManagerA GetLastError = %d",GetLastError());
    qDebug()<<"sc_Manager error ="<<GetLastError();
    if (sc_Manager)
    {
        SC_HANDLE sc_service = ::OpenServiceA(sc_Manager, strServiceName.c_str(), SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_STOP);
        qDebug()<<"sc service error ="<<GetLastError();
        //LOG_INFO("OpenServiceA GetLastError = %d",GetLastError());
        if (sc_service)
        {
            SERVICE_STATUS_PROCESS service_status;
            ZeroMemory(&service_status, sizeof(SERVICE_STATUS_PROCESS));
            DWORD dwpcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
            if (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
                (LPBYTE)&service_status,
                dwpcbBytesNeeded,
                &dwpcbBytesNeeded))
            {
                SERVICE_CONTROL_STATUS_REASON_PARAMSA service_control_status;
                DWORD dwerror = NULL;
                ZeroMemory(&service_control_status, sizeof(SERVICE_CONTROL_STATUS_REASON_PARAMSA));
                if (service_status.dwCurrentState == SERVICE_RUNNING)
                {
                    service_control_status.dwReason = SERVICE_STOP_REASON_FLAG_PLANNED | SERVICE_STOP_REASON_MAJOR_APPLICATION | SERVICE_STOP_REASON_MINOR_NONE;;
                    if (!::ControlServiceExA(sc_service, SERVICE_CONTROL_STOP, SERVICE_CONTROL_STATUS_REASON_INFO, &service_control_status))
                    {
                        dwerror = ::GetLastError();
                        //LOG_ERROR("Stop_Server  ControlServiceExA GetLastError = %d",GetLastError());
                        qDebug()<<"ControlServiceExA error ="<<dwerror;
                        ::CloseServiceHandle(sc_service);
                        ::CloseServiceHandle(sc_Manager);
                        return bResult;
                    }
                    while (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
                        (LPBYTE)&service_status,
                        dwpcbBytesNeeded,
                        &dwpcbBytesNeeded))
                    {
                        Sleep(service_status.dwWaitHint);
                        if (service_status.dwCurrentState == SERVICE_STOPPED)
                        {

                            bResult = true;
                            break;
                        }
                    }
                }
            }
            ::CloseServiceHandle(sc_service);
        }
        ::CloseServiceHandle(sc_Manager);
    }
    return bResult;
}
/*
@根据服务名查询服务状态
*/
DWORD WindowsServiceManage::Query_Server_Status(const std::string& strServiceName)
{
    DWORD nResult = 0;
    if (strServiceName.empty())
    {
        return nResult;
    }
    SC_HANDLE sc_Manager = ::OpenSCManagerA(NULL, NULL, GENERIC_EXECUTE);
    if (sc_Manager)
    {
        SC_HANDLE sc_service = ::OpenServiceA(sc_Manager, strServiceName.c_str(), SERVICE_ALL_ACCESS);
        if (sc_service)
        {
            SERVICE_STATUS_PROCESS service_status;
            ZeroMemory(&service_status, sizeof(SERVICE_STATUS_PROCESS));
            DWORD dwpcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
            if (::QueryServiceStatusEx(sc_service, SC_STATUS_PROCESS_INFO,
                (LPBYTE)&service_status,
                dwpcbBytesNeeded,
                &dwpcbBytesNeeded))
            {
                nResult = service_status.dwCurrentState;
            }
            ::CloseServiceHandle(sc_service);
        }
        ::CloseServiceHandle(sc_Manager);
    }
    return nResult;
}
/*
@ 根据服务名 安装服务
*/
bool WindowsServiceManage::SerivceInstall(const std::string& strServiceName)
{
    bool bResult = false;
    SC_HANDLE schSCManger;
    SC_HANDLE schService;
    CHAR szPath[MAX_PATH];
    ZeroMemory(&szPath, MAX_PATH);
    if (!::GetModuleFileNameA(NULL, szPath, MAX_PATH)) {
        //LOG_INFO << "不能安装服务 (" << ::GetLastError() << ")\r\n";
        return false;
    }
    //获取SCM数据库的句柄。
    schSCManger = ::OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManger) {
        //LOG_INFO << "打开服务管理器失败 (" << ::GetLastError() << ")\r\n";
        return false;
    }
    //创建服务
    schService = ::CreateServiceA(schSCManger, strServiceName.c_str(),
        strServiceName.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        szPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    if (!schService)
    {
        //LOG_INFO << "创建服务失败 (" << ::GetLastError() << ")\r\n";
        ::CloseServiceHandle(schSCManger);
        return bResult;
    }
    else
    {
        //LOG_INFO << "创建服务成功\r\n";
        //::StartService(schService, NULL, NULL);
        bResult = true;
    }

    ::CloseServiceHandle(schService);
    ::CloseServiceHandle(schSCManger);
    return bResult;
}
