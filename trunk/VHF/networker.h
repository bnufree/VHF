#ifndef NETWORKER_H
#define NETWORKER_H

#include <QObject>

#define REQUEST_TIMEOUT 10*1000

class QNetworkReply;

#define         NetWorkerIns        NetWorker::instance()

class NetWorker : public QObject
{
    Q_OBJECT
public:
    static NetWorker* instance();
    ~NetWorker();

    void setToken(QString token);
    static QString errorNo2String(int errorNo);
    int judgeStatus(const QByteArray recv);
    QByteArray sendSyncRequest(const QString api, const QByteArray data = QByteArray());
    void sendAsyncRequest(const QString api, const QByteArray data = QByteArray());

    int send_heartbeat();
    int Login(const QString username, const QString password);
    int Relogin() {return Login(mUserName, mPassWord);}
    void Logout();
    void SendHeartbeat();
    int QueryExtensionList(QByteArray& recv);
    int DialUpExtension(const QString caller, const QString callee);
    int DialUpOutto(const QString caller, const QString outto);
    int HangUpExtension(const QString extid);
    int UpdateExtensionUsername(const QString extid, const QString username);
    void startHeartTimer();
    int Prompt(const QString extid, const QString prompt , bool bSyn = false);


signals:
    void finished(QNetworkReply *reply);
    void signal_extension_status_changed(const QString extension_num, const QString status);
    void signal_heartbeat_timeup();
    void SignalTimeout();
    void SignalReconnected();
    void SignalConfigChanged(const QString extId);
    void signalPlay(const QString extid, const QString prompt , bool bSyn);

public slots:
    void new_connection();
    void recieve_data();
    void slotPlay(const QString extid, const QString prompt , bool bSyn){Prompt(extid, prompt, bSyn);}

private:
    explicit NetWorker(QObject* parent = nullptr);
    NetWorker(const NetWorker&) Q_DECL_EQ_DELETE;
    NetWorker& operator=(NetWorker rhs) Q_DECL_EQ_DELETE;

    //为保证二进制兼容，封装一个辅助类
    class NetCore;
    friend class NetCore;
    NetCore* d;
    QString mUserName;
    QString mPassWord;

private:
    static NetWorker     *minstance;

    class MGarbage
    {
    public:
        ~MGarbage()
        {
            if (NetWorker::minstance) delete NetWorker::minstance;
        }
    };
    static MGarbage Garbage;
};

#endif // NETWORKER_H
