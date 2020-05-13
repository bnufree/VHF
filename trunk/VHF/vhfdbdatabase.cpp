#include "vhfdbdatabase.h"
#include <QMutexLocker>
#include <QDebug>
#include <QFile>
#include <QApplication>

#ifdef Q_OS_WIN
#define         APP_DIR                  QApplication::applicationDirPath()
#else
#define         APP_DIR                  QString("/sdcard/vhf")
#endif

#define     QDebug()                    qDebug()<<__FUNCTION__<<__LINE__
#define     DB_FILE                     QString("%1/db.data").arg(APP_DIR)
#define     SQL_WHERE                   " where "

QString DBColValList::insertString() const
{
    if(size() == 0) return QString();
    QStringList colList;
    QVariantList valList;
    for(int i=0; i<size(); i++)
    {
        HQ_QUERY_CONDITION data = at(i);
        colList.append(data.mColName);
        valList.append(QVariant::fromValue(data.mColVal));
    }
    QStringList valStrlist;
    for(int i=0; i<valList.size(); i++)
    {
        HQ_COL_VAL val = valList[i].value<HQ_COL_VAL>();
        if(val.mType == HQ_DATA_TEXT)
        {
            valStrlist.append(QString("'%1'").arg(val.mValue.toString()));
        } else if(val.mType == HQ_DATA_INT)
        {
            valStrlist.append(QString("%1").arg(val.mValue.toInt()));
        } else if(val.mType == HQ_DATA_LONG)
        {
            valStrlist.append(QString("%1").arg(val.mValue.toLongLong()));
        }
        else
        {
            valStrlist.append(QString("%1").arg(val.mValue.toDouble(), 0, 'f', 3));
        }
    }
    return QString("(%1) values (%2)").arg(colList.join(",")).arg(valStrlist.join(","));
}

QString DBColValList::updateString() const
{
    if(size() == 0) return QString();
    QStringList valStrlist;
    for(int i=0; i<size(); i++)
    {
        HQ_QUERY_CONDITION data = at(i);
        if(data.mColVal.mType == HQ_DATA_TEXT)
        {
            valStrlist.append(QString("%1 = '%2'").arg(data.mColName).arg(data.mColVal.mValue.toString()));
        } else if(data.mColVal.mType == HQ_DATA_INT)
        {
            valStrlist.append(QString("%1 = %2").arg(data.mColName).arg(data.mColVal.mValue.toInt()));
        } else if(data.mColVal.mType == HQ_DATA_LONG)
        {
            valStrlist.append(QString("%1 = %2").arg(data.mColName).arg(data.mColVal.mValue.toLongLong()));
        }
        else
        {
            valStrlist.append(QString("%1 = %2").arg(data.mColName).arg(data.mColVal.mValue.toDouble(), 0, 'f', 3));
        }
    }
    return valStrlist.join(",");
}

QString DBColValList::whereString() const
{
    if(size() == 0) return QString();
    QStringList valStrlist;
    for(int i=0; i<size(); i++)
    {
        HQ_QUERY_CONDITION data = at(i);
        if(data.mColVal.mType == HQ_DATA_TEXT)
        {
            if(data.mColCompare == HQ_COMPARE_EQUAL)
            {
                valStrlist.append(QString("%1 = '%2'").arg(data.mColName).arg(data.mColVal.mValue.toString()));
            } else if(data.mColCompare == HQ_COMPARE_STRLIKE)
            {
                valStrlist.append(QString("%1 like '%2'").arg(data.mColName).arg(data.mColVal.mValue.toString()));
            } else if(data.mColCompare == HQ_COMPARE_TEXTIN)
            {
                QStringList list = data.mColVal.mValue.toStringList();
                if(list.size() > 0)
                {
                    QStringList resList;
                    foreach (QString str, list) {
                        resList.append(QString("'%1'").arg(str));
                    }
                    valStrlist.append(QString("%1 in () ").arg(data.mColName).arg(resList.join(",")));
                }
            } else if(data.mColCompare == HQ_COMPARE_GREAT)
            {
                valStrlist.append(QString("%1 > '%2'").arg(data.mColName).arg(data.mColVal.mValue.toString()));
            } else
            {
                valStrlist.append(QString("%1 < '%2'").arg(data.mColName).arg(data.mColVal.mValue.toString()));
            }
        } else if(data.mColVal.mType == HQ_DATA_INT)
        {
            if(data.mColCompare == HQ_COMPARE_EQUAL)
            {
                valStrlist.append(QString("%1 = %2").arg(data.mColName).arg(data.mColVal.mValue.toInt()));
            } else if(data.mColCompare == HQ_COMPARE_GREAT)
            {
                valStrlist.append(QString("%1 > %2").arg(data.mColName).arg(data.mColVal.mValue.toInt()));
            } else
            {
                valStrlist.append(QString("%1 < %2").arg(data.mColName).arg(data.mColVal.mValue.toInt()));
            }
        } else if(data.mColVal.mType == HQ_DATA_LONG)
        {
            if(data.mColCompare == HQ_COMPARE_EQUAL)
            {
                valStrlist.append(QString("%1 = %2").arg(data.mColName).arg(data.mColVal.mValue.toLongLong()));
            } else if(data.mColCompare == HQ_COMPARE_GREAT)
            {
                valStrlist.append(QString("%1 > %2").arg(data.mColName).arg(data.mColVal.mValue.toLongLong()));
            } else
            {
                valStrlist.append(QString("%1 < %2").arg(data.mColName).arg(data.mColVal.mValue.toLongLong()));
            }
        }
        else
        {
            if(data.mColCompare == HQ_COMPARE_EQUAL)
            {
                valStrlist.append(QString("%1 = %2").arg(data.mColName).arg(data.mColVal.mValue.toDouble(), 0, 'f', 3));
            } else if(data.mColCompare == HQ_COMPARE_GREAT)
            {
                valStrlist.append(QString("%1 > %2").arg(data.mColName).arg(data.mColVal.mValue.toDouble(), 0, 'f', 3));
            } else if(data.mColCompare == HQ_COMPARE_LESS)
            {
                valStrlist.append(QString("%1 < %2").arg(data.mColName).arg(data.mColVal.mValue.toDouble(), 0, 'f', 3));
            }
        }
    }
    return  QString(" where %1").arg(valStrlist.join(" and "));
}

DBDataBase::DBDataBase(QObject *parent) : QObject(parent)
{
    mInitDBFlg = initSqlDB();
    if(mInitDBFlg)
    {
        mSQLQuery = QSqlQuery(mDB);
    }
    qDebug()<<"db init:"<<mInitDBFlg<<QFile::exists(mDB.databaseName());
}

DBDataBase::~DBDataBase()
{
    if(mInitDBFlg)
    {
        mDB.close();
    }
}

bool DBDataBase::isDBOK()
{
    return mInitDBFlg;
}

bool DBDataBase::initSqlDB()
{
    //初始化本地数据库的连接
    qDebug()<<"database:"<<QSqlDatabase::database().databaseName();
    qDebug()<<QSqlDatabase::drivers()<<QSqlDatabase::connectionNames();
    if(QSqlDatabase::contains("qt_sql_default_connection"))
    {
        qDebug()<<"add database qt_sql_default_connection";
        mDB = QSqlDatabase::database("qt_sql_default_connection");
    }
    else
    {
        if(QSqlDatabase::isDriverAvailable("QSQLITE"))
        {
            qDebug()<<"add database sqlite";
#if 1
            mDB = QSqlDatabase::addDatabase("QSQLITE");
#else
            mDB = QSqlDatabase::addDatabase("QPSQL");
#endif
            qDebug()<<"dbVaild:"<<mDB.isValid();
        } else
        {
            qDebug()<<"sqlite not found";
            return false;
        }
    }
    if(mDB.isValid())
    {
        mDB.setDatabaseName(DB_FILE);
        return mDB.open();
    } else
    {
        return false;
    }
}

QString DBDataBase::getErrorString()
{
    QMutexLocker locker(&mSQLMutex);
    return QString("sql: \\n %1 \\n error:%2").arg(mSQLQuery.lastQuery()).arg(mSQLQuery.lastError().text());
}

bool DBDataBase::isTableExist(const QString &pTable)
{
    QMutexLocker locker(&mSQLMutex);
    if(!mSQLQuery.exec(tr("SELECT COUNT(*) FROM sqlite_master where type='table' and name='%1'").arg(pTable))) return false;
    while (mSQLQuery.next()) {
        return mSQLQuery.value(0).toBool();
    }
    return false;
}

bool DBDataBase::createTable(const QString &pTable, const TableColList& cols)
{
    QStringList colist;
    foreach (TABLE_COL_DEF data, cols) {
        colist.append(tr(" [%1] %2 ").arg(data.mKey).arg(data.mDef));
    }

    QString sql = tr("CREATE TABLE [%1] ( %2 )").arg(pTable).arg(colist.join(","));
    qDebug()<<"sql:"<<sql;
    QMutexLocker locker(&mSQLMutex);
    return mSQLQuery.exec(sql);
}

bool DBDataBase::createBroadcastTaskTable()
{
    if(isTableExist(TABLE_BROADCAST_TASK)) return true;
    TableColList colist;
    colist.append({HQ_TABLE_COL_ID, "INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL"});
    colist.append({HQ_TABLE_COL_TIME, "TIMESTAMP NOT NULL"});
    colist.append({HQ_TABLE_COL_STATION, "VARCHAR(10000) NULL"});
    colist.append({HQ_TABLE_COL_CONTENT, "VARCHAR(10000) NULL"});
    colist.append({HQ_TABLE_COL_AUDIO, "VARCHAR(100) NULL"});
    return createTable(TABLE_BROADCAST_TASK, colist);
}

bool DBDataBase::updateBroadTaskList(const TaskList& dataList)
{
    int size = dataList.size();
    if(size == 0) return false;
    mDB.transaction();
    for(int i=0; i<size; i++)
    {
        BroadcastTask data = dataList[i];
        DBColValList list;
        list.append(DBColVal(HQ_TABLE_COL_STATION, data.mBroadExtIdList.join(","), HQ_DATA_TEXT));
        list.append(DBColVal(HQ_TABLE_COL_CONTENT, data.mContent, HQ_DATA_TEXT));
        list.append(DBColVal(HQ_TABLE_COL_AUDIO, data.mFileName, HQ_DATA_TEXT));
        list.append(DBColVal(HQ_TABLE_COL_TIME, data.mTime, HQ_DATA_LONG));
        if(!updateTable(TABLE_BROADCAST_TASK, list, list[0], false)){
            mDB.rollback();
            return false;
        }
    }
    mDB.commit();
    return true;
}

bool DBDataBase::queryBroadcastTaskList(TaskList& list, qint64 start, qint64 end, const QString& station)
{
    DBColValList where;
    if(start > 0)
    {
        where.append(DBColVal(HQ_TABLE_COL_TIME, start, HQ_DATA_LONG, HQ_COMPARE_GREAT));
    }
    if(end > 0)
    {
        where.append(DBColVal(HQ_TABLE_COL_TIME, end, HQ_DATA_LONG, HQ_COMPARE_LESS));
    }

    if(station.size() > 0)
    {
        where.append(DBColVal(HQ_TABLE_COL_STATION, station, HQ_DATA_TEXT, HQ_COMPARE_TEXTIN));
    }

    QMutexLocker locker(&mSQLMutex);
    if(!mSQLQuery.exec(tr("select * from %1 %2 order by %4 asc").arg(TABLE_BROADCAST_TASK).arg(where.whereString()).arg(HQ_TABLE_COL_TIME))) return false;
    while (mSQLQuery.next()) {
        BroadcastTask task;
        task.mDBID = mSQLQuery.value(HQ_TABLE_COL_ID).toInt();
        task.mTime = mSQLQuery.value(HQ_TABLE_COL_TIME).toLongLong();
        task.mContent = mSQLQuery.value(HQ_TABLE_COL_CONTENT).toString();
        task.mFileName = mSQLQuery.value(HQ_TABLE_COL_AUDIO).toString();
        task.mBroadExtIdList = mSQLQuery.value(HQ_TABLE_COL_STATION).toStringList();
        list.append(task);
    }
    return true;
}

bool DBDataBase::delBroadcastTask(int id)
{
    DBColValList list;
    if(id > 0)
    {
        list.append(DBColVal(HQ_TABLE_COL_ID, id, HQ_DATA_INT));
    }
    return deleteRecord(TABLE_BROADCAST_TASK, list);
}

bool DBDataBase::createDBTables()
{
    qDebug()<<__FUNCTION__<<__LINE__;
    if(!createBroadcastTaskTable()) return false;
    return true;
}


bool DBDataBase::updateTable(const QString& tableName, const DBColValList& values, const DBColValList& key, bool check )
{
    //检查记录已经添加
    bool exist = false;
    if( check && (!isRecordExist(exist, tableName, key))) return false;
    QMutexLocker locker(&mSQLMutex);
    if(exist){
        //更新
        return mSQLQuery.exec(QString("update %1 set %2 %3").arg(tableName).arg(values.updateString()).arg(key.whereString()));
    } else {
        //添加
        return mSQLQuery.exec(QString("insert into %1 %2").arg(tableName).arg(values.insertString()));
    }
    return true;
}

bool DBDataBase::deleteRecord(const QString &table, const DBColValList &list)
{
    QMutexLocker locker(&mSQLMutex);
    QString sql = QString("delete from %1 %2").arg(table).arg(list.whereString());
    qDebug()<<__FUNCTION__<<sql;
    if(!mSQLQuery.exec(sql)) return false;
    return true;
}

bool DBDataBase::isRecordExist(bool& exist, const QString& table, const DBColValList& list)
{
    exist = false;
    if(list.size() == 0) return true;
    QMutexLocker locker(&mSQLMutex);
    if(!mSQLQuery.exec(QString("select count(1) from %1 %2 ").arg(table).arg(list.whereString()))) return false;
    while (mSQLQuery.next()) {
        exist = mSQLQuery.value(0).toBool();
        break;
    }
    return true;
}


QString DBDataBase::errMsg()
{
    return QString("sql:%1\\nerr:%2").arg(mSQLQuery.lastQuery()).arg(mSQLQuery.lastError().text());
}



