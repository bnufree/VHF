#include "extension.h"
#include "networker.h"

#include <QMessageBox>
#include <QProcess>
extern bool mDebug;
Extension::Extension(bool autoEnterMeeting, QObject *parent, const ExtensionData extensionData) :
    QObject(parent),
    data(extensionData)
{
    if(mDebug)
    {
        QTimer* pingTimer = new QTimer(this);
        connect(pingTimer, &QTimer::timeout, this, &Extension::pingTest);
        pingTimer->start(PING_TIMER);
    }

    if(autoEnterMeeting)
    {
        isMeetingAvailable = true;
        QTimer* autoEnterTimer = new QTimer(this);
        connect(autoEnterTimer, &QTimer::timeout, this, &Extension::autoEnter);
        autoEnterTimer->start(AUTO_ENTER_TIMER);
    } else
    {
        isMeetingAvailable = false;
    }
}

/**
 * @brief Extension::dial_up_extension 分机互拨
 * @param callee 目标分机号码
 * @return 错误码
 */
int Extension::dial_up_extension(const QString callee)
{
    if (!isPingSuccess || data.status_int == EXTENSION_STATUS::UNAVAILABLE)
        return 3;
    QString caller = data.number;
    return NetWorker::instance()->DialUpExtension(caller, callee);
}

/**
 * @brief Extension::dial_up_outto 拨打外线（会议室号码为外线）
 * @param outto 目标外线号码
 * @return 错误码
 */
int Extension::dial_up_outto(const QString outto)
{
    if (!isPingSuccess || data.status_int == EXTENSION_STATUS::UNAVAILABLE)
        return 3;
    QString caller = data.number;
    return NetWorker::instance()->DialUpOutto(caller, outto);
}

int Extension::dial_up_outto()
{
    return dial_up_outto(data.meetingNo);
}

/**
 * @brief Extension::hang_up 挂断分机
 * @return 错误码
 */
int Extension::hang_up()
{
    if (!isPingSuccess || data.status_int == EXTENSION_STATUS::UNAVAILABLE)
        return 3;
    return NetWorker::instance()->HangUpExtension(data.number);
}

/**
 * @brief Extension::update_username 编辑分机用户名
 * @param new_username 新用户名
 * @return 错误码
 */
int Extension::update_username(const QString new_username)
{
    return NetWorker::instance()->UpdateExtensionUsername(data.number, new_username);
}

void Extension::pingTest()
{
//    qDebug() << "Ping process" << address << status_;
    if (data.status_int == EXTENSION_STATUS::UNAVAILABLE)
        return;

    QProcess pingProcess;
    QString strArg = "ping " + data.address + " -n 1 -l 1";  //address 为设备IP地址;-n为发包次数;-l为包大小
//    QString strArg = "ping " + data.address + " -n 2";      //address 为设备IP地址;-n为发包次数;-l为包大小
    pingProcess.start(strArg,QIODevice::ReadOnly);
    pingProcess.waitForFinished(PING_WAIT_TIME);

    QString p_stdout = QString::fromLocal8Bit( pingProcess.readAllStandardOutput());
    pingProcess.deleteLater();
//    qDebug()<<p_stdout ;

    bool bPingSuccess = true;
    if(p_stdout.contains("TTL="))
    {
        bPingSuccess = true;
        if(mDebug)  qDebug() << "Check status" << qPrintable(data.address) << ": online";
    }
    else {
        qDebug() << "Check status" << qPrintable(data.address) << ": offline";
    }
    if (isPingSuccess != bPingSuccess)
    {
        isPingSuccess = bPingSuccess;
        emit statusChanged();
    }
}

void Extension::setStatus(const QString status)
{
    if (status == "Busy")
        data.status_int = EXTENSION_STATUS::BUSY;
    else if (status == "Registered")
        data.status_int = EXTENSION_STATUS::REGISTER;
    else if (status == "Ringing")
        data.status_int = EXTENSION_STATUS::RINGING;
    else
        data.status_int = EXTENSION_STATUS::UNAVAILABLE;

    data.status_str = status;
    emit statusChanged();
}

void Extension::autoEnter()
{
    if (!isAutoEnter)
        return;
    if(isPlayingAudio) return;

    qDebug()<<"data status:"<<data.status_int<<data.status_str;
    if (data.status_int == EXTENSION_STATUS::REGISTER)
        dial_up_outto();
}
