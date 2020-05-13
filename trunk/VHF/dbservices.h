#ifndef DBSERVICE_H
#define DBSERVICE_H

#include <QObject>
#include <QThread>
#include "vhfdbdatabase.h"


#define DATA_SERVICE DBServices::instance()

class DBServices : public QObject
{
    Q_OBJECT
protected:
    DBServices(QObject *parent = 0);
    ~DBServices();

public:
    bool   isDBInitOk();
    friend class CGarbo;
    static DBServices* instance();
    TaskList getTaskList();

signals:
    //开始创建数据库需要的表信息，初始化数据库
    void signalInitDBTables();
    void signalDbInitFinished();
    //任务列表更新
    void signalInsertTaskList(const TaskList& list);
    void signalRemoveTask(int id);
    void signalQueryTaskList(qint64 start = 0, qint64 end = 0, const QString station = "");
    void signalSendTaskList(const TaskList& list);

public slots:
    void slotInitDBTables();
    void slotInsertTaskList(const TaskList& list);
    void slotRemoveTask(int id);
    void slotQueryTaskList(qint64 start, qint64 end, const QString station);

private:
    void initSignalSlot();
    QString errMsg()   {return mDataBase.errMsg();}

private:    //本类使用的变量
    static DBServices *m_pInstance;
    static QMutex mutex;//实例互斥锁。
    //static QAtomicPointer<DBServices> m_pInstance;/*!<使用原子指针,默认初始化为0。*/

    class CGarbo        // 它的唯一工作就是在析构函数中删除CSingleton的实例
    {
    public:
        ~CGarbo()
        {
            if (DBServices::m_pInstance)
            {
                delete DBServices::m_pInstance;
                DBServices::m_pInstance = nullptr;
            }
        }
    };
    static CGarbo               s_Garbo; // 定义一个静态成员，在程序结束时，系统会调用它的析构函数
    QThread                     m_threadWork;       //工作线程
    DBDataBase                  mDataBase;
    TaskList                    mTasklist;
    QMutex                      mTaskMutex;
};

#endif // DBSERVICE_H
