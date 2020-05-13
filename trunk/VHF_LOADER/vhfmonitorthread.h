#ifndef VHFMONITORTHREAD_H
#define VHFMONITORTHREAD_H

#include <QThread>
#include <QProcess>

class VHFMonitorThread : public QThread
{
    Q_OBJECT
public:
    explicit VHFMonitorThread(const QString& app, QObject *parent = 0);
    void run();
    ~VHFMonitorThread();
    void runApp();

signals:

public slots:
private:
    QString mAppName;
    qint64  mAppID;
    QString mDogFile;
    QProcess*  mSub;
};

#endif // VHFMONITORTHREAD_H
