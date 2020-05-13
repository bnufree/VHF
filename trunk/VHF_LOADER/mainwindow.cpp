#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
//#include <windows.h>
//#include <tlhelp32.h>// for CreateToolhelp32Snapshot
//#include <psapi.h>   // for GetModuleFileNameEx
//#include <QTimer>
//#include <QProcess>
//#include <QDateTime>


MainWindow::MainWindow(const QString& exe, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
//    this->setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);
    this->setWindowFlag(Qt::SubWindow);
    mThread = new VHFMonitorThread(exe, this);
    mThread->start();
}

MainWindow::~MainWindow()
{
    if(mThread)
    {
        mThread->deleteLater();
    }
    delete ui;
}






