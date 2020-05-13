#include "quploadfilethread.h"
#include "yeastarnetwork.h"

QUploadFileThread::QUploadFileThread(QString strIp, int nPort, QString name, QString strPassword, QObject *parent)
    : QThread(parent)
    , m_bFInish(false)
    , m_strName(name)
    , m_strPassword(strPassword)
    , m_strIP(strIp)
    , m_nPort(nPort)
{
}

QUploadFileThread::~QUploadFileThread()
{
    if(isRunning())
    {
        finishThread();
    }
    qDebug()<<"stop upload thread now...";
    this->quit();
    this->wait();
    qDebug()<<"upload thread finished...";
}

void QUploadFileThread::run()
{
    qDebug()<<"start upload thread.........";
    YeastarNetWork *server = getVHFServer();
    while(true)
    {
        if(m_bFInish || !server) break;
        if(!server->isLogin())
        {
            if(!server->login(m_strName, m_strPassword))
            {
                QThread::msleep(1000);
                continue;
            }
        }

        BroadcastSetting task;
        m_lock.lock();
        if(m_listFile.size() > 0)
        {
            task = m_listFile[0];
            m_listFile.pop_front();
        }

        m_lock.unlock();
        QString strFileName = task.mLocalAudioName;
        if(!strFileName.isEmpty()){
            qDebug()<<"upload file:"<<strFileName;
            QString strVHFFileName = server->uploadFile(strFileName);
            qDebug()<<"upload return file:"<<strVHFFileName;
            if(strVHFFileName.isEmpty())
            {
                server->setLogin(false);
                qDebug()<<"reupload file now:"<<strFileName;
                if(!server->login(m_strName, m_strPassword))
                {
                    continue;
                }
                qDebug()<<"relogin file now:"<<strFileName;
                strVHFFileName = server->uploadFile(strFileName);
                qDebug()<<"reupload return file:"<<strVHFFileName;
            }

            if(strVHFFileName.isEmpty())
            {
                qDebug()<<"upload file failed";
                emit sigSendFileFaild(strFileName);
            } else
            {
                //m_pNetwork->getFileList();
                qDebug()<<"send to mainthread file:"<<strVHFFileName;
                task.mFileName = strVHFFileName;
                emit sigSendFileSuccess(task);
            }
        }
        QThread::msleep(100);

    }

    if(server) server->deleteLater();

    qDebug()<<__FUNCTION__<<__LINE__;
    return;
}

void QUploadFileThread::slotResponse(const QString& recv)
{
    if(recv.contains("success")&&recv.contains("file"))
    {
        QString strRecv = recv;
        QString str(":");
        int nIndex = strRecv.lastIndexOf(str);
        QString strName = strRecv.mid(nIndex + 1 + 1 ,strRecv.length() - nIndex - str.length() - 3);
        qDebug() << "send success name:" << strName;
        //emit sigSendFileSuccess(strName);
    }
    //emit sigResponse(recv);
}

void QUploadFileThread::addFile(const BroadcastSetting& setting)
{
    if(setting.mLocalAudioName.isEmpty())
        return;

    m_lock.lock();
    m_listFile.push_back(setting);
    m_lock.unlock();

    return;
}

YeastarNetWork* QUploadFileThread::getVHFServer()
{
    YeastarNetWork* server = new YeastarNetWork(m_strIP, m_nPort);
    connect(server, SIGNAL(signalSendStatus(const QString& )), this, SIGNAL(sigSendStatus(const QString& )));
    connect(server, SIGNAL(signalSendBytes(const QString& )), this, SIGNAL(sigResponse(const QString& )));
    return server;
}

void QUploadFileThread::finishThread()
{
//    while(true)
//    {
//        m_lock.lock();
//        if(m_listFile.size())
//        {
//            m_lock.unlock();
//            QThread::sleep(100);
//            continue;
//        }
//        m_bFInish = true;
//        break;
//    }
    m_bFInish = true;
    qDebug()<<__FUNCTION__<<__LINE__<<m_bFInish;
//    this->wait();
//    this->quit();
    return;
}

//void start(Priority = InheritPriority)
//{

//}
