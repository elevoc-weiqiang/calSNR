#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusbar->setSizeGripEnabled(false);
    ui->statusbar->showMessage(u8"已就绪");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_start_clicked()
{
    if(!m_ready)
    {
        return;
    }
    m_ready = false;
    ui->start->setEnabled(false);
    ui->statusbar->showMessage(u8"处理中");
    if(m_thread.joinable())
    {
        m_thread.join();
    }
    m_thread = std::thread(&MainWindow::Run,this);
    //Run();
}

void MainWindow::Run()
{
    CProcesser m_processer;
    if(!m_processer.Init())
    {
        QMessageBox::warning(this,u8"失败",u8"请检查 pcm 文件是否存在活被占用");
        return;
    }

    m_processer.Start(); 

    m_ready = true;
    ui->start->setEnabled(true);
    ui->statusbar->showMessage(u8"完成");
}

