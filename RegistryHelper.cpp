#include <ObjBase.h>
#include<QtDebug>
#include "RegistryHelper.h"
#include "Log/LogHelper.h"

using namespace std;

DWORD RegistryHelper::windowsVersion = 0;
bool RegistryHelper::RegRestore(std::wstring key,const std::wstring& fileName)
{
    HANDLE   hToken        = NULL;
    LUID sedebugnameValue  ;
    TOKEN_PRIVILEGES   tkp;

    if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) )
    {
        qDebug()<<"OpenProcessToken   failed";
        return false;
    }

    if(!LookupPrivilegeValue(NULL,SE_RESTORE_NAME,&sedebugnameValue))
    {
        qDebug()<<"LookupPrivilegeValue failed";
        return false;
    }
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = sedebugnameValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if(!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
    {
        qDebug()<<"AdjustTokenPrivileges";
        return  false;
    }
    CloseHandle(hToken);

    //bool bEnableRet = EnablePriviledge(SE_BACKUP_NAME);
    qDebug()<<"RegRestore key = "<<key;
    HKEY keyHandle = openKey(key, KEY_READ|KEY_WOW64_64KEY);
    LSTATUS result = RegRestoreKey(keyHandle, fileName.c_str(), REG_FORCE_RESTORE);
    qDebug()<<"RegRestore = "<<result;
    qDebug()<<"keyHandle = "<<keyHandle;
    RegCloseKey(keyHandle);
    if(!result)
    {
        return true;
    }
    return false;
}

bool RegistryHelper::RegSaveFile(std::wstring key,const std::wstring& fileName)
{
    /*HANDLE   hToken        = NULL;
    LUID sedebugnameValue  ;
    TOKEN_PRIVILEGES   tkp;

    if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) )
    {
        qDebug()<<"OpenProcessToken   failed";
        return 0;
    }

    if(!LookupPrivilegeValue(NULL,SE_BACKUP_NAME,&sedebugnameValue))
    {
        qDebug()<<"LookupPrivilegeValue failed";
        return 0;
    }
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = sedebugnameValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if(!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
    {
        qDebug()<<"AdjustTokenPrivileges";
        return  0;
    }
    CloseHandle(hToken);*/
    bool bEnableRet = EnablePriviledge(SE_BACKUP_NAME);

    HKEY keyHandle = openKey(key, KEY_READ | KEY_WOW64_64KEY);
    LSTATUS result = RegSaveKey(keyHandle, fileName.c_str(), nullptr);
    qDebug()<<"RegSaveFile = "<<result;
    RegCloseKey(keyHandle);
    if(!result)
    {
        return true;
    }

    return false;
}

std::vector<std::wstring> RegistryHelper::readMultiValue(std::wstring key,
                                                         std::wstring valuename)
{
    std::vector<std::wstring>vecResult;
    HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

    LSTATUS status;
    DWORD type;
    DWORD bufSize;
    status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
    if (status != ERROR_SUCCESS)
    {
        RegCloseKey(keyHandle);
        //throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
    }

    if (type != REG_MULTI_SZ &&type != REG_SZ)
    {
        RegCloseKey(keyHandle);
        throw RegistryException(L"Registry value " + key + L"\\" + valuename + L" has wrong type");
    }

    vector<wchar_t> temp(bufSize/sizeof(wchar_t));
    wchar_t* buf = new wchar_t[bufSize / sizeof(wchar_t) + 1];
    status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, (LPBYTE)buf, &bufSize);

    RegCloseKey(keyHandle);

    if (status != ERROR_SUCCESS)
    {
        delete buf;
        //throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
    }

    // Remove zero-termination
    if (buf[bufSize / sizeof(wchar_t) - 1] == L'\0')
        bufSize -= sizeof(wchar_t);

    wstring result = wstring((wchar_t*)buf, (wstring::size_type)bufSize / sizeof(wchar_t));
    delete buf;
    //vecResult.push_back(result);
    QString strName = QString::fromStdWString(result);
    QChar h0 = 0x00;
    strName.replace(h0,"\n");
    /*int size_pos = strName.indexOf("\n");
    qDebug()<<"size_pos = "<<size_pos;
    qDebug()<<"result = "<<strName;
    //result.replace(L"\0",);
    while(-1 != size_pos)
    {
        vecResult.push_back(strName.mid(0,size_pos).toStdWString());
        strName = strName.mid(size_pos + 1);
        size_pos = strName.indexOf("\n");
    }*/
    int size_pos = result.find(L'\u0000');
    //qDebug()<<"size_pos = "<<size_pos;
    //qDebug()<<"result = "<<result;
    //qDebug()<<"RegistryHelper::readMultiValue key = "<<QString::fromStdWString(key);
    //qDebug()<<"RegistryHelper::readMultiValue valuename = "<<QString::fromStdWString(valuename);
    if(-1 == size_pos)
    {
        if(!result.empty())
        {
            vecResult.push_back(result);
        }

    }
    else
    {
        while(-1 != size_pos)
        {
            vecResult.push_back(result.substr(0,size_pos));
            result = result.substr(size_pos + 1);
            size_pos = result.find(L'\u0000');
            //qDebug()<<"111 result = "<<result;
            //qDebug()<<"111 size_pos = "<<size_pos;
        }
    }

    return vecResult;
}

wstring RegistryHelper::readValue(wstring key, wstring valuename)
{
    wstring result;

    HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

    LSTATUS status;
    DWORD type;
    DWORD bufSize;
    status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
    if (status != ERROR_SUCCESS)
    {
        RegCloseKey(keyHandle);
        //throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
    }

    if (type != REG_SZ)
    {
        RegCloseKey(keyHandle);
        throw RegistryException(L"Registry value " + key + L"\\" + valuename + L" has wrong type");
    }

    wchar_t* buf = new wchar_t[bufSize / sizeof(wchar_t) + 1];
    status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, (LPBYTE)buf, &bufSize);

    RegCloseKey(keyHandle);

    if (status != ERROR_SUCCESS)
    {
        delete buf;
        //throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
    }

    // Remove zero-termination
    if (buf[bufSize / sizeof(wchar_t) - 1] == L'\0')
        bufSize -= sizeof(wchar_t);
    result = wstring((wchar_t*)buf, (wstring::size_type)bufSize / sizeof(wchar_t));
    delete buf;

    return result;
}

unsigned long RegistryHelper::readDWORDValue(wstring key, wstring valuename)
{
    unsigned long result;
    //qDebug()<<"RegistryHelper::readDWORDValue  valuename = "<<QString::fromStdWString(valuename);
    //LOG_INFO("RegistryHelper::readDWORDValue  valuename = %s",valuename.c_str());
    HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

    LSTATUS status;
    DWORD type;
    DWORD bufSize;
    status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
    if (status != ERROR_SUCCESS)
    {
        RegCloseKey(keyHandle);
        //throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
        return  result;
    }

    if (type != REG_DWORD)
    {
        RegCloseKey(keyHandle);
        throw RegistryException(L"Registry value " + key + L"\\" + valuename + L" has wrong type");
    }

    BYTE* buf = new BYTE[bufSize];
    status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, buf, &bufSize);

    RegCloseKey(keyHandle);

    if (status != ERROR_SUCCESS)
    {
        delete buf;
        //throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
    }

    result = ((unsigned long*)buf)[0];
    delete buf;

    return result;
}

bool RegistryHelper::keyExists(wstring key)
{
    bool result;

    HKEY rootKey;
    wstring subKey = splitKey(key, &rootKey);

    HKEY keyHandle;
    result = (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle) == ERROR_SUCCESS);

    if (result)
        RegCloseKey(keyHandle);

    return result;
}

bool RegistryHelper::valueExists(wstring key, wstring valuename)
{
    //qDebug()<<"RegistryHelper::valueExists";
    //LOG_INFO("RegistryHelper::valueExists");
    HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

    DWORD type;
    DWORD bufSize;
    LSTATUS status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
    RegCloseKey(keyHandle);
    //qDebug()<<"RegistryHelper::valueExists = "<<status;
    //LOG_INFO("RegistryHelper::valueExists status =%d",status);
    return status == ERROR_SUCCESS;
}


vector<wstring> RegistryHelper::enumSubValues(wstring key)
{
    vector<wstring> result;

    HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

    wchar_t keyName[256];
    DWORD keyLength = sizeof(keyName) / sizeof(wchar_t);
    int i = 0;

    LSTATUS status;
    while ((status = RegEnumValueW(keyHandle, i++, keyName, &keyLength, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
    {
        keyLength = sizeof(keyName) / sizeof(wchar_t);

        result.push_back(keyName);
    }

    RegCloseKey(keyHandle);

    //if (status != ERROR_NO_MORE_ITEMS)
    //	throw RegistryException(L"Error while enumerating sub keys of registry key " + key + L": " + StringHelper::getSystemErrorString(status));

    return result;
}

vector<wstring> RegistryHelper::enumSubKeys(wstring key)
{
    vector<wstring> result;
    std::string strKey = std::string(key.begin(),key.end());
    //LOG_INFO("RegistryHelper::enumSubKeys Start key = %s",strKey.c_str());
    HKEY keyHandle = openKey(key, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY);

    wchar_t keyName[256];
    DWORD keyLength = sizeof(keyName) / sizeof(wchar_t);
    int i = 0;

    LSTATUS status;
    while ((status = RegEnumKeyExW(keyHandle, i++, keyName, &keyLength, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
    {
        //LOG_INFO("RegistryHelper::enumSubKeys i++ = %d",i);
        keyLength = sizeof(keyName) / sizeof(wchar_t);

        result.push_back(keyName);
    }

    RegCloseKey(keyHandle);

    //if (status != ERROR_NO_MORE_ITEMS)
    //	throw RegistryException(L"Error while enumerating sub keys of registry key " + key + L": " + StringHelper::getSystemErrorString(status));
    //LOG_INFO("RegistryHelper::enumSubKeys End");
    return result;
}

bool RegistryHelper::keyEmpty(wstring key)
{
    HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

    DWORD keyCount;
    DWORD valueCount;
    LSTATUS status = RegQueryInfoKeyW(keyHandle, NULL, NULL, NULL, &keyCount, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);

    RegCloseKey(keyHandle);

    //if (status != ERROR_SUCCESS)
    //	throw RegistryException(L"Error while reading info for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

    return keyCount == 0 && valueCount == 0;
}

wstring RegistryHelper::getGuidString(GUID guid)
{
    wchar_t* temp;
    if (FAILED(StringFromCLSID(guid, &temp)))
        throw RegistryException(L"Could not convert GUID to string");
    std::wstring result(temp);
    CoTaskMemFree(temp);

    return result;
}

HKEY RegistryHelper::openKey(const wstring& key, REGSAM samDesired)
{
    HKEY rootKey;
    wstring subKey = splitKey(key, &rootKey);

    HKEY keyHandle;
    LSTATUS status = RegOpenKeyExW(rootKey, subKey.c_str(), 0, samDesired, &keyHandle);
    //qDebug()<<"openKey status = "<<status;
    //qDebug()<<"openKey key = "<<key;
    //if (status != ERROR_SUCCESS)
    //    throw RegistryException(L"Error while opening registry key " + key + L": " + StringHelper::getSystemErrorString(status));

    return keyHandle;
}

wstring RegistryHelper::splitKey(const wstring& key, HKEY* rootKey)
{
    size_t pos = key.find(L'\\');
    if (pos == wstring::npos)
        throw RegistryException(L"Key " + key + L" has invalid format");

    wstring rootPart = key.substr(0, pos);
    wstring pathPart = key.substr(pos + 1);

//	wstring p = StringHelper::toUpperCase(rootPart);
    wstring p = rootPart;
    if (p == L"HKEY_CLASSES_ROOT")
        *rootKey = HKEY_CLASSES_ROOT;
    else if (p == L"HKEY_CURRENT_CONFIG")
        *rootKey = HKEY_CURRENT_CONFIG;
    else if (p == L"HKEY_CURRENT_USER")
        *rootKey = HKEY_CURRENT_USER;
    else if (p == L"HKEY_LOCAL_MACHINE")
        *rootKey = HKEY_LOCAL_MACHINE;
    else if (p == L"HKEY_USERS")
        *rootKey = HKEY_USERS;
    else
        throw RegistryException(L"Unknown root key " + rootPart);

    return pathPart;
}

bool RegistryHelper::writeDWORDValue(std::wstring key, std::wstring valuename, unsigned long value)
{

    if(RegistryHelper::createKey(key))
    {
        HKEY keyHandle = openKey(key, /*KEY_CREATE_SUB_KEY | KEY_WOW64_32KEY*/KEY_WRITE);
        long lReturn = RegSetValueEx( keyHandle, valuename.c_str(), 0L, REG_DWORD, (const BYTE *) &value, sizeof(DWORD) );

        if( ERROR_SUCCESS == lReturn )
        {
            return true;
        }
    }

    return false;
}

bool RegistryHelper::setMultiSZValue(std::wstring key, std::wstring valuename, std::wstring value)
{
     HANDLE tokenHandle;
     if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
      throw RegistryException(L"Error in OpenProcessToken while taking ownership");

     LUID luid;
     if (!LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &luid))
      throw RegistryException(L"Error in LookupPrivilegeValue while taking ownership");

     TOKEN_PRIVILEGES tp;
     tp.PrivilegeCount = 1;
     tp.Privileges[0].Luid = luid;
     tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

     if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
      throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

     HKEY keyHandle = openKey(key, WRITE_OWNER | KEY_WOW64_64KEY);
     //qDebug()<<"keyHandle = "<<keyHandle;
     PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
     if (NULL == sd)
      throw RegistryException(L"Error in SetPrivilege while taking ownership");

     if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
      throw RegistryException(L"Error in InitializeSecurityDescriptor while taking ownership");

     PSID sid = NULL;
     SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
     if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
      0, 0, 0, 0, 0, 0, &sid))
      throw RegistryException(L"Error in AllocateAndInitializeSid while taking ownership");

     if (!SetSecurityDescriptorOwner(sd, sid, FALSE))
      throw RegistryException(L"Error in SetSecurityDescriptorOwner while taking ownership");

     LSTATUS status = RegSetKeySecurity(keyHandle, OWNER_SECURITY_INFORMATION, sd);
     //if (status != ERROR_SUCCESS)
      //throw RegistryException(L"Error while setting security information for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

     tp.Privileges[0].Attributes = 0;

     if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
      throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

     FreeSid(sid);
     LocalFree(sd);

    //if(RegistryHelper::keyExists(key))

    {
        const wchar_t  *sz = value.c_str();
        HKEY keyHandle = openKey(key, /*KEY_CREATE_SUB_KEY | KEY_WOW64_32KEY*/KEY_SET_VALUE | KEY_WOW64_64KEY);

        long lReturn = RegSetValueEx( keyHandle, valuename.c_str(), 0L, REG_MULTI_SZ, (const BYTE *) sz, value.length() *2 + 2);
        //qDebug()<<"lReturn = "<<lReturn;
        //LOG_INFO("RegistryHelper::setMultiSZValue valuename =%s, value =%s,lReturn = %d",valuename.c_str(),value.c_str(),lReturn);
        RegCloseKey(keyHandle);
        if( ERROR_SUCCESS == lReturn )
        {
            return true;
        }
    }

    return false;
}


bool RegistryHelper::setSZValue(std::wstring key, std::wstring valuename, std::wstring value)
{
     HANDLE tokenHandle;
     if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
      throw RegistryException(L"Error in OpenProcessToken while taking ownership");

     LUID luid;
     if (!LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &luid))
      throw RegistryException(L"Error in LookupPrivilegeValue while taking ownership");

     TOKEN_PRIVILEGES tp;
     tp.PrivilegeCount = 1;
     tp.Privileges[0].Luid = luid;
     tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

     if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
      throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

     HKEY keyHandle = openKey(key, WRITE_OWNER | KEY_WOW64_64KEY);
     //qDebug()<<"keyHandle = "<<keyHandle;
     PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
     if (NULL == sd)
      throw RegistryException(L"Error in SetPrivilege while taking ownership");

     if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
      throw RegistryException(L"Error in InitializeSecurityDescriptor while taking ownership");

     PSID sid = NULL;
     SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
     if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
      0, 0, 0, 0, 0, 0, &sid))
      throw RegistryException(L"Error in AllocateAndInitializeSid while taking ownership");

     if (!SetSecurityDescriptorOwner(sd, sid, FALSE))
      throw RegistryException(L"Error in SetSecurityDescriptorOwner while taking ownership");

     LSTATUS status = RegSetKeySecurity(keyHandle, OWNER_SECURITY_INFORMATION, sd);
     //if (status != ERROR_SUCCESS)
      //throw RegistryException(L"Error while setting security information for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

     tp.Privileges[0].Attributes = 0;

     if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
      throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

     FreeSid(sid);
     LocalFree(sd);

    //if(RegistryHelper::keyExists(key))

    {
        const wchar_t  *sz = value.c_str();
        HKEY keyHandle = openKey(key, /*KEY_CREATE_SUB_KEY | KEY_WOW64_32KEY*/KEY_SET_VALUE | KEY_WOW64_64KEY);

        long lReturn = RegSetValueEx( keyHandle, valuename.c_str(), 0L, REG_SZ, (const BYTE *) sz, value.length() *2 + 2);
        //qDebug()<<"lReturn = "<<lReturn;
        //LOG_INFO("RegistryHelper::setMultiSZValue valuename =%s, value =%s,lReturn = %d",valuename.c_str(),value.c_str(),lReturn);
        RegCloseKey(keyHandle);
        if( ERROR_SUCCESS == lReturn )
        {
            return true;
        }
    }

    return false;
}

bool RegistryHelper::deleteValue(std::wstring key, std::wstring valuename)
{
    if (RegistryHelper::valueExists(key, valuename))
    {
        HKEY keyHandle = openKey(key, KEY_WRITE);
        long lReturn = RegDeleteValue(keyHandle, valuename.c_str());
        if( ERROR_SUCCESS == lReturn )
        {
            return true;
        }
    }

    return false;
}

bool RegistryHelper::createKey(std::wstring key)
{
    HKEY hKey;
    DWORD dw;
    HKEY rootKey;
    wstring subKey = splitKey(key, &rootKey);
    HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    long lReturn = RegCreateKeyEx( rootKey, subKey.c_str(), 0L, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dw);

    if( ERROR_SUCCESS == lReturn )
    {
        return true;
    }
    return false;
}

bool RegistryHelper::EnablePriviledge(LPCTSTR lpSystemName)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp = {1};
    if (OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)) {
        if (LookupPrivilegeValue(NULL,lpSystemName,&tkp.Privileges[0].Luid)) {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,(PTOKEN_PRIVILEGES) NULL, 0);
            if (GetLastError() != ERROR_SUCCESS) {
                CloseHandle(hToken);
                return false;
            }
        }
        CloseHandle(hToken);
    }
    return true;

}

bool RegistryHelper::setDWORDValue(std::wstring key, std::wstring valuename, unsigned long value)
{
    HANDLE tokenHandle;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
     throw RegistryException(L"Error in OpenProcessToken while taking ownership");

    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &luid))
     throw RegistryException(L"Error in LookupPrivilegeValue while taking ownership");

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
     throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

    HKEY keyHandle = openKey(key, WRITE_OWNER | KEY_WOW64_64KEY);
    qDebug()<<"keyHandle = "<<keyHandle;
    PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (NULL == sd)
     throw RegistryException(L"Error in SetPrivilege while taking ownership");

    if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
     throw RegistryException(L"Error in InitializeSecurityDescriptor while taking ownership");

    PSID sid = NULL;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
     0, 0, 0, 0, 0, 0, &sid))
     throw RegistryException(L"Error in AllocateAndInitializeSid while taking ownership");

    if (!SetSecurityDescriptorOwner(sd, sid, FALSE))
     throw RegistryException(L"Error in SetSecurityDescriptorOwner while taking ownership");

    LSTATUS status = RegSetKeySecurity(keyHandle, OWNER_SECURITY_INFORMATION, sd);
    //if (status != ERROR_SUCCESS)
     //throw RegistryException(L"Error while setting security information for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

    tp.Privileges[0].Attributes = 0;

    if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
     throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

    FreeSid(sid);
    LocalFree(sd);

   //if(RegistryHelper::keyExists(key))
   {
       HKEY keyHandle = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);
       long lReturn = RegSetValueEx( keyHandle, valuename.c_str(), 0L, REG_DWORD, (const BYTE *)&value, sizeof(DWORD));
       std::string strValName = std::string(valuename.begin(),valuename.end());
       //qDebug()<<"lReturn = "<<lReturn;
       //LOG_INFO("RegistryHelper::setDWORDValue valuename =%s, value =%s,lReturn = %d",strValName.c_str(),value,lReturn);
       RegCloseKey(keyHandle);
       if( ERROR_SUCCESS == lReturn )
       {
           return true;
       }
   }

   return false;
}
