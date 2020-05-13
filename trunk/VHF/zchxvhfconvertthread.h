#ifndef ZCHXVHFCONVERTTHREAD_H
#define ZCHXVHFCONVERTTHREAD_H

#include <QObject>
#include "vhfdatadefines.h"
#include <QThread>

class QUploadFileThread;
class zchxBroadcastTaskMgr;

class zchxVhfConvertThread : public QObject
{
    Q_OBJECT
public:
    explicit zchxVhfConvertThread(QObject *parent = 0);

signals:
    void    signalRecvBroadSetting(const BroadcastSetting& setting);
    void    signalSendBroadTaskList(const TaskList& now);
    void    signalSendTotalBroadcastList(const TaskList& list);
    void    signalSendErrorMsg(const QString& msg);
    void    signalRemoveBroadTask(int id);
public slots:
    void slotRecvBroadSetting(const BroadcastSetting& setting);
    void slotAudioTaskfinished();
    void slotRecvUploadFileSuccess(const BroadcastSetting& setting);
    void slotRemoveBroadTask(int id);
private:
    QThread     mWorkThread;
    QUploadFileThread* mUploadThread;
    zchxBroadcastTaskMgr*               mTaskMgr;
};

#endif // ZCHXVHFCONVERTTHREAD_H
