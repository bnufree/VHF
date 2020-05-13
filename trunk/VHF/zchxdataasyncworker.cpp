#include "zchxdataasyncworker.h"
#include "networker.h"
#include <QDateTime>
#include <QDebug>

zchxDataAsyncWorker::zchxDataAsyncWorker(QObject *parent) : QObject(parent)
{
    mHeartbeatErrorCount = 0;
    connect(this, SIGNAL(signalQueryExtensionNumber()), this, SLOT(slotUpdateExtension()));
    connect(NetWorker::instance(), SIGNAL(signal_heartbeat_timeup()), this, SLOT(slotPBXHeartWork()));
    connect(NetWorker::instance(), &NetWorker::SignalReconnected, this, &zchxDataAsyncWorker::slotUpdateExtension);
    connect(NetWorker::instance(), &NetWorker::SignalConfigChanged, this, &zchxDataAsyncWorker::slotUpdateExtension);
    this->moveToThread(&mWorkThread);
    mWorkThread.start();

    //服务器心跳开始
    NetWorker::instance()->startHeartTimer();
}

zchxDataAsyncWorker::~zchxDataAsyncWorker()
{
    mWorkThread.quit();
    mWorkThread.wait(500);
    mWorkThread.terminate();
}


void zchxDataAsyncWorker::slotPBXHeartWork()
{
    if(mHeartbeatErrorCount >= 2)
    {
        if(NetWorker::instance()->Relogin() == 0)
        {
            mHeartbeatErrorCount = 0;
        }
        return;
    }
    int error_code = NetWorker::instance()->send_heartbeat();
    QString curTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ");
    bool sts = true;
    QString message = "";
    if (error_code)
    {
        sts = false;
        message = QString("%1 %2:%3")
                .arg(curTimeStr)
                .arg(QStringLiteral("PBX服务器心跳失败，原因"))
                .arg(NetWorker::instance()->errorNo2String(error_code));
        mHeartbeatErrorCount++;
    }
    else
    {
        message = QString("%1 %2")
                .arg(curTimeStr)
                .arg(QStringLiteral("PBX服务器心跳成功"));
    }
    emit signalSendPBXHeartResult(sts, message);
}

void zchxDataAsyncWorker::slotUpdateExtension()
{
    QByteArray recv;
    int result = NetWorker::instance()->QueryExtensionList(recv);
    bool sts = true;
    if (0 != result || recv.isEmpty()) sts = false;
    qDebug()<<"update extension end, status:"<<sts;
    emit signalSendExtensionData(recv, sts);
}
