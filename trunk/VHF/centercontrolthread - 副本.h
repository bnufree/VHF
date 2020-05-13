#ifndef CENTERCONTROLTHREAD_H
#define CENTERCONTROLTHREAD_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
class QTimer;

class CenterControlThread : public QObject
{
    Q_OBJECT
public:
    explicit CenterControlThread(const QString& host, int port, int interval, QObject *parent = nullptr);
    virtual ~CenterControlThread();
    void     start();

signals:
    void    signalRecvCmd(int, bool);
public slots:
    void    slotRequest();
    void    slotReadData();
    void    slotSocketStateChange(QAbstractSocket::SocketState state);
private:
    QString mHost;
    int     mPort;
    QTcpSocket*  mSocket;
    int     mInterVal;
    QThread     mWorkThread;
    QTimer*     mTimer;
    bool        mIsConnected;
    int         mMaxOpendTimeSecs;
    qint64   mInitOpendTime;
    bool        mIsOpend;
    QString   mRecvContentList;

};

#endif // CENTERCONTROLTHREAD_H
