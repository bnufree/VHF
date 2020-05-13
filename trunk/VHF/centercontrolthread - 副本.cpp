#include "centercontrolthread.h"
#include "tcpsocket.h"
#include <QTimer>
#include <QDebug>
#include "inifile.h"
#include <QDateTime>

extern  bool        mDebug;
CenterControlThread::CenterControlThread(const QString& host, int port, int interval, QObject *parent) : QObject(parent)
  , mHost(host)
  , mPort(port)
  , mInterVal(interval)
  , mSocket(0)
  , mTimer(0)
  , mIsConnected(false)
  , mIsOpend(false)
{
    mMaxOpendTimeSecs = IniFile::instance()->getMicrophoneMaxOpenSeconds();
    mSocket = new QTcpSocket(this);
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    connect(mSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(slotSocketStateChange(QAbstractSocket::SocketState)));
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(slotReadData()));
    mTimer = new QTimer();
    mTimer->setInterval(mInterVal);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(slotRequest()));

    qDebug()<<"host:"<<mHost<<" port:"<<mPort<<" interval:"<<mInterVal;

    this->moveToThread(&mWorkThread);
    mWorkThread.start();    
    mTimer->start();
}
void CenterControlThread::start()
{
    qDebug()<<"start connect to host"<<mHost<<mPort;
    mSocket->connectToHost(mHost, mPort);
}

CenterControlThread::~CenterControlThread()
{
    if(mTimer) mTimer->stop();
    if(mSocket) mSocket->abort();
}

void CenterControlThread::slotRequest()
{
    if(mDebug)  qDebug()<<"start request data:"<<mSocket;
    if(mSocket)
    {
        if(mDebug)  qDebug()<<"socket state:"<<mSocket->state();
        if(mSocket->state() == QAbstractSocket::ConnectedState)
        {
            QByteArray cmd = QString("AT+OCCH1=?\r\n").toLatin1();

            if(mDebug)  qDebug()<<" send cmd:"<<cmd;
            mSocket->write(cmd);
            mSocket->flush();
        }
    }
}

void CenterControlThread::slotReadData()
{
    if(!mIsConnected || !mSocket) return;
    QByteArray data = mSocket->readAll();
    if(data.size() == 0) return;
    QString str = QString::fromLatin1(data);
    if(!str.isEmpty()) mRecvContentList.append(str);
    //对数据进行分割
    if(mDebug)  qDebug()<<"recv data:"<<data<<" exist:"<<mRecvContentList;
    QStringList res = mRecvContentList.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    if(res.size() == 0) return;
    // 检查最后一个选项
    str = res.last();
    if(str.size() == 8)
    {
        if(str == "+OCCH1:1")
        {
            //        qDebug()<<"send open command";
            if(!mIsOpend)
            {
                mInitOpendTime = QDateTime::currentSecsSinceEpoch();
                mIsOpend = true;
                qDebug()<<"first init open at time:"<< QDateTime::fromSecsSinceEpoch(mInitOpendTime).toString("yyyy-MM-dd hh:mm:ss  ")<<mInitOpendTime;
                qDebug()<<"send normal open cmd now.....";
                emit signalRecvCmd(1, false);
            }
            if(mIsOpend)
            {
                if(QDateTime::currentSecsSinceEpoch() - mInitOpendTime >= mMaxOpendTimeSecs)
                {
                    qDebug()<<"send abnormal close cmd"<<mMaxOpendTimeSecs;
                    //麦克风出现异常,强制关闭
                    emit signalRecvCmd(0, true);
                } else
                {

                }
            } else
            {
                qDebug()<<"abnomal occured..why??????????";
            }

        } else if(str == "+OCCH1:0")
        {
            if(mIsOpend)
            {
                qDebug()<<"send close command";
                emit signalRecvCmd(0, false);
            }
            mIsOpend = false;
        } else
        {
            qDebug()<<"recv error string:"<<str;
            mIsOpend = false;
        }
        mRecvContentList.clear();
    } else if(str == "ERROR")
    {
        mRecvContentList.clear();
    } else
    {
        mRecvContentList = str;
    }

    if(mDebug)  qDebug()<<"current cmd:"<<mRecvContentList;

}

void CenterControlThread::slotSocketStateChange(QAbstractSocket::SocketState state)
{
    if(state == QAbstractSocket::ConnectedState)
    {
        mIsConnected = true;
        qDebug()<<"control socket connted";
    } else if(state == QAbstractSocket::UnconnectedState)
    {
        mIsConnected = false;
        qDebug()<<"control socket disconnted";
        mSocket->abort();
        QThread::sleep(1);
        mSocket->connectToHost(mHost, mPort);
    } else if(state == QAbstractSocket::HostLookupState)
    {
        qDebug()<<"control socket lookup host";
    } else if(state == QAbstractSocket::ConnectingState)
    {
        qDebug()<<"control socket connecting...";
    } else if(state == QAbstractSocket::ClosingState)
    {
        qDebug()<<"control socket closed";
    }
}
