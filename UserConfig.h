#ifndef USERCONFIG_H
#define USERCONFIG_H
#include <QString>
#include <vector>
#include <memory>

struct UserConfigInf
{
    QString _strCalInputAudioPath = "";
    QString _strCalOutputAudioPath = "";
    bool _isCalStatus = false;
    QString _stroutPutAudioLogPath = "";
    double _inAudioBlankHeadTimeLength = 8.950;
    double _inAudioBlankTailTimeLength = 11.820;
};

class UserConfig
{
public:
    static std::shared_ptr<UserConfig>& GetInstance(QString path)
    {
        if (nullptr == m_UserConfig)
        {
            m_UserConfig = std::shared_ptr<UserConfig>(new UserConfig(path));
        }

        return m_UserConfig;
    }

    static std::shared_ptr<UserConfig>& GetInstance()
    {
        if (nullptr == m_UserConfig)
        {
            m_UserConfig = std::shared_ptr<UserConfig>(new UserConfig("./UserConfig.conf"));
        }

        return m_UserConfig;
    }

    bool ReadConfig();

    UserConfigInf GetUiConfig(){ return m_UserConfigInfs;}

    QString GetSaveDataPath(){return m_strSaveDataPath;}

private:
    UserConfig() {};

    UserConfig(QString path) { m_path = path; };

    static std::shared_ptr<UserConfig>m_UserConfig;

    QString m_path;

    UserConfigInf m_UserConfigInfs;

    QString m_strSaveDataPath;
};

#endif // USERCONFIG_H
