#ifndef EXTENSION_H
#define EXTENSION_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#define PING_TIMER 5*1000   //定时PING延时
#define PING_WAIT_TIME 3*1000  //PING等待连接时间
#define AUTO_ENTER_TIMER 5*1000 //分机自动接入定时

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

class Extension : public QObject
{
    Q_OBJECT
public:
    explicit Extension(bool autoEnterMeeting, QObject *parent = nullptr,
                       const ExtensionData extensionData = ExtensionData());

    int dial_up_extension(const QString callee);
    int dial_up_outto(const QString outto);
    int dial_up_outto();
    int hang_up();
    int update_username(const QString new_username);
    void pingTest();
    void setStatus(const QString status);
    int  getStatus() const {return data.status_int;}
    QString getStatusStr() const {return data.status_str;}
    void setUserName(const QString& name) {data.username = name;}
    void setPreStatus(int sts) {data.preStatus = (EXTENSION_STATUS)sts;}
    void autoEnter();
    ExtensionData getData() const {return data;}

    bool isPingSuccess = true;
    bool isAutoEnter = false;
    bool isPlayingAudio = false;
    bool isMeetingAvailable = false;
private:
    ExtensionData data;

signals:
    void statusChanged();
};

#endif // EXTENSION_H
