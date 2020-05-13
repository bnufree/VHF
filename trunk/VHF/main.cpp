#include "logindialog.h"
#include "controlwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QFile>
#include <QMutex>
#include <iostream>
#include <QTextCodec>
#include <QDir>
#include <QSharedMemory>

//#define DateTime
const QString DateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");

void logMessageOutputQt5(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QString log_file_name = "";
    static qint64 max = 10485760;//10 * 1024 * 1024;
    static QMutex mutex;
    static qint64 log_file_lenth = 0;
    mutex.lock();
    QString text;
    switch (type) {
    case QtDebugMsg:
        text = QString("Debug:");
        break;
    case QtWarningMsg:
        text = QString("Warning:");
        break;
    case QtCriticalMsg:
        text = QString("Critical:");
        break;
    case QtFatalMsg:
        text = QString("Fatal:");
        abort();
    default:
        break;
    }
    QString message = QString("[%1] %2 [%3] [%4] %5").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(text).arg(context.file).arg(context.line).arg(msg);

    QDir dir(QCoreApplication::applicationDirPath() + QString("/log"));
    if(!dir.exists())
    {
        dir.mkpath(dir.path());
    }
    if(log_file_name.isEmpty() || log_file_lenth > max)
    {
        //重新启动的情况,将日志目录下的文件删除,保留最近的文件
        {
            QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
            foreach(QFileInfo info, list)
            {
                QFile::remove(info.absoluteFilePath());
            }
        }
        log_file_lenth = 0;
        log_file_name = QCoreApplication::applicationDirPath() + QString("/log/LOG_%1.txt").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    }
    QFile file(log_file_name);
    if(file.open(QIODevice::ReadWrite | QIODevice::Append))
    {
        QTextStream text_stream(&file);
        text_stream << message << endl;
        file.flush();
        file.close();
        log_file_lenth = file.size();
    } else
    {
        std::cout <<"open file failed...........";
    }
    mutex.unlock();
    message = QString("[%1] %2 [%3] [%4] ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(text).arg(context.file).arg(context.line);
    std::cout << "\033[31m" << message.toStdString();
    std::cout << "\033[32m" << msg.toUtf8().toStdString() <<std::endl;
}

const QString myVersion = "202005060000";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	//程序是否正在运行的检查
    qDebug()<<QApplication::applicationFilePath();


    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QSharedMemory mem(QApplication::applicationFilePath());
    if (!mem.create(1))
    {
        QMessageBox msgBox(QMessageBox::Warning,QStringLiteral("提示"),QStringLiteral("您已开启一个VHF客户端程序，该程序不能同时开启多个。\n"));
        msgBox.addButton(QStringLiteral("确定"), QMessageBox::AcceptRole);
        msgBox.exec();
        a.exit(1);
        return 0;
    }
    QTranslator translator;
    QString path = QCoreApplication::applicationDirPath() + "/../Resource/zh_cn.qm";
    qDebug()<<"path:"<<path;
    translator.load(path);
//    translator.load(QString("%1/Resource/zh_cn.qm").arg(path));
    a.installTranslator(&translator);
//    if (argc > 1 && argv[1] == QString("-log"))
//    {
        //execTime = QDateTime::currentDateTime();
        qInstallMessageHandler(logMessageOutputQt5);
//    }
    qDebug()<<"app version:"<<myVersion;
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted)
    {
        ControlWindow controlWindow;
        controlWindow.show();
        return a.exec();
    }
    else return 0;
}
