#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include "processer.h"
#include <thread>
#include "Log/LogHelper.h"
#include "RegistryHelper.h"
#include<memory>
#include <future>
#include<condition_variable>
#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#endif

#include "UserConfig.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

    bool CleanOldData();

    void StartAudioService();

    bool GetMicChannelNum();
signals:
    void Signal_CleanDataUI(int);

private slots:
    void on_start_clicked();

    void Slot_stateChanged(QProcess::ProcessState state);

    void Slot_processEndCal(const double& inRes,const double&outRes);

    void Slot_processEndInit(bool bSucessed);

    void on_pushButton_CalSNR_clicked();

    void Slot_CleanDataUI(int);
private:
    bool m_ready = true;
    Ui::MainWindow *ui;
    std::thread m_thread;
    void Run();
    bool m_AllLogEnable = false;
    bool m_UpLogEnable = false;
    bool m_NeedDelReg = false;
    //QProcess* m_pProcessAudioStop;
    //QProcess* m_pProcessAudioStart;
    int m_curMicChannels = 2;;
    QString m_inPcmPath;
    QString m_outPcmPath;
    std::future<bool> m_cleanFuture;
    //std::condition_variable m_conditionVar;
    std::future<bool> m_ftEndCal;
    std::future<bool> m_ftInitCal;
    ProcessRes m_CalRes;
    CProcesser *m_pProcesser;

    QString m_strPathInAudio = "c:/ElevocTest/inputAudiofp32_voip.pcm";
    QString m_strPathOutAudio = "c:/ElevocTest/outputAudiofp32_voip.pcm";
    bool m_bCalMode = false;//计算模式，仿真测试
    QString m_OutPutAudioLogPath = "C:/EleDector/CalSnrRes/data";
};
#endif // MAINWINDOW_H
