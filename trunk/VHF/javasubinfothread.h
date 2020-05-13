#ifndef JAVASUBINFOTHREAD_H
#define JAVASUBINFOTHREAD_H

#include <QThread>
#include <QJsonObject>

//targetNumber	目标编号（预警自动时有值，定时自动和手动时无值）	String
//model	模式，1：预警自动；2：定时自动；3：手动	short
//frequency	播放次数，默认值：1	int
//timeInterval	播放时间间隔（播放次数大于1时生效） 单位：秒，默认值：0	int
//text	播放内容	String
enum JavaMode{
    Mode_None = 0,
    Mode_Auto_Warn,
    Mode_Auto_Timer,
    Mode_Manual,
};

typedef struct JavaInfo{
    QString     targetNumber;
    JavaMode    mode;
    int         frequency;
    int         timeInterval;
    QString     text;

    JavaInfo()
    {
        frequency = 1;
        timeInterval = 0;
    }

    JavaInfo(const QJsonObject& obj)
    {
        targetNumber = obj.value("targetNumber").toString();
        mode = JavaMode(obj.value("mode").toInt());
        frequency = obj.value("frequency").toInt();
        timeInterval = obj.value("timeInterval").toInt();
        text = obj.value("text").toString();
    }
}JAVA_INFO;

Q_DECLARE_METATYPE(JavaInfo)

class JavaSubInfoThread : public QThread
{
    Q_OBJECT
public:
    explicit JavaSubInfoThread(const QString& host, const QString topic, int port, QObject *parent = nullptr);

protected:
    void    run();

signals:
    void    signalSendJavaSubInfo(const JavaInfo& info);
public slots:
private:
    QString     mHost;
    int         mPort;
    QString     mTopic;
};

#endif // JAVASUBINFOTHREAD_H
