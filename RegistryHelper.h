#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class RegistryHelper
{
public:
    static std::wstring readValue(std::wstring key, std::wstring valuename);
    static unsigned long readDWORDValue(std::wstring key, std::wstring valuename);
    static std::vector<std::wstring> readMultiValue(std::wstring key, std::wstring valuename);
    static std::vector<unsigned char> readBinaryValue(std::wstring key, std::wstring valuename);
    static void writeValue(std::wstring key, std::wstring valuename, std::wstring value);
    static bool writeDWORDValue(std::wstring key, std::wstring valuename, unsigned long value);
    static void writeMultiValue(std::wstring key, std::wstring valuename, std::wstring value);
    static void writeMultiValue(std::wstring key, std::wstring valuename, std::vector<std::wstring> values);
    static bool deleteValue(std::wstring key, std::wstring valuename);
    static bool createKey(std::wstring key);
    static void deleteKey(std::wstring key);
    static void makeWritable(std::wstring key);
    static void takeOwnership(std::wstring key);
    static ACCESS_MASK getFileAccessForUser(std::wstring path, unsigned long rid);
    static std::vector<std::wstring> enumSubKeys(std::wstring key);
    static bool keyExists(std::wstring key);
    static bool valueExists(std::wstring key, std::wstring valuename);
    static bool keyEmpty(std::wstring key);
    static void saveToFile(std::wstring key, std::vector<std::wstring> valuenames, std::wstring filepath);
    static std::wstring getGuidString(GUID guid);
    static bool isWindowsVersionAtLeast(unsigned major, unsigned minor);
    static HKEY openKey(const std::wstring& key, REGSAM samDesired);
    static bool RegSaveFile(std::wstring key,const std::wstring& fileName);

    static bool RegRestore(std::wstring key,const std::wstring& fileName);

    static bool setMultiSZValue(std::wstring key, std::wstring valuename, std::wstring value);

    static bool EnablePriviledge(LPCTSTR lpSystemName);

    static bool setDWORDValue(std::wstring key, std::wstring valuename, unsigned long value);

    static std::vector<std::wstring> enumSubValues(std::wstring key);

    static bool setSZValue(std::wstring key, std::wstring valuename, std::wstring value);

private:
    static std::wstring splitKey(const std::wstring& key, HKEY* rootKey);

    static unsigned long windowsVersion;
};

class RegistryException
{
public:
    RegistryException(const std::wstring& message)
        : message(message) {}

    std::wstring getMessage()
    {
        return message;
    }

private:
    std::wstring message;
};
