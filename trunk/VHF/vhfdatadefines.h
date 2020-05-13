#ifndef VHFDATADEFINES
#define VHFDATADEFINES

#include <QString>
#include <QStringList>
#include <QDate>
#include <QObject>

//表名
#define         TABLE_BROADCAST_TASK                "broadcast_task"
//列名
#define         HQ_TABLE_COL_ID                     "id"
#define         HQ_TABLE_COL_TIME                   "time"
#define         HQ_TABLE_COL_STATION                "station"
#define         HQ_TABLE_COL_CONTENT                "content"
#define         HQ_TABLE_COL_AUDIO                   "audio"
#define         HQ_TABLE_COL_TABLE_NM               "name"



struct TABLE_COL_DEF
{
    QString mKey;
    QString mDef;
};

typedef QList<TABLE_COL_DEF>        TableColList;

//播放任务结构体
typedef  struct BroadcastTask_t{
    int             mDBID;                      //数据库ID
    QStringList     mBroadExtIdList;            //播放岸站
    qint64          mTime;                      //播放时间
    QString         mFileName;                  //播放的文件名  服务器返回的文件名
    QString         mContent;                   //播放的内容

    bool static sort(const BroadcastTask_t & p1, const BroadcastTask_t & p2)
    {
        return p1.mTime <= p2.mTime;
    }
}BroadcastTask;

Q_DECLARE_METATYPE(BroadcastTask)

typedef  QList<BroadcastTask>           TaskList;

enum BroadcastType{
    Task_Immediately = 0,
    Task_Planning,
};

//播放任务参数
struct BroadcastSetting{
    BroadcastType   mType;
    qint64          mStartTime;
    int             mTimes;
    int             mTimeGap;     //秒
    QString         mFileName;                  //播放的文件名  服务器返回的文件名
    QString         mLocalAudioName;            //生成的本地的文件名
    QString         mContent;                   //播放的内容
    QStringList     mBroadExtIdList;            //播放岸站
    bool            mIsTry;                     //是否是试听
    int             mVol;                       //合成语音的声音音量

    BroadcastSetting()
    {
        mIsTry = false;
    }
};

Q_DECLARE_METATYPE(BroadcastSetting)

#endif // VHFDATADEFINES

