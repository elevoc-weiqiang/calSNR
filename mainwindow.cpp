#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include "WindowsServiceManage.h"

#define ElevocPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Elevoc"
#define IN_FILE_PATH "c:/ElevocTest/inputAudiofp32_voip.pcm"
#define OUT_FILE_PATH "c:/ElevocTest/outputAudiofp32_voip.pcm"

bool isDirExist(QString fullPath)
{
    QDir dir(fullPath);
    if(dir.exists())
    {
      return true;
    }
    else
    {
       bool ok = dir.mkpath(fullPath);//创建多级目录
       return ok;
    }
}

bool isFileExist(QString fullFileName)
{
    QFileInfo fileInfo(fullFileName);
    if(fileInfo.isFile())
    {
        return true;
    }
    return false;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    GetMicChannelNum();
    if(!UserConfig::GetInstance()->ReadConfig())
    {
        LOG_ERROR("ReadConfig","ReadConfig UserConfig.conf FAILED");
        qDebug()<<"ReadConfig UserConfig.conf FAILED";
        ui->pushButton_CalSNR->setEnabled(false);
    }
    else
    {

        m_bCalMode = UserConfig::GetInstance()->GetUiConfig()._isCalStatus;//仿真计算模式
        m_OutPutAudioLogPath = UserConfig::GetInstance()->GetUiConfig()._stroutPutAudioLogPath;
        if(m_bCalMode)
        {
            m_strPathOutAudio = UserConfig::GetInstance()->GetUiConfig()._strCalOutputAudioPath;
            m_strPathInAudio = UserConfig::GetInstance()->GetUiConfig()._strCalInputAudioPath;
            qDebug()<<"ReadConfig _isCalStatus = "<<m_bCalMode;
            ui->start->setEnabled(false);
            ui->pushButton_CalSNR->setEnabled(true);
        }
        else
        {
            m_curMicChannels = 2;//使用日志文件计算时为双通道和四通道的日志保存数据保存都为2通道
            ui->start->setEnabled(true);
            ui->pushButton_CalSNR->setEnabled(false);
        }
    }

    this->setWindowTitle("Elevoc SNR Tools V0.0.1");
    ui->statusbar->setSizeGripEnabled(false);
    ui->statusbar->showMessage(u8"待执行");
    ui->statusbar->setStyleSheet("color:blue");
    ui->progressBar->setVisible(false);
    ui->label_7->setVisible(false);
    //m_pProcessAudioStop = new QProcess(this);
    //m_pProcessAudioStart = new QProcess(this);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);

    QPalette pe;
    pe.setColor(QPalette::WindowText, QColor(Qt::GlobalColor::blue));
    ui->label->setPalette(pe);
    ui->label_2->setPalette(pe);
    ui->label_3->setPalette(pe);

    m_pProcesser = new CProcesser(m_curMicChannels,
                                  m_strPathInAudio.toStdString(),
                                  m_strPathOutAudio.toStdString(),
                                  m_OutPutAudioLogPath.toStdString(),
                                  UserConfig::GetInstance()->GetUiConfig()._inAudioBlankHeadTimeLength,
                                  UserConfig::GetInstance()->GetUiConfig()._inAudioBlankTailTimeLength
                                  );

    connect(m_pProcesser,&CProcesser::signal_finished_init,this,&MainWindow::Slot_processEndInit);
    connect(m_pProcesser,&CProcesser::signal_finished_cal,this,&MainWindow::Slot_processEndCal);
}

MainWindow::~MainWindow()
{
    if(m_NeedDelReg)
    {
        RegistryHelper::writeDWORDValue(ElevocPath,L"EnableLog_Up",0);
    }

    delete ui;
    if(m_pProcesser)
    {
        delete m_pProcesser;
        m_pProcesser = nullptr;
    }

    //delete m_pProcessAudioStart;
}


void MainWindow::on_start_clicked()
{
    if (RegistryHelper::valueExists(ElevocPath, L"EnableLog"))
    {
        m_AllLogEnable = RegistryHelper::readDWORDValue(ElevocPath, L"EnableLog") == 1 ? true : false;
        LOG_DEBUG("MainWindow","m_AllLogEnable =%d",m_AllLogEnable);
    }
    else
    {
        if (RegistryHelper::valueExists(ElevocPath, L"EnableLog_Up"))
        {
            m_UpLogEnable = RegistryHelper::readDWORDValue(ElevocPath, L"EnableLog_Up") == 1 ? true : false;
            LOG_DEBUG("MainWindow","m_UpLogEnable =%d",m_UpLogEnable);
        }
    }

    if(!m_AllLogEnable && !m_UpLogEnable)
    {
        RegistryHelper::writeDWORDValue(ElevocPath,L"EnableLog_Up",1);
        m_UpLogEnable = true;
        m_NeedDelReg = true;//恢复系统数据
    }

    ui->progressBar->setVisible(true);
    ui->label_7->setVisible(true);
    ui->label_7->setText(u8"正在处理中...");
    ui->start->setEnabled(false);
    m_cleanFuture = std::async(&MainWindow::CleanOldData,this);//清除旧数据（文件夹从创建）
    //启动audioservice
    StartAudioService();
}

void MainWindow::Slot_processEndCal(const double& inRes,const double&outRes)
{
    qDebug()<<"Slot_processEndCal";
    if(m_bCalMode)
    {
        qDebug()<<"Slot_processEndCal m_bCalMode = "<< m_bCalMode;
        ui->start->setEnabled(false);
        ui->pushButton_CalSNR->setEnabled(true);
    }
    else
    {
        ui->start->setEnabled(true);
        ui->pushButton_CalSNR->setEnabled(false);
    }
    if(m_ftEndCal.get())
    {
        m_CalRes._dInRes = inRes;
        m_CalRes._dOutRes = outRes;
        ui->label_3->setText(QString("%1 db").arg(m_CalRes._dInRes));
        ui->label_2->setText(QString("%1 db").arg(m_CalRes._dOutRes));
        ui->label->setText(QString("%1 db").arg(abs(m_CalRes._dOutRes - m_CalRes._dInRes)));
        ui->statusbar->showMessage(u8"计算成功");
    }
    else
    {
        ui->statusbar->showMessage(u8"计算失败");
        ui->label_3->setText(u8"NA");
        ui->label_2->setText(u8"NA");
        ui->label->setText(u8"NA");
        ui->statusbar->setStyleSheet("color:red");
    }

    ui->progressBar->setVisible(false);
    ui->label_7->setVisible(false);
}

void MainWindow::Slot_processEndInit(bool bSucessed)
{
    if(!bSucessed)
    {
        QMessageBox::warning(this,u8"失败",u8"请检查 pcm 文件是否存在活被占用");
        ui->progressBar->setVisible(false);
        ui->label_7->setVisible(false);
        ui->start->setEnabled(true);
        ui->statusbar->showMessage(u8"计算失败");
        ui->label_3->setText(u8"NA");
        ui->label_2->setText(u8"NA");
        ui->label->setText(u8"NA");
        ui->statusbar->setStyleSheet("color:red");
        return;
    }

    m_ftEndCal = std::async(&CProcesser::Start,m_pProcesser);
}

void MainWindow::Run()
{
    m_ftInitCal = std::async(&CProcesser::Init,m_pProcesser);
}

bool  MainWindow::CleanOldData()
{
    bool bDelRes = true;
    ui->label_3->setText(u8"正在统计...");
    ui->label_2->setText(u8"正在统计...");
    ui->label->setText(u8"正在统计...");
    ui->statusbar->showMessage(u8"正在统计...");
    WindowsServiceManage wsm0;
    unsigned long uRet0 = wsm0.Query_Server_Status("audiosrv");
    LOG_DEBUG("CleanOldData","Query_Server_Status  =%d",uRet0);
    qDebug()<<"StopAudioService uRet ="<<uRet0;
    if(4 == uRet0)//状态值4的时为运行状态
    {
        QProcess process;
        process.start("PowerShell.exe Stop-service -Name audiosrv -force");
        process.waitForFinished();

        WindowsServiceManage wsm;
        unsigned long uRet = wsm.Query_Server_Status("audiosrv");
        LOG_DEBUG("CleanOldData","Stop-service Query_Server_Status  =%d",uRet);
        qDebug()<<"StopAudioService 000 uRet ="<<uRet;
        if(4 == uRet)
        {
            LOG_ERROR("CleanOldData","Stop-service  audiosrv Faild Query_Status  =%d Expected status  = 1",uRet);
            //return false;//停止service失败
        }
    }

    QString strPath = QString("c:/ElevocTest/");
    isDirExist(strPath);
    QString strPathIn = "c:/ElevocTest/inputAudiofp32_voip.pcm";
    QString strPathOut = "c:/ElevocTest/outputAudiofp32_voip.pcm";
    if(isFileExist(m_strPathInAudio))
    {
        bDelRes = QFile::remove(m_strPathInAudio);
        LOG_DEBUG("CleanOldData","remove file  = %s  %s",m_strPathInAudio.toStdString().c_str(),bDelRes? "true":"false");
    }

    if(isFileExist(m_strPathOutAudio))
    {
        bDelRes = QFile::remove(m_strPathOutAudio);
        LOG_DEBUG("CleanOldData","remove file   =%s  %s",m_strPathOutAudio.toStdString().c_str(),bDelRes? "true":"false");
    }

    return bDelRes;
}

void MainWindow::StartAudioService()
{
    WindowsServiceManage wsm0;
    unsigned long uRet = wsm0.Query_Server_Status("audiosrv");
    qDebug()<<"StartAudioService uRet ="<<uRet;
    LOG_DEBUG("StartAudioService","Query_Server_Status  =%d",uRet);
    //if(1 == uRet)//状态值1的时为停止状态
    {
        std::thread th1([this]{
            if(!m_cleanFuture.get())
            {
                ui->progressBar->setVisible(false);
                ui->label_7->setVisible(false);
                ui->start->setEnabled(false);
                ui->statusbar->showMessage(u8"统计失败");
                QMessageBox::warning(this,u8"失败",u8"重启audio服务失败，请重启电脑！");
                return;
            }
            QProcess process;
            process.start("PowerShell.exe Start-service audiosrv");
            process.waitForFinished();

            WindowsServiceManage wsm;
            unsigned long uRet = wsm.Query_Server_Status("audiosrv");
            qDebug()<<"StartAudioService 000 uRet ="<<uRet;
            LOG_DEBUG("StartAudioService","000 Query_Server_Status  =%d",uRet);
             ui->progressBar->setVisible(false);
             ui->label_7->setVisible(false);
             ui->start->setEnabled(false);
             ui->pushButton_CalSNR->setEnabled(true);
        });
        th1.detach();
    }
}


void MainWindow::Slot_stateChanged(QProcess::ProcessState state)
{
    switch(state)
    {
        case QProcess::ProcessState::NotRunning:
        {
            qDebug()<<"QProcess::ProcessState::NotRunning";
            break;
        }
        case QProcess::ProcessState::Starting:
        {
            qDebug()<<"QProcess::ProcessState::Starting";
            break;
        }

        case QProcess::ProcessState::Running:
        {
            qDebug()<<"QProcess::ProcessState::Running";

            break;
        }
    }
}

bool MainWindow::GetMicChannelNum()
{
    HRESULT hr=NULL;
    CoInitialize(NULL);
    IMMDeviceEnumerator *deviceEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
                          __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    IMMDevice *defaultDevice = NULL;

    qDebug()<<"MixerWrap::MicphoneChs 1";
    if (hr != S_OK)
    {
        return false;
    }

    hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &defaultDevice);

    if (defaultDevice == NULL)
    {
        qDebug()<<"MixerWrap::defaultDevice";
    }

    deviceEnumerator->Release();
    deviceEnumerator = NULL;
    IAudioClient* pAudioClient = NULL;
    WAVEFORMATEX *ppDeviceFormat;
    hr = defaultDevice->Activate(__uuidof(IAudioClient),CLSCTX_ALL,NULL,(void**)&pAudioClient);
    pAudioClient->GetMixFormat(&ppDeviceFormat);
    pAudioClient->Release();
    defaultDevice->Release();
    m_curMicChannels = ppDeviceFormat->nChannels;
    //m_curMicChannels = 4;
    qDebug()<<"1111 uValue = "<<m_curMicChannels;
    LOG_DEBUG("MainWindow","GetMicChannelNum = %d",m_curMicChannels);
}

void MainWindow::on_pushButton_CalSNR_clicked()
{
    if(m_bCalMode)
    {
        if(!isFileExist(m_strPathInAudio)||!isFileExist(m_strPathOutAudio))
        {
            QString strTmp = "";
            if(!isFileExist(m_strPathInAudio))
            {
                strTmp += m_strPathInAudio;
                strTmp += " ";
            }
            if(!isFileExist(m_strPathOutAudio))
            {
                strTmp += m_strPathOutAudio;
                strTmp += " ";
            }

            QMessageBox::warning(this,u8"失败",QString(u8"计算文件%1不存在").arg(strTmp));
            return;
        }
    }
    else
    {
        if(!isFileExist(OUT_FILE_PATH)||!isFileExist(IN_FILE_PATH))
        {
            QString strTmp = "";
            if(!isFileExist(OUT_FILE_PATH))
            {
                strTmp += OUT_FILE_PATH;
                strTmp += " ";
            }
            if(!isFileExist(IN_FILE_PATH))
            {
                strTmp += IN_FILE_PATH;
                strTmp += " ";
            }

            QMessageBox::warning(this,u8"失败",QString(u8"计算文件%1不存在").arg(strTmp));
            return;
        }
    }

    qDebug()<<"on_pushButton_CalSNR_clicked";
    qDebug()<<"on_pushButton_CalSNR_clicked 000";
    ui->progressBar->setVisible(true);
    ui->label_7->setVisible(true);
    ui->label_7->setText(u8"正在处理...");
    ui->start->setEnabled(false);
    ui->pushButton_CalSNR->setEnabled(false);
    ui->statusbar->showMessage(u8"处理中");
    ui->statusbar->setStyleSheet("color:blue");
    ui->label_3->setText(u8"正在处理...");
    ui->label_2->setText(u8"正在处理...");
    ui->label->setText(u8"正在处理...");
    Run();
}

