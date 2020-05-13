#include <QDebug>
#include <QSharedPointer>
#include <QSqlError>
#include <QDir>
#include "dbservices.h"
#include <QMap>
#include <QMutexLocker>

#define         DATE_STR_FORMAT         "yyyy-MM-dd"

DBServices* DBServices::m_pInstance = nullptr;
DBServices::CGarbo DBServices::s_Garbo;
QMutex DBServices::mutex;

DBServices::DBServices(QObject *parent) :
    QObject(parent)
{
    mTasklist.clear();
    initSignalSlot();
    //3、开启异步通讯
    moveToThread(&m_threadWork);
    m_threadWork.start();
}


DBServices::~DBServices()
{
    m_threadWork.quit();
    m_threadWork.wait(500);
    m_threadWork.terminate();
}

bool DBServices::isDBInitOk()
{
    return mDataBase.isDBOK();
}

//初始化数据库
void DBServices::slotInitDBTables()
{
    if(!isDBInitOk()) return ;
    qDebug()<<__FUNCTION__<<__LINE__;
    if(mDataBase.createDBTables())
    {
        emit signalDbInitFinished();
    } else
    {
        qDebug()<<"init DB Tables failed:"<<mDataBase.errMsg();
    }
    qDebug()<<__FUNCTION__<<__LINE__;
}


void DBServices::initSignalSlot()
{
    qRegisterMetaType<QList<BroadcastTask> >("const QList<BroadcastTask>&");
    qRegisterMetaType<TaskList >("const TaskList&");
    qRegisterMetaType<BroadcastSetting>("const BroadcastSetting&");
    qRegisterMetaType<BroadcastTask>("const BroadcastTask&");

    connect(this, &DBServices::signalInsertTaskList, this, &DBServices::slotInsertTaskList);
    connect(this, &DBServices::signalInitDBTables, this, &DBServices::slotInitDBTables);
    connect(this, &DBServices::signalQueryTaskList, this, &DBServices::slotQueryTaskList);
    connect(this, &DBServices::signalRemoveTask, this, &DBServices::slotRemoveTask);
}

DBServices *DBServices::instance()
{

    if(m_pInstance == nullptr)//第一次检测
    {
        QMutexLocker locker(&mutex);//加互斥锁。
        if(m_pInstance == nullptr)
        {
            m_pInstance = new DBServices();
        }
    }
    return m_pInstance;
}


void DBServices::slotInsertTaskList(const TaskList& list)
{
    if(!isDBInitOk()) return ;
    if(!mDataBase.updateBroadTaskList(list))
    {
        qDebug()<<"update task list error:"<<errMsg();
    }

    emit signalQueryTaskList();

}
void DBServices::slotRemoveTask(int id)
{
    if(!isDBInitOk()) return ;
    if(!mDataBase.delBroadcastTask(id))
    {
        qDebug()<<"update task list error:"<<errMsg();
    }

    emit signalQueryTaskList();
}
void DBServices::slotQueryTaskList(qint64 start, qint64 end, const QString station)
{
    if(!isDBInitOk()) return ;
    QMutexLocker locker(&mTaskMutex);
    mTasklist.clear();
    if(!mDataBase.queryBroadcastTaskList(mTasklist, start, end, station))
    {
        qDebug()<<"update task list error:"<<errMsg();
    }

    emit signalSendTaskList(mTasklist);
}

TaskList DBServices::getTaskList()
{
    QMutexLocker locker(&mTaskMutex);
    return mTasklist;
}

