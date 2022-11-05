#include <ObjBase.h>

#include "RegistryHelper.h"

using namespace std;

DWORD RegistryHelper::windowsVersion = 0;

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
	HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	DWORD type;
	DWORD bufSize;
	LSTATUS status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	RegCloseKey(keyHandle);
	return status == ERROR_SUCCESS;
}

vector<wstring> RegistryHelper::enumSubKeys(wstring key)
{
	vector<wstring> result;

	HKEY keyHandle = openKey(key, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY);

	wchar_t keyName[256];
	DWORD keyLength = sizeof(keyName) / sizeof(wchar_t);
	int i = 0;

	LSTATUS status;
	while ((status = RegEnumKeyExW(keyHandle, i++, keyName, &keyLength, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
	{
		keyLength = sizeof(keyName) / sizeof(wchar_t);

		result.push_back(keyName);
	}

	RegCloseKey(keyHandle);

	//if (status != ERROR_NO_MORE_ITEMS)
	//	throw RegistryException(L"Error while enumerating sub keys of registry key " + key + L": " + StringHelper::getSystemErrorString(status));

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
