#include "networker.h"

#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QEventLoop>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QRegExp>
#include <QNetworkInterface>
#include <QTimer>
#include "inifile.h"

//心跳包使得token的时间延长30分钟，所以心跳时间设定为20分钟
#define HEARTBEAT_TIMER 60*1000*2  //心跳包间隔时间，默认10秒
NetWorker* NetWorker::minstance = nullptr;
NetWorker::MGarbage NetWorker::Garbage;

class NetWorker::NetCore
{
public:
    NetCore(NetWorker* q) :
        manager(new QNetworkAccessManager(q))
    {
        m_ServerAddr = IniFile::instance()->GetPbxServerIp();
        ippbxServerPort = IniFile::instance()->GetPbxServerPort();
        QString apiVersion = IniFile::instance()->GetApiVersion();
        m_ListenPort = IniFile::instance()->GetListenPort();
        m_Url = QString("http://%1:%2/api/%3/").arg(m_ServerAddr).arg(ippbxServerPort).arg(apiVersion);

        //获取本机IP地址
        QList<QHostAddress> ip_list = QNetworkInterface::allAddresses();
        foreach(QHostAddress ip_item, ip_list)
        {
            if (ip_item.protocol() == QAbstractSocket::IPv4Protocol &&
                    ip_item != QHostAddress::Null &&
                    ip_item != QHostAddress::LocalHost)
            {
                QStringList localhost_address = ip_item.toString().split('.');
                QStringList server_address = m_ServerAddr.split('.');
                if (localhost_address.at(0) == server_address.at(0) &&
                        localhost_address.at(1) == server_address.at(1) &&
                        localhost_address.at(2) == server_address.at(2))
                {
                    localhost_address_ = ip_item.toString();
                    qDebug() << "Localhost Address: " << localhost_address_;
                    break;
                }
            }
        }
    }

    QNetworkAccessManager* manager;
    QTcpServer* listenServer;
    QTimer* timer;

    QString m_ServerAddr;
    QString m_ListenPort;
    QString localhost_address_;
    QString m_ApiToken;
    QString m_Url;
    QString ippbxServerPort;
    bool bHeartbeat = true;
};

NetWorker* NetWorker::instance()
{
    if(!minstance) minstance = new NetWorker;
    return minstance;
}

NetWorker::NetWorker(QObject *parent) :
    QObject(parent),
    d(new NetWorker::NetCore(this))
{
    connect(d->manager, &QNetworkAccessManager::finished,
            this, &NetWorker::finished);

    d->listenServer = new QTcpServer(this);
    connect(d->listenServer, &QTcpServer::newConnection, this, &NetWorker::new_connection);
    if (!d->listenServer->listen(QHostAddress::Any, quint16(d->m_ListenPort.toInt())))
    {
        qDebug() << "Listen failed: " << d->listenServer->errorString();
    }
    else
    {
        qDebug() << "Listen successfully!"<<d->listenServer->serverAddress().toString();
    }

    d->timer = new QTimer(this);
    connect(d->timer, &QTimer::timeout, this, &NetWorker::signal_heartbeat_timeup);
    connect(this, SIGNAL(signalPlay(QString,QString,bool)), this, SLOT(slotPlay(QString,QString,bool)));

}

NetWorker::~NetWorker()
{
    if(d->listenServer)
    {
        d->listenServer->close();
        d->listenServer->deleteLater();
        qDebug()<<"listen server closed...";
    }
    delete d;
    d = nullptr;
}

void NetWorker::startHeartTimer()
{
    if(d->timer)
    {
        d->timer->start(HEARTBEAT_TIMER);
    }
}


void NetWorker::new_connection()
{
    qDebug()<<"a new center connected now..";
    //获取连接
    QTcpSocket* socket = d->listenServer->nextPendingConnection();
    if(socket)
    {
        //连接信号槽，已读取新数据
        connect(socket, &QTcpSocket::readyRead, this, &NetWorker::recieve_data);
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        qDebug()<<"center server:"<<socket->peerAddress().toString()<<socket->peerName();
    } else
    {
        qDebug()<<"abnormal client now...";
    }
}

/**
 * @brief NetWorker::recieve_data 接收到系统报告
 */
void NetWorker::recieve_data()
{
    QByteArray buffer;
    //读取缓冲区数据
    buffer = static_cast<QTcpSocket*>(sender())->readAll();
    qDebug()<<"NetWorker::recieve_data:"<<buffer;
    if (!buffer.isEmpty())
    {
        //解析HTTP数据
        QList<QByteArray> recieve_data = buffer.split('\n');
        qDebug() << "Report from PBX:" << recieve_data.last();

        //分析JSON数据
        QJsonDocument json_document = QJsonDocument::fromJson(recieve_data.last());
        QJsonObject json_object = json_document.object();
        QString action = json_object.value("action").toString();

        //分机状态变更
        if (action == "ExtensionStatus")
        {
            emit signal_extension_status_changed(json_object.value("extension").toString(),
                                                 json_object.value("status").toString());
        }
        //配置变更
        else if (action == "ConfigChange")
        {
            emit SignalConfigChanged(json_object.value("extid").toString());
        }
    }
}

void NetWorker::setToken(QString token)
{
    d->m_ApiToken = token;
}

/**
 * @brief NetWorker::login 登录接口，同步通信
 * @param username 用户名
 * @param password 经过MD5加密后的密码
 * @return errorNo 错误码，0为成功，1为无错误码失败
 */
int NetWorker::Login(const QString username, QString password)
{
    d->m_ApiToken.clear();
    QJsonObject obj;
    obj.insert("username", username);
    obj.insert("password", password);
    obj.insert("port", d->m_ListenPort);

    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QByteArray recv = sendSyncRequest("login", data);

    int errNo = judgeStatus(recv);
    if(errNo == 0)
    {
        mUserName = username;
        mPassWord = password;
    }

    return errNo;
}

/**
 * @brief NetWorker::logout 登出接口，未接收返回值
 */
void NetWorker::Logout()
{
    sendAsyncRequest("logout");
//    sendSyncRequest("logout");
}

/**
 * @brief NetWorker::heartbeat 发送心跳包，异步通信
 */
void NetWorker::SendHeartbeat()
{
    QJsonObject obj;
    obj.insert("ipaddr", d->m_ServerAddr);
    obj.insert("port", d->m_ListenPort);
    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    sendAsyncRequest("heartbeat", data);
}

/**
 * @brief NetWorker::queryDeviceList 查询分机信息，异步通信
 */
int NetWorker::QueryExtensionList(QByteArray &recv)
{
    recv = sendSyncRequest("extensionlist/query");
    return judgeStatus(recv);
}

/**
 * @brief NetWorker::dialExtension 分机互拨接口，同步通信
 * @param caller 主叫分机号
 * @param callee 被叫分机号
 * @return errorNo 错误码，0为成功，1为无错误码失败
 */
int NetWorker::DialUpExtension(const QString caller, const QString callee)
{
    QJsonObject obj;
    obj.insert("caller", caller);
    obj.insert("callee", callee);
    obj.insert("autoanswer", "yes");
    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QByteArray recv = sendSyncRequest("extension/dial_extension", data);

    return judgeStatus(recv);
}

/**
 * @brief NetWorker::DialUpOutto 拨打外线电话接口，同步通信
 * @param caller 主叫分机号
 * @param outto 被叫外线号
 * @return 错误码，0为成功，1为无错误码失败
 */
int NetWorker::DialUpOutto(const QString caller, const QString outto)
{

    QJsonObject obj;
    obj.insert("extid", caller);
    obj.insert("outto", outto);
    obj.insert("autoanswer", "yes");
    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QByteArray recv = sendSyncRequest("extension/dial_outbound", data);

    return judgeStatus(recv);
}

/**
 * @brief NetWorker::hangupExtension 挂断分机接口，同步通信
 * @param extid 挂断分机号
 * @return errorNo 错误码，0为成功，1为超时，2为无错误码失败
 */
int NetWorker::HangUpExtension(const QString extid)
{
    QJsonObject obj;
    obj.insert("extid", extid);
    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QByteArray recv = sendSyncRequest("extension/hangup", data);

    return judgeStatus(recv);
}

/**
 * @brief NetWorker::UpdateExtensionUsername 编辑分机用户名
 * @param extid 分机号
 * @param username 用户名
 * @return 错误码
 */
int NetWorker::UpdateExtensionUsername(const QString extid, const QString username)
{
    QJsonObject obj;
    obj.insert("extid", extid);
    obj.insert("username", username);
    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

#if 1
    QByteArray recv = sendSyncRequest("extension/update", data);

    return judgeStatus(recv);
#else
    sendAsyncRequest("extension/update", data);
    return 0;
#endif
}

/**
 * @brief NetWorker::judgeStatus 判断接口返回状态
 * @param recv 接口返回值
 * @return errorNo 错误码，0为成功，1为超时，2为无错误码失败
 */
int NetWorker::judgeStatus(const QByteArray recv)
{
    if (recv.isEmpty())
        return 1;
    QJsonParseError error;
    int errorNo = 0;
    QJsonDocument doc = QJsonDocument::fromJson(recv, &error);
    if (error.error == QJsonParseError::NoError)
    {
        if (!(doc.isNull() || doc.isEmpty()) && doc.isObject())
        {
            QJsonObject obj = doc.object();
            if (obj.contains("status"))
            {
                //状态为成功
                if (obj.value("status").toString() == "Success")
                {
                    if (obj.contains("token"))
                    {
                        //登录时有token字段，需要记录
                        NetWorker::instance()->setToken(obj.value("token").toString());
                    }
                }
                else
                {
                    if (obj.contains("errno"))
                        errorNo = obj.value("errno").toString().toInt();
                    else
                        errorNo = 2;
                }
            }
        }
    }
    return errorNo;
}

/**
 * @brief NetWorker::sendSyncRequest 同步通信接口
 * @param api 接口字段
 * @param data json格式数据
 * @return 接口返回值
 */
QByteArray NetWorker::sendSyncRequest(const QString api, const QByteArray data)
{
//    d->timer->stop();
//    d->timer->start(HEARTBEAT_TIMER);
    QUrl url(d->m_Url + api);
    if (!d->m_ApiToken.isEmpty())
    {
        QUrlQuery query;
        query.addQueryItem("token", d->m_ApiToken);
        url.setQuery(query);
    }

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply;
    if (data.isEmpty())
    {
        qDebug() << "Send to PBX:" << url.toString();
        reply = d->manager->get(request);
    }
    else
    {
        qDebug() << "Send to PBX:" << url.toString() << "\nData:" << QString(data);
        reply = d->manager->post(request, data);
    }
    QEventLoop eventloop;
    QTimer* timeout_timer = new QTimer(this);
    connect(timeout_timer, &QTimer::timeout, &eventloop, &QEventLoop::quit);
    connect(d->manager, &QNetworkAccessManager::finished, &eventloop, &QEventLoop::quit);
    timeout_timer->setSingleShot(true);
    timeout_timer->start(REQUEST_TIMEOUT);
    eventloop.exec();

    if (api == "heartbeat")
    {
        if (!reply->isReadable() || !reply->isFinished())
        {
            qDebug() << "----------------------------heartbeat timeout-----------------";
            d->bHeartbeat = false;
            emit SignalTimeout();
            reply->deleteLater();
            return QByteArray();
        }

        if (!d->bHeartbeat)
        {
            qDebug() << "----------------------------PBX Reconnected now-----------------";
            d->bHeartbeat = true;
            emit SignalReconnected();
        }
    }
    else
    {
        if (!reply->isReadable() || !reply->isFinished())
        {
            qDebug() << "operation timeout";
            reply->deleteLater();
            return QByteArray();
        }
    }

    QByteArray recv = reply->readAll();
    reply->deleteLater();
    qDebug() << "Reply from PBX:" << recv.data();

    return recv;
}

/**
 * @brief NetWorker::sendAsyncRequest 异步通信接口
 * @param api 接口字段
 * @param data json格式数据
 */
void NetWorker::sendAsyncRequest(const QString api, const QByteArray data)
{
//    d->timer->stop();
//    d->timer->start(HEARTBEAT_TIMER);
    QUrl url(d->m_Url + api);
    if (!d->m_ApiToken.isEmpty())
    {
        QUrlQuery query;
        query.addQueryItem("token", d->m_ApiToken);
        url.setQuery(query);
    }

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (data.isEmpty())
    {
        qDebug() << endl << url.toString() << endl;
        d->manager->get(request);
    }
    else
    {
        qDebug() << endl << url.toString() << QString(data) << endl;
        d->manager->post(request, data);
    }
}

/**
 * @brief NetWorker::send_heartbeat 发送心跳包
 * @return 错误码
 */
int NetWorker::send_heartbeat()
{
    QJsonObject obj;
    obj.insert("ipaddr", d->localhost_address_);
    obj.insert("port", d->m_ListenPort);
//    qDebug()<<"ipaddr:"<<d->localhost_address_<<"port:"<<d->m_ListenPort;

    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QByteArray recv = sendSyncRequest("heartbeat", data);

    return judgeStatus(recv);
}

/**
 * @brief NetWorker::errorNo2String 错误码转为错误信息
 * @param errorNo 错误码
 * @return 错误信息（可供客户端显示）
 */
QString NetWorker::errorNo2String(int errorNo)
{
    QString errorString;
    switch (errorNo)
    {
    case 1:errorString = QStringLiteral("连接超时");
        return errorString;
    case 3:errorString = QStringLiteral("话机离线");
        return errorString;
    case 10002:errorString = QStringLiteral("不支持XML数据封包格式");
        break;
    case 10003:errorString = QStringLiteral("API不支持的请求号处理");
        break;
    case 10004:errorString = QStringLiteral("部分参数为空");
        break;
    case 10006:errorString = QStringLiteral("分机不存在");
        break;
    case 10007:errorString = QStringLiteral("通话不存在");
        break;
    case 10008:errorString = QStringLiteral("分机不处于空闲状态");
        break;
    case 10009:errorString = QStringLiteral("分机不允许被监听");
        break;
    case 10010:errorString = QStringLiteral("监听模式不匹配");
        break;
    case 10011:errorString = QStringLiteral("分机不允许被监听");
        break;
    case 10012:errorString = QStringLiteral("IVR不存在");
        break;
    case 10013:errorString = QStringLiteral("队列不存在");
        break;
    case 10014:errorString = QStringLiteral("响铃组不存在");
        break;
    case 10015:errorString = QStringLiteral("通话未桥接成功");
        break;
    case 10016:errorString = QStringLiteral("接受来电/拒绝来电超时");
        break;
    case 10017:errorString = QStringLiteral("配置分机失败");
        break;
    case 10018:errorString = QStringLiteral("配置IVR失败");
        break;
    case 10019:errorString = QStringLiteral("配置队列失败");
        break;
    case 10020:errorString = QStringLiteral("添加闹铃失败或者修改闹铃失败");
        break;
    case 10021:errorString = QStringLiteral("分机闹铃不存在");
        break;
    case 10022:errorString = QStringLiteral("分机闹铃已存在");
        break;
    case 10023:errorString = QStringLiteral("语音文件不存在");
        break;
    case 10024:errorString = QStringLiteral("分机没有呼出权限");
        break;
    case 10025:errorString = QStringLiteral("apply太过频繁");
        break;
    case 10026:errorString = QStringLiteral("GSM模块名称不存在");
        break;
    case 10027:errorString = QStringLiteral("GSM模块正在使用或没有空闲的模块");
        break;
    case 10028:errorString = QStringLiteral("号码不能为空");
        break;
    case 10031:errorString = QStringLiteral("呼出号码不符合要求");
        break;
    case 10032:errorString = QStringLiteral("密码错误");
        break;
    case 10033:errorString = QStringLiteral("分机路由配置失败");
        break;
    case 20002:errorString = QStringLiteral("用户登录失败 (locked)");
        break;
    case 20003:errorString = QStringLiteral("用户登录失败（无效的用户名或密码）");
        break;
    case 20004:errorString = QStringLiteral("无此类Token");
        break;
    case 20005:errorString = QStringLiteral("Token为空");
        break;
    case 20006:errorString = QStringLiteral("Token超时");
        break;
    case 20007:errorString = QStringLiteral("数据为空");
        break;
    case 20008:errorString = QStringLiteral("端口错误");
        break;
    case 20009:errorString = QStringLiteral("请求号为空");
        break;
    case 20010:errorString = QStringLiteral("请求发送给apisrv处理失败");
        break;
    case 20011:errorString = QStringLiteral("HeartBeat请求配置失败");
        break;
    case 20012:errorString = QStringLiteral("录音文件不存在");
        break;
    case 20013:errorString = QStringLiteral("验证码错误");
        break;
    case 20014:errorString = QStringLiteral("录音文件已被删除");
        break;
    case 20015:errorString = QStringLiteral("allowedip错误");
        break;
    case 20018:errorString = QStringLiteral("语音文件不存在");
        break;
    default:errorString = tr("errorNo is %1").arg(errorNo);
        return errorString;
    }
    errorString.append(QString(" (%1)").arg(errorNo));
    return errorString;
}


/**
 * @brief NetWorker::Recording 给分机播放语音
 * @param extid 分机号  prompt 自定义音乐
 * @return errorNo 错误码，0为成功，1为超时，2为无错误码失败
 */
int NetWorker::Prompt(const QString extid, const QString prompt , bool bSyn)
{
    QJsonObject obj;
    obj.insert("extid", extid);
    obj.insert("prompt", prompt);
    obj.insert("autoanswer", "yes");
    qDebug()<<"send obj:"<<obj;
    QJsonDocument doc;
    doc.setObject(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    if(bSyn)
    {
        QByteArray recv = sendSyncRequest("extension/playprompt", data);

        return judgeStatus(recv);
    }

    sendAsyncRequest("extension/playprompt", data); //采用异步发送

    return 0;
}
