#ifndef RELAYCONTROLLER_H
#define RELAYCONTROLLER_H

#include <QObject>
#include "tcpsocket.h"

#define CONNECT_TIMEOUT 1*1000  //继电器连接超时时间
#define RESEND_TIMEOUT 70*1000  //继电器闭合指令循环发送间隔时间
#define HEARTBEAT_TIME 10*1000  //继电器心跳间隔时间
#define RECONNECT_TIME 5*1000  //继电器断线后重连间隔时间

class QTimer;
class ControlWidget;
enum RelayControllerStatus{
    Status_Unavailable = 0,
    Status_On,
    Status_Off,
};

struct RelayData{
    QString address;
    int port; //默认12345
    int channel;
    int autoCloseTime;
    int heartInterval;

};

class RelayController : public QObject
{
    Q_OBJECT
public:
    explicit RelayController(QObject *parent = nullptr,
                             const RelayData relayData = RelayData());
    ~RelayController();

    void socket_connect();
    void send_test_command();
    void send_set_status_command(int status, int duration);
    void startResenTimer();
    void stopResendTimer();
    void setLongPress(bool sts) {isLongPress = sts;}
    int  getStatus() const {return mStatus;}

    RelayData data;
    bool isConnected = false;
    bool isPlayingAudio = false;

    TcpSocket* socket;
    QTimer* resendTimer;
    QTimer* heartbeatTimer;
    QTimer* reconnectTimer;

signals:
    void SignalConnectionChanged();
    void set_server(QString,int);
    void sent_data(QByteArray);
    void signalStatusChanged(RelayControllerStatus sts);

public slots:
    void RelayConnected(QString ip, int port);
    void RelayDisConnected(QString ip, int port);
    void slotRecvData(const QByteArray& recv);

private:
    bool send_command(QByteArray command);
    void SendCommand(QByteArray command);
private:
    bool isResendTimer;
    bool isLongPress;
    int  mStatus;

};

#endif // RELAYCONTROLLER_H
