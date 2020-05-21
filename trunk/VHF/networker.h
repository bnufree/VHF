#ifndef NETWORKER_H
#define NETWORKER_H

#include <QObject>
#include <QThread>
#include <vhfdata.h>

#define REQUEST_TIMEOUT 10*1000

class QNetworkReply;

#define         NetWorkerIns        NetWorker::instance()

class NetWorker : public QObject
{
    Q_OBJECT
public:
    static NetWorker* instance();
    ~NetWorker();

    bool    isOk() const {return mIsOK;}
    void    startHeart();
    void    setToken(QString token);
    static  QString errorNo2String(int errorNo);
    int     judgeStatus(const QByteArray recv);
private:    
    QByteArray sendSyncRequest(const QString api, const QByteArray data = QByteArray());
    void sendAsyncRequest(const QString api, const QByteArray data = QByteArray());

public slots:
    void slotLogin(const QString username, const QString password, bool returnRes);
    void slotLogout();
    void slotQueryExtensionList();    
    void slotDialUpExtension(const QString caller, const QString callee);
    void slotDialUpOutto(const QString caller, const QString outto);
    void slotHangUpExtension(const QString extid);
    void slotUpdateExtensionUsername(const QString extid, const QString username);
    void slotPrompt(const QString extid, const QString prompt);
    void slotQueryExtensionStatus(const QString& number);


signals:
    void signalLogin(const QString& userName, const QString& pwd, bool returnRes);
    void signalSendLoginResult(int code);
    void signalLogout();
    void signalQueryExtension();
    void signalSendExtensionList(const ExtensionDataList& list, int code);
    void signalDialUpExtension(const QString caller, const QString callee);
    void signalDialUpOutto(const QString caller, const QString outto);
    void signalHangUpExtension(const QString extid);
    void signalUpdateExtensionUsername(const QString extid, const QString username);
    void signalPrompt(const QString extid, const QString prompt);
    void signalQueryExtensionStatus(const QString& number);

//    void finished(QNetworkReply *reply);
    void signal_extension_status_changed(const QString extension_num, const QString status);
    void signal_heartbeat_timeup();
    void SignalTimeout();
    void SignalReconnected();
    void SignalConfigChanged(const QString extId);

public slots:
    void slotPlay(const QString extid, const QString prompt)
    {
        slotPrompt(extid, prompt);
    }
    void slotNewReportConnection();
    void slotRecvReportData();
    void slotHeartBeat();
    void slotReadyReadServerData();

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
    QThread               mWorkThread;

    class MGarbage
    {
    public:
        ~MGarbage()
        {
            if (NetWorker::minstance) delete NetWorker::minstance;
        }
    };
    static MGarbage Garbage;
    bool                mIsOK;
};

#endif // NETWORKER_H
