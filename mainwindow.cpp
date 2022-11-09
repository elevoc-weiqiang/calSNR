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
    this->setWindowTitle("Elevoc SNR Tools V0.0.1");
    ui->statusbar->setSizeGripEnabled(false);
    ui->statusbar->showMessage(u8"已就绪");
    ui->statusbar->setStyleSheet("color:green");
    ui->progressBar->setVisible(false);
    ui->label_7->setVisible(false);
    //m_pProcessAudioStop = new QProcess(this);
    //m_pProcessAudioStart = new QProcess(this);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);
    ui->pushButton_CalSNR->setEnabled(false);
    QPalette pe;
    pe.setColor(QPalette::WindowText, QColor(Qt::GlobalColor::blue));
    ui->label->setPalette(pe);
    ui->label_2->setPalette(pe);
    ui->label_3->setPalette(pe);
    GetMicChannelNum();
    m_pProcesser = new CProcesser(m_curMicChannels,IN_FILE_PATH,OUT_FILE_PATH);
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
    delete m_pProcesser;
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
        m_NeedDelReg = true;
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
    m_ready = true;
    ui->start->setEnabled(true);
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
        LOG_DEBUG("CleanOldData","444 Query_Server_Status  =%d",uRet);
        qDebug()<<"StopAudioService 000 uRet ="<<uRet;
        if(4 == uRet)
        {
            LOG_DEBUG("CleanOldData","Stop-service audiosrv After Query_Server_Status  =%d",uRet);
            //return false;//停止service失败
        }
    }

    QString strPath = QString("c:/ElevocTest/");
    isDirExist(strPath);
    QString strPathIn = "c:/ElevocTest/inputAudiofp32_voip.pcm";
    QString strPathOut = "c:/ElevocTest/outputAudiofp32_voip.pcm";
    if(isFileExist(strPathIn))
    {
        bDelRes = QFile::remove(strPathIn);
        LOG_DEBUG("CleanOldData","remove file   =%s  %s",strPathIn.toStdString().c_str(),bDelRes? "true":"false");
    }

    if(isFileExist(strPathOut))
    {
        bDelRes = QFile::remove(strPathOut);
        LOG_DEBUG("CleanOldData","remove file   =%s  %s",strPathOut.toStdString().c_str(),bDelRes? "true":"false");
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
    if(!isFileExist(OUT_FILE_PATH)||!isFileExist(IN_FILE_PATH))
    {
        QMessageBox::warning(this,u8"失败",u8"计算文件不存在");
        return;
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

