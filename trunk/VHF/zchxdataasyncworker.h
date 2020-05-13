#ifndef ZCHXDATAASYNCWORKER_H
#define ZCHXDATAASYNCWORKER_H

#include <QObject>
#include <QThread>
#include <QTimer>

class zchxDataAsyncWorker : public QObject
{
    Q_OBJECT
public:
    explicit zchxDataAsyncWorker(QObject *parent = nullptr);
    ~zchxDataAsyncWorker();

signals:
    //获取分机数据
    void    signalSendExtensionData(const QByteArray& data, bool sts);
    void    signalQueryExtensionNumber();
    //服务器心跳检测
    void    signalPBXHeartWork();
    void    signalSendPBXHeartResult(bool sts, const QString& msg);

public slots:
    void    slotUpdateExtension();
    void    slotPBXHeartWork();
private:
    QThread mWorkThread;
    int     mHeartbeatErrorCount;

};

#endif // ZCHXDATAASYNCWORKER_H
