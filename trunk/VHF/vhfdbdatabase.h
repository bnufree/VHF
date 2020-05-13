#ifndef VHFDBDATABASE_H
#define VHFDBDATABASE_H

#include <QObject>
#include <QtSql>
#include <QMutex>
#include "vhfdatadefines.h"


enum    HQ_DATA_TYPE{
    HQ_DATA_TEXT = 1,
    HQ_DATA_INT,
    HQ_DATA_DOUBLE,
    HQ_DATA_LONG,
};

enum    HQ_DATA_COMPARE{
    HQ_COMPARE_EQUAL = 0,
    HQ_COMPARE_LESS,
    HQ_COMPARE_GREAT,
    HQ_COMPARE_STRLIKE,
    HQ_COMPARE_TEXTIN,
};

struct HQ_COL_VAL{
    QVariant mValue;
    int mType;

    HQ_COL_VAL()
    {
        mType = HQ_DATA_INT;
    }

    HQ_COL_VAL(const QVariant& val, HQ_DATA_TYPE t)
    {
        mType = t;
        mValue = val;
    }
};


Q_DECLARE_METATYPE(HQ_COL_VAL)

typedef class HQ_QUERY_CONDITION{
public:
    QString  mColName;
    HQ_COL_VAL  mColVal;
    HQ_DATA_COMPARE mColCompare;

    HQ_QUERY_CONDITION(const QString colStr, const QVariant& val, HQ_DATA_TYPE t, HQ_DATA_COMPARE compare = HQ_COMPARE_EQUAL)
    {
        mColName = colStr;
        mColVal.mType = t;
        mColVal.mValue = val;
        mColCompare = compare;
    }

    HQ_QUERY_CONDITION(){}
}DBColVal;

class DBPlacementList:public QStringList
{
public:
    DBPlacementList():QStringList(){}
    DBPlacementList(int num){
        for(int i=0; i<num; i++)
        {
            append("?");
        }
    }
};

class DBColValList:public QList<HQ_QUERY_CONDITION>
{
public:
    DBColValList():QList<HQ_QUERY_CONDITION>() {}
    DBColValList(const HQ_QUERY_CONDITION& val) : QList<HQ_QUERY_CONDITION>() {append(val);}
    DBColValList(const QList<HQ_QUERY_CONDITION>& list) : QList<HQ_QUERY_CONDITION>(list) {}
    QString insertString() const;
    QString updateString() const;
    QString whereString() const;
};



class DBDataBase : public QObject
{
    Q_OBJECT
public:
    explicit DBDataBase(QObject *parent = 0);
    ~DBDataBase();
    QString getErrorString();
    bool isDBOK();
    bool createDBTables();
    QString errMsg();

    //基本数据更新
    bool updateBroadTaskList(const TaskList& dataList);
    bool queryBroadcastTaskList(TaskList& list, qint64 start, qint64 end, const QString& station);
    bool delBroadcastTask(int id);

private:
    bool initSqlDB();
    //创建数据库需要的各类表

    //任务列表
    bool createBroadcastTaskTable();
    bool isTableExist(const QString &pTable);
    bool createTable(const QString& pTable, const TableColList& cols);

    //表的通用操作
    bool updateTable(const QString& tableName, const DBColValList& values, const DBColValList& key, bool check = true );
    bool deleteRecord(const QString& table, const DBColValList& list = DBColValList());    
    bool isRecordExist(bool& exist, const QString& table, const DBColValList& list);

signals:

public slots:
private:
    QSqlDatabase        mDB;
    QSqlQuery           mSQLQuery;
    QMutex              mSQLMutex;
    bool                mInitDBFlg;
};

#endif // VHFDBDATABASE_H
