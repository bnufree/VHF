#ifndef CENTERCONTROLTHREAD_H
#define CENTERCONTROLTHREAD_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>

class CenterControlThread : public QThread
{
    Q_OBJECT
public:
    explicit CenterControlThread(const QString& host, int port, int interval, QObject *parent = nullptr);
    virtual ~CenterControlThread();
    void     cancel() {mCancel = true;}

protected:
    void run();
signals:
    void    signalRecvCmd(int, bool);
public slots:
private:
    QString mHost;
    int     mPort;
    int     mInterVal;
    int         mMaxOpendTimeSecs;
    qint64   mInitOpendTime;
    bool        mIsOpend;
    QString   mRecvContentList;
    bool      mCancel;

};

#endif // CENTERCONTROLTHREAD_H
