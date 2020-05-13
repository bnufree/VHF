#include "yeastarnetwork.h"
#include <QCryptographicHash>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDomDocument>

const QString _keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
QString YeastarNetWork::base64Str(const QString &utf8_str)
{
    QString output = "";
    char chr1, chr2, chr3, enc1, enc2, enc3, enc4;
    int i = 0;
    while (i < utf8_str.length()) {
        chr1 = utf8_str.at(i++).toLatin1();
        chr2 = utf8_str.at(i++).toLatin1();
        chr3 = utf8_str.at(i++).toLatin1();
        enc1 = chr1 >> 2;
        enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
        enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
        enc4 = chr3 & 63;
        if (QChar(chr2).isNumber()) {
            enc3 = enc4 = 64;
        } else if (QChar(chr3).isNumber()) {
            enc4 = 64;
        }
        output = output +
                _keyStr.mid(enc1, 1) + _keyStr.mid(enc2,1) +
                _keyStr.mid(enc3, 1) + _keyStr.mid(enc4, 1);
    }
    return output;
}

//    // public method for decoding
//    this.decode = function (input) {
//        var output = "";
//        var chr1, chr2, chr3;
//        var enc1, enc2, enc3, enc4;
//        var i = 0;
//        input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");
//        while (i < input.length) {
//            enc1 = _keyStr.indexOf(input.charAt(i++));
//            enc2 = _keyStr.indexOf(input.charAt(i++));
//            enc3 = _keyStr.indexOf(input.charAt(i++));
//            enc4 = _keyStr.indexOf(input.charAt(i++));
//            chr1 = (enc1 << 2) | (enc2 >> 4);
//            chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
//            chr3 = ((enc3 & 3) << 6) | enc4;
//            output = output + String.fromCharCode(chr1);
//            if (enc3 != 64) {
//                output = output + String.fromCharCode(chr2);
//            }
//            if (enc4 != 64) {
//                output = output + String.fromCharCode(chr3);
//            }
//        }
//        output = _utf8_decode(output);
//        return output;
//    }

//    // private method for UTF-8 encoding
//    _utf8_encode = function (string) {
//        string = string.replace(/\r\n/g,"\n");
//        var utftext = "";
//        for (var n = 0; n < string.length; n++) {
//            var c = string.charCodeAt(n);
//            if (c < 128) {
//                utftext += String.fromCharCode(c);
//            } else if((c > 127) && (c < 2048)) {
//                utftext += String.fromCharCode((c >> 6) | 192);
//                utftext += String.fromCharCode((c & 63) | 128);
//            } else {
//                utftext += String.fromCharCode((c >> 12) | 224);
//                utftext += String.fromCharCode(((c >> 6) & 63) | 128);
//                utftext += String.fromCharCode((c & 63) | 128);
//            }

//        }
//        return utftext;
//    }

//    // private method for UTF-8 decoding
//    _utf8_decode = function (utftext) {
//        var string = "";
//        var i = 0;
//        var c = c1 = c2 = 0;
//        while ( i < utftext.length ) {
//            c = utftext.charCodeAt(i);
//            if (c < 128) {
//                string += String.fromCharCode(c);
//                i++;
//            } else if((c > 191) && (c < 224)) {
//                c2 = utftext.charCodeAt(i+1);
//                string += String.fromCharCode(((c & 31) << 6) | (c2 & 63));
//                i += 2;
//            } else {
//                c2 = utftext.charCodeAt(i+1);
//                c3 = utftext.charCodeAt(i+2);
//                string += String.fromCharCode(((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
//                i += 3;
//            }
//        }
//        return string;
//    }
//}


YeastarNetWork::YeastarNetWork(const QString& ip, int port, QObject *parent) :
    QObject(parent),
    mIP(ip),
    mPort(port),
    mMgr(new QNetworkAccessManager),
    mIsLogin(false)
{
    mUrl = QString("http://%1:%2").arg(mIP).arg(mPort);
    qDebug()<<"url:"<<mUrl<<mPort;
}

YeastarNetWork::~YeastarNetWork()
{
    if(mMgr) delete mMgr;
}

QString YeastarNetWork::postValFromMap(const QMap<QString, QString> &map)
{
    QStringList list;
    QMap<QString, QString>::const_iterator it = map.begin();
    for(; it != map.end(); it++)
    {
        qDebug()<<it.key()<<it.value();
        list.append(QString("%1=%2").arg(it.key()).arg(it.value()));
    }
    qDebug()<<list;
    return list.join("&");
}

bool YeastarNetWork::getFileList()
{
    if(!mIsLogin)
    {
        if(!login("admin", "password")) return false;
    }
    //http://192.168.5.150:8621/cgi/webcgi?pbxcenter=get&node=customprompt&_dc=1557799128563&page=1&start=0&limit=10]
    QByteArray recv = get(QString("%1/cgi/webcgi?pbxcenter=get&node=customprompt&_dc=%1&page=1&start=0&limit=50")
                           .arg(mUrl).arg(QDateTime::currentMSecsSinceEpoch()));

    //emit signalSendBytes(QString::fromUtf8(recv));
    if(recv.length() > 0)
    {
        QDomDocument doc;
        if(!doc.setContent(recv))
        {
            emit signalSendBytes("xml file have error: " );
            return false;
        }
        QDomElement rootNode = doc.documentElement();
        //总计
        int total = 0;
        QDomElement totalNode = rootNode.firstChildElement("total");
        if(!totalNode.isNull())
        {
            QDomElement ele = totalNode.toElement();
            total = ele.text().toInt();
        }
        emit signalSendStatus(tr("total files count:%1").arg(total));
        //具体的文件
        QDomElement patchNode = rootNode.firstChildElement("customprompt");
        while(!patchNode.isNull())
        {
            int id;
            QString name, value, file;
            QDomNodeList list = patchNode.childNodes();
            //emit signalSendStatus(tr("child node size:%1").arg(list.size()));
            for(int i=0; i<list.size(); i++)
            {
                QDomNode node = list.item(i);
                if(node.nodeName() == "id") id = node.toElement().text().toInt();
                if(node.nodeName() == "name") name = node.toElement().text();
                if(node.nodeName() == "value") value = node.toElement().text();
                if(node.nodeName() == "file") file = node.toElement().text();
            }
            qDebug() << "getFileList:" << tr("id:%1, value:%2, file:%3").arg(id).arg(value).arg(file);
            emit signalSendStatus(tr("id:%1, value:%2, file:%3").arg(id).arg(value).arg(file));
            patchNode = patchNode.nextSiblingElement("customprompt");
        }
    }

    return true;
}



QString YeastarNetWork::uploadFile(const QString &fileName)
{
    QString strDestFileName("");
    if(!mIsLogin)
    {
        if(!login("admin", "password")) return strDestFileName;
    }
    QFile *file = new QFile(fileName);
    if(!file->open(QIODevice::ReadOnly))
    {
        file->close();
        delete file;
        emit signalSendStatus("open file error");
        return strDestFileName;
    }
    QString wkfIle = fileName;
    int index = fileName.lastIndexOf("/");
    if(index >= 0)
    {
        wkfIle = fileName.right(fileName.length() - index - 1);
    }

    //构建post的值
    QString position_header = QString("form-data; name=\"file\"; filename=\"%1\"").arg(wkfIle);
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QString boundary = "----WebKitFormBoundaryjZzoxBAWrYV1eeXF";
    //multiPart->setBoundary(boundary);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, position_header);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wave");
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);
    qDebug()<<"url:"<<mUrl;
    QByteArray recv = post(QString("%1/cgi/webcgi?pbxcenter=upload&node=customprompt&webpage=menuprompt&weblangprefix=l.cp.customprompt&webtabsactive=3&weboperation=upload&file=%2")
                           .arg(mUrl).arg(wkfIle), multiPart, QString::fromLatin1(multiPart->boundary()));

    emit signalSendBytes(QString::fromUtf8(recv));

    bool sts = false;
    QString resfile = "";
    if(recv.length() > 0)
    {
        //{success:true, file:'2019-chengdu.wav'}
        QString str = QString::fromUtf8(recv);
        int index = 0;
        int i =0;
        if(recv.contains("success")&&recv.contains("file"))
        {
            QString strRecv = recv;
            QString str(":");
            int nIndex = strRecv.lastIndexOf(str);
            QString strName = strRecv.mid(nIndex + 1 + 1 ,strRecv.length() - nIndex - str.length() - 3);
            qDebug() << "send success name:" << strName;
            strDestFileName = strName;
            //emit sigSendFileSuccess(strName);
        }
#if 0
        while ((index = str.indexOf(":", index)) >= 0) {
            i++;
            //emit signalSendStatus(str.right(str.length() - index - 1));
            if(i ==1)
            {
                int index_dot = str.indexOf(",", index);
                int len = str.length() - index -2;
                if(index_dot > 0)
                {
                    len = index_dot - index -1;
                }
                QString res = str.mid(index+1, len);
                if(res.trimmed() == "true")
                {
                    sts = true;
                }
            } else
            {
                resfile = str.right(str.length() - index - 1);
                strDestFileName = resfile;
            }
            index += 1;
            if(i == 2) break;

        }

        //emit signalSendStatus(tr("upload  sts:%1, file: %2").arg(sts).arg(resfile));
#endif
    }

    return strDestFileName;
}

bool YeastarNetWork::login(const QString &usr, const QString &pwd)
{
    QString passwordMd5;
    QByteArray baMd5 = QCryptographicHash::hash(pwd.toUtf8(), QCryptographicHash::Md5);
    QString hex_md5 =  QString::fromStdString(baMd5.toHex().constData());
    QByteArray base64 = hex_md5.toUtf8().toBase64(QByteArray::OmitTrailingEquals);
    QByteArray parse_hex = QByteArray::fromBase64(base64);
    passwordMd5.append(base64);
//    qDebug()<<"base64:"<<base64<<" md5:"<<hex_md5<<" my base64:"<<base64Str(hex_md5);
    //构建post的值
    QMap<QString, QString> postMap;
    postMap["weboperation"] = "login";
    postMap["logignore"] = "password";
    postMap["username"] = usr;
//    postMap["password"] = "PY[2\\IPlO4K3[YG5PlXmPlHmQFO{P4Tn[li6OoPoQVm?";
    postMap["password"] = pwd;
    QString postVal = postValFromMap(postMap);
    qDebug()<<"post value:"<<postVal<<baMd5.toHex();
    //构建Header;
    HeaderMap headers;
    headers[QNetworkRequest::ContentTypeHeader] = "application/x-www-form-urlencoded; charset=UTF-8";
    QByteArray recv = post(QString("%1/cgi/webcgi?login").arg(mUrl), postVal.toUtf8(), headers);
    emit signalSendBytes(QString::fromUtf8(recv));
    bool sts = false;
    if(recv.length() > 0)
    {
        QString resStr = QString::fromUtf8(recv);
        QRegularExpressionMatch match;
        QRegularExpression reg("<result>([\\-0-9]{1,})</result>");
        int index = resStr.indexOf(reg, 0, &match);
        emit signalSendStatus(tr("find result:%1").arg(index));
        if(match.hasMatch())
        {
            sts = (match.captured(1).toInt() == 1);
        }
        emit signalSendStatus(match.captured());
    }
    mIsLogin = sts;
    if(sts)
    {
        emit signalSendStatus("log in success");
    }
    return sts;
}


QByteArray YeastarNetWork::get(const QString &url/*, const QList<QNetworkCookie>& list*/)
{
    if(!mMgr)   return QByteArray();
    QByteArray recv;
    QNetworkReply *reply = mMgr->get(QNetworkRequest(url));
//    qDebug()<<reply;
    if(!reply) return recv;

    QEventLoop subloop;
    connect(reply, SIGNAL(finished()), &subloop, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &subloop, SLOT(quit()));
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &subloop, SLOT(quit()));
    timer.start(10*1000);
    subloop.exec();
    if(timer.isActive())
    {
//        qDebug()<<"reply finished";
        timer.stop();
    }

//    qDebug()<<reply->isFinished()<<reply->errorString();
    if(reply->error() == QNetworkReply::NoError && reply->isFinished())
    {
        recv = reply->readAll();
    }

    reply->abort();
    reply->close();
    delete reply;
    reply = 0;
    return recv;
}

QByteArray YeastarNetWork::post(const QString &url, QHttpMultiPart *multipart, const QString& boundary)
{
    if(!mMgr) return QByteArray();
    QByteArray recv;
    QNetworkRequest req(url);
    QString content_header = tr("multipart/form-data; boundary=%1").arg(boundary);
    req.setHeader(QNetworkRequest::ContentTypeHeader, content_header);
    emit signalSendStatus(url);
    QNetworkReply *reply = mMgr->post(req, multipart);
    if(!reply)
    {
        emit signalSendStatus("reply error");
        return recv;
    }

    multipart->setParent(reply);

    QEventLoop subloop;
    connect(reply, SIGNAL(finished()), &subloop, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &subloop, SLOT(quit()));
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &subloop, SLOT(quit()));
    timer.start(10*1000);
    subloop.exec();
    if(timer.isActive())
    {
        emit signalSendStatus("reply finished");
        timer.stop();
    } else
    {
        emit signalSendStatus("time out");
    }
    if(reply->error() == QNetworkReply::NoError && reply->isFinished())
    {
         emit signalSendStatus("start read reply content");
        recv = reply->readAll();
    } else
    {
        emit signalSendStatus(QString("reply finished with error :%1").arg(reply->errorString()));
    }

    QNetworkCookieJar * jar= mMgr->cookieJar();
    if(jar)
    {
        QList<QNetworkCookie> list= jar->cookiesForUrl(req.url());
        for(int i=0;i<list.length();i++)
        {
            qDebug()<<list[i].name()<<":"<<list[i].value();
            emit signalSendStatus(QString("cookie found:%1-%2").arg(QString::fromStdString(list[i].name().data())).arg(QString::fromStdString(list[i].value().data())));
        }
    } else
    {
        emit signalSendStatus("cookie jar not found!!!");
    }
    reply->abort();
    reply->close();
    delete reply;
    reply = 0;
    return recv;
}

QByteArray YeastarNetWork::post(const QString &url, const QByteArray& post, const HeaderMap& headers)
{
    if(!mMgr) return QByteArray();
    QByteArray recv;
    QNetworkRequest req(url);
    emit signalSendStatus(url);
    foreach (QNetworkRequest::KnownHeaders header, headers.keys()) {
        req.setHeader(header, headers[header]);
    }
    QNetworkReply *reply = mMgr->post(req, post);
    if(!reply)
    {
        emit signalSendStatus("reply error");
        return recv;
    }

    QEventLoop subloop;
    connect(reply, SIGNAL(finished()), &subloop, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &subloop, SLOT(quit()));
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &subloop, SLOT(quit()));
    timer.start(10*1000);
    subloop.exec();
    if(timer.isActive())
    {
        emit signalSendStatus("reply finished");
        timer.stop();
    } else
    {
        emit signalSendStatus("time out");
    }
    if(reply->error() == QNetworkReply::NoError && reply->isFinished())
    {
         emit signalSendStatus("start read reply content");
        recv = reply->readAll();
    } else
    {
        emit signalSendStatus(QString("reply finished with error :%1").arg(reply->errorString()));
    }
    QNetworkCookieJar * jar= mMgr->cookieJar();
    if(jar)
    {
        QList<QNetworkCookie> list= jar->cookiesForUrl(req.url());
        for(int i=0;i<list.length();i++)
        {
            qDebug()<<list[i].name()<<":"<<list[i].value();
            emit signalSendStatus(QString("cookie found:%1-%2").arg(QString::fromStdString(list[i].name().data())).arg(QString::fromStdString(list[i].value().data())));
        }
    } else
    {
        emit signalSendStatus("cookie jar not found!!!");
    }
    reply->abort();
    reply->close();
    delete reply;
    reply = 0;
    return recv;
}

