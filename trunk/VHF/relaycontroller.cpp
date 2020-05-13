#include "relaycontroller.h"
#include <QTimer>

extern bool  mDebug;

RelayController::RelayController(QObject *parent, const RelayData relayData) :
    QObject(parent),
    data(relayData),
    socket(new TcpSocket(this)),
    resendTimer(new QTimer(this)),
    heartbeatTimer(new QTimer(this)),
    reconnectTimer(new QTimer(this)),
    isResendTimer(false),
    isLongPress(false),
    isConnected(false),
    mStatus(Status_Unavailable)
{
    qRegisterMetaType<RelayControllerStatus>("RelayControllerStatus");
    connect(reconnectTimer, &QTimer::timeout, this ,&RelayController::socket_connect);
    reconnectTimer->start(RECONNECT_TIME);
    connect(heartbeatTimer, &QTimer::timeout, this, &RelayController::send_test_command);
    heartbeatTimer->start(data.heartInterval);
    connect(resendTimer, &QTimer::timeout, [=](){if(!isPlayingAudio)send_set_status_command(1, 60);});
    connect(this,SIGNAL(set_server(QString,int)), socket, SLOT(SetServer(QString,int)));
    connect(this,SIGNAL(sent_data(QByteArray)), socket, SLOT(SendData(QByteArray)));
    connect(socket,SIGNAL(Connected(QString,int)), this, SLOT(RelayConnected(QString,int)));
    connect(socket,SIGNAL(DisConnected(QString,int)), this, SLOT(RelayDisConnected(QString,int)));
    connect(socket, SIGNAL(RecieveData(QByteArray)), this, SLOT(slotRecvData(QByteArray)));
}

RelayController::~RelayController()
{
    socket->close();
    socket->deleteLater();
}

/**
 * @brief RelayController::socket_connect 连接继电器
 *        间隔10秒重连机制
 */
void RelayController::socket_connect()
{
    if (isConnected)
        return;
    if (data.address == "0.0.0.0" )
    {
        qDebug() << "Connection to" << qPrintable(data.address) << "cancelled";
        isConnected = false;
        return;
    }


    emit set_server(data.address, data.port);
}

/**
 * @brief RelayController::send_test_command 发送测试指令（用作心跳检测）
 */
void RelayController::send_test_command()
{
    if (!isConnected) return;
    //send_command(QString("AT\r\n").toLatin1());
    //读取状态变化
    QByteArray command = QString("AT+STACH0=?\r\n").toLatin1();
    send_command(command);
}

void RelayController::slotRecvData(const QByteArray& recv)
{
    QString res = QString::fromLatin1(recv);
    if(mDebug)  qDebug()<<"recv data from socket:"<<this->data.address<<res;
    QString head = QString("+STACH1:");
    int index = res.indexOf(head);
    if( index >= 0 )
    {
        //获取状态值
        QString sts = res.mid(index+8, 1);
        if(sts.length() == 1)
        {
            if(sts.toInt() == 0)
            {
                mStatus = RelayControllerStatus::Status_Off;
                emit signalStatusChanged(RelayControllerStatus::Status_Off);

            } else {
                mStatus = RelayControllerStatus::Status_On;
                emit signalStatusChanged(RelayControllerStatus::Status_On);
            }
        }
    }
}

void RelayController::startResenTimer()
{
    if(!isResendTimer) isResendTimer = true;
    resendTimer->start(RESEND_TIMEOUT);
}

void RelayController::stopResendTimer()
{
    resendTimer->stop();
    isResendTimer = false;
}

/**
 * @brief RelayController::send_set_status_command 发送设置闭合状态指令，接听按下时每间隔10秒重新发送闭合指令
 *        "AT+STACH0=1,30\r\n"  : 设置继电器所有通道吸合，延时30秒松开
 * @param status 目标状态。0：断开；1：闭合
 */
void RelayController::send_set_status_command(int status, int duration)
{
    QByteArray command;
    if (status)
    {
        if(isLongPress)
        {
            command = QString("AT+STACH0=1\r\n").toLatin1();
        } else
        {
//            int duration = 60;
            if(duration != 0)
            {
                command = (QString("AT+STACH0=1,%1\r\n").arg(duration)).toLatin1();
            } else
            {
                command = QString("AT+STACH0=1\r\n").toLatin1();
            }
        }
    }
    else
    {
        command = QString("AT+STACH0=0\r\n").toLatin1();
        resendTimer->stop();
    }
    SendCommand(command);
}

/**
 * @brief RelayController::send_command 发送指令（接收返回值判断成功与否）
 * @param command 指令
 * @return
 */
bool RelayController::send_command(QByteArray command)
{
    if(mDebug)  qDebug() << "Command to" << qPrintable(data.address) << ":" << QString(command);
    emit sent_data(command);

    return true;
}

/**
 * @brief RelayController::send_command 发送指令（单发送，不接收）
 * @param command 指令
 * @return
 */
void RelayController::SendCommand(QByteArray command)
{
    if(mDebug)  qDebug() << "Send to: " << qPrintable(data.address) << ":" << QString(command);
    emit sent_data(command);
}

void RelayController::RelayConnected(QString ip, int port)
{
    qDebug() << ip <<port<<"relay connected;";
    isConnected = true;
    mStatus = RelayControllerStatus::Status_Off;
    emit signalStatusChanged(RelayControllerStatus::Status_Off);
}

void RelayController::RelayDisConnected(QString ip, int port)
{
    qDebug() << ip <<port<<"relay disconnected;";
    isConnected = false;
    mStatus = RelayControllerStatus::Status_Unavailable;
    emit signalStatusChanged(RelayControllerStatus::Status_Unavailable);
}
