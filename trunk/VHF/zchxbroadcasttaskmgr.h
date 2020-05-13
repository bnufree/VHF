#ifndef ZCHXBROADCASTTASKMGR_H
#define ZCHXBROADCASTTASKMGR_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include "vhfdatadefines.h"

class zchxBroadcastTaskMgr : public QObject
{
    Q_OBJECT
public:
    explicit zchxBroadcastTaskMgr(QObject *parent = nullptr);
    void     updateTask();
    ~zchxBroadcastTaskMgr();

    void removeTask(int id);

signals:
    void     signalSendBroadTaskList(const TaskList& now);
    void     signalAppendTask(const BroadcastSetting& setting);
    void     signalSendTotalBroadcastList(const TaskList& list);
public slots:
    void     slotRecvTaskSetting(const BroadcastSetting& setting);
    void     slotCheckTaskBroadcastNow();
    void     slotDBInitFinished();
private:
    QThread                     mWorkThread;
    TaskList                    mTaskList;
    QTimer                      *mTaskCheckTimer;
};

#endif // ZCHXBROADCASTTASKMGR_H
