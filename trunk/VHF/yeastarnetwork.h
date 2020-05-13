#ifndef YEASTARNETWORK_H
#define YEASTARNETWORK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

typedef QMap<QNetworkRequest::KnownHeaders, QVariant>       HeaderMap;

class YeastarNetWork : public QObject
{
    Q_OBJECT
public:
    explicit YeastarNetWork(const QString& ip, int port, QObject *parent = 0);
    virtual ~YeastarNetWork();
    bool     login(const QString& usr, const QString& pwd );
    QString  uploadFile(const QString& fileName);
    bool     getFileList();
    bool     isLogin() const {return mIsLogin;}
    void     setLogin(bool sts) {mIsLogin = sts;}
private:
    QString   postValFromMap(const QMap<QString, QString>& map);
    QByteArray get(const QString &url/*, const QList<QNetworkCookie>& list*/);
    QByteArray post(const QString &url, const QByteArray& post, const HeaderMap& headers);
    QByteArray post(const QString &url, QHttpMultiPart* multipart, const QString& boundary);
    QString     base64Str(const QString& utf8_str);

signals:
    void      signalSendBytes(const QString& recv);
    void      signalSendStatus(const QString& sts);
public slots:
private:
    QString     mIP;
    int         mPort;
    QString     mUrl;
    QNetworkAccessManager* mMgr;
    bool                    mIsLogin;
};

#endif // YEASTARNETWORK_H
