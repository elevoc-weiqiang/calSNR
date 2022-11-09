#include "UserConfig.h"
#include<QDebug>
#include<QJsonDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <iostream>

std::shared_ptr<UserConfig> UserConfig::m_UserConfig = nullptr;

bool  UserConfig::ReadConfig()
{
    qDebug()<<"ReadConfig open1";
    QFile file(m_path);
    if(!file.open(QFile::ReadOnly))
    {
        qDebug()<<"ReadConfig open";
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError jError;
    QJsonDocument jDoc = QJsonDocument::fromJson(data,&jError);

    qDebug()<<"ReadConfig open2";
    if(jError.error != QJsonParseError::NoError)
    {
        return false;
    }

    qDebug()<<"ReadConfig open3";
    QJsonObject obj = jDoc.object();
    QJsonArray jArr = obj["User"].toArray();

    /*struct UiConfigInf
{
    QString _strCalInputAudioPath = "";
    QString _strCalOutputAudioPath = "";
    bool _isCalStatus = false;
    QString _stroutPutAudioLogPath = "";
};*/
    foreach(auto e , jArr)
    {
        auto tobj = e.toObject();
        m_UserConfigInfs._strCalInputAudioPath = tobj["CalInputAudioPath"].toString();
        m_UserConfigInfs._strCalOutputAudioPath = tobj["CalOutputAudioPath"].toString();
        m_UserConfigInfs._isCalStatus = tobj["isCalStatus"].toBool();
        m_UserConfigInfs._stroutPutAudioLogPath = tobj["outPutAudioLogPath"].toString();
        qDebug()<<"m_UserConfigInfs._strCalInputAudioPath = "<<m_UserConfigInfs._strCalInputAudioPath;
        qDebug()<<"m_UserConfigInfs._strCalOutputAudioPath = "<<m_UserConfigInfs._strCalOutputAudioPath;
        qDebug()<<"m_UserConfigInfs._isCalStatus = "<<m_UserConfigInfs._isCalStatus;
        qDebug()<<"m_UserConfigInfs._stroutPutAudioLogPath = "<<m_UserConfigInfs._stroutPutAudioLogPath;
    }

    jArr = obj["OtherOptions"].toArray();
    foreach(auto e , jArr)
    {
        auto tobj = e.toObject();
        m_strSaveDataPath = tobj["OutPutFilePath"].toString();
    }

    return true;
}
