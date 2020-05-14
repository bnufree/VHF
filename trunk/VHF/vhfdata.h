#ifndef VHFDATA_H
#define VHFDATA_H

#include <QList>

enum EXTENSION_STATUS
{
    UNAVAILABLE,
    REGISTER,           //空闲状态
    BUSY,
    RINGING,
};

struct ExtensionData{
    QString number;//分机号
    QString username;//用户名
    EXTENSION_STATUS status_int;//分机状态
    QString address;//分机IP
    int groupNo;//组号
    QString meetingNo;//会议室号码
    EXTENSION_STATUS  preStatus; //分机前一个状态
    QString   status_str;
    ExtensionData()
    {
        preStatus = UNAVAILABLE;
    }
};

Q_DECLARE_METATYPE(ExtensionData)
typedef QList<ExtensionData>        ExtensionDataList;

enum ExtensionQueryStatus{
    Extension_Query_TimeOut = 0,
    Extension_Query_Failed,
    ExtenSion_Query_Success,
};

#endif // VHFDATA_H
