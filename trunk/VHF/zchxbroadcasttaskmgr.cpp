#include "zchxbroadcasttaskmgr.h"
#include <QDateTime>
#include "inifile.h"
#include "dbservices.h"

#define         TASK_CHECK_INTERVAL         10*1000

zchxBroadcastTaskMgr::zchxBroadcastTaskMgr(QObject *parent) : QObject(parent)
{
    mTaskCheckTimer = new QTimer();
    mTaskCheckTimer->setInterval(TASK_CHECK_INTERVAL);
    connect(mTaskCheckTimer, &QTimer::timeout, this,  &zchxBroadcastTaskMgr::slotCheckTaskBroadcastNow);
    connect(this, &zchxBroadcastTaskMgr::signalAppendTask, this, &zchxBroadcastTaskMgr::slotRecvTaskSetting);
    connect(DATA_SERVICE, &DBServices::signalSendTaskList, this, &zchxBroadcastTaskMgr::signalSendTotalBroadcastList);
    connect(DATA_SERVICE, &DBServices::signalDbInitFinished, this, &zchxBroadcastTaskMgr::slotDBInitFinished);
    this->moveToThread(&mWorkThread);
    mWorkThread.start();
    mTaskCheckTimer->start();
    DATA_SERVICE->signalInitDBTables();
}

zchxBroadcastTaskMgr::~zchxBroadcastTaskMgr()
{
    if(mTaskCheckTimer) delete mTaskCheckTimer;
    mWorkThread.quit();
    mWorkThread.wait(500);
    mWorkThread.terminate();
}


void zchxBroadcastTaskMgr::slotDBInitFinished()
{
    updateTask();
}

void zchxBroadcastTaskMgr::updateTask()
{
    DATA_SERVICE->signalQueryTaskList();
}

void zchxBroadcastTaskMgr::removeTask(int id)
{
    emit DATA_SERVICE->signalRemoveTask(id);
}
void zchxBroadcastTaskMgr::slotCheckTaskBroadcastNow()
{
    qint64 cur = QDateTime::currentMSecsSinceEpoch();
    TaskList total = DATA_SERVICE->getTaskList();
    TaskList jobs;
    while (total.size() > 0) {
        if(total.first().mTime <= cur)
        {
            if(fabs(total.first().mTime - cur) <= 10 * TASK_CHECK_INTERVAL)
            {
                jobs.append(total.first());
            }
            removeTask(total.first().mDBID);
            total.removeFirst();
        } else {
            break;
        }
    }
    if(jobs.size() > 0)
    {
        emit signalSendBroadTaskList(jobs);
    }
}

void zchxBroadcastTaskMgr::slotRecvTaskSetting(const BroadcastSetting& setting)
{
    qDebug()<<"recv broad task:"<<setting.mBroadExtIdList<<setting.mContent<<setting.mFileName<<
              QDateTime::fromMSecsSinceEpoch(setting.mStartTime).toString("yyyyMMddhhmmss")<<setting.mTimeGap<<setting.mTimes<<
              (setting.mType == 0 ? "immediately" : "plan");
    //开始生成任务列表
    if(setting.mType == Task_Immediately)
    {
        BroadcastTask task;
        task.mTime = QDateTime::currentMSecsSinceEpoch();
        task.mContent = setting.mContent;
        task.mFileName = setting.mFileName;
        task.mBroadExtIdList = setting.mBroadExtIdList;
        qDebug()<<"send immediately task:"<<setting.mContent;
        emit signalSendBroadTaskList(QList<BroadcastTask>() << task);
        return;

    }
    TaskList list;
    if(setting.mType == Task_Planning)
    {
        //根据设定的开始时间和次数和间隔开始生成任务列表
        for(int i=0; i<setting.mTimes; i++)
        {
            BroadcastTask task;
            task.mTime = setting.mStartTime + i * setting.mTimeGap * 1000;
            task.mContent = setting.mContent;
            task.mFileName = setting.mFileName;
            task.mBroadExtIdList = setting.mBroadExtIdList;
            list.append(task);
        }
        emit DATA_SERVICE->signalInsertTaskList(list);
    }
}
