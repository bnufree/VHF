#include "watchdogthread.h"
#include <QApplication>
#include <QDir>

WatchDogThread::WatchDogThread(QObject *parent) : QThread(parent)
{

}

void WatchDogThread::run()
{
    QDir dir(QApplication::applicationDirPath() + QString("/watchdog"));
    if(!dir.exists())
    {
        dir.mkpath(dir.path());
    }
    QString fileName = QString("%1/watchdog.txt").arg(dir.path());
    QFile file(fileName);
    while (true) {
        if(file.open(QIODevice::WriteOnly))
        {
            file.write("ddddd");
            file.close();
        }
        sleep(10);
    }
}
