#include "centercontrolthread.h"
#include "tcpsocket.h"
#include <QTimer>
#include <QDebug>
#include "inifile.h"
#include <QDateTime>

extern  bool        mDebug;
CenterControlThread::CenterControlThread(const QString& host, int port, int interval, QObject *parent) : QThread(parent)
  , mHost(host)
  , mPort(port)
  , mInterVal(interval)
  , mIsOpend(false)
  , mCancel(false)
{
    mMaxOpendTimeSecs = IniFile::instance()->getMicrophoneMaxOpenSeconds();
    qDebug()<<"host:"<<mHost<<" port:"<<mPort<<" interval:"<<mInterVal<<" max open secs:"<<mMaxOpendTimeSecs;
}
void CenterControlThread::run()
{
    bool isConnect = false;
    QTcpSocket socket;
    while (!mCancel)
    {
        //连接到服务器
        if(!isConnect)
        {
            qDebug()<<"start connect to host"<<mHost<<mPort;
            socket.connectToHost(mHost, mPort);
            if(!socket.waitForConnected(10000))
            {
                qDebug()<<"connect to host timeout. error:"<<socket.errorString();
                sleep(5);
                continue;
            }
            isConnect = true;
        }

        if(mDebug)  qDebug()<<"socket state:"<<socket.state();
        if(socket.state() == QAbstractSocket::ConnectedState)
        {
            //发送查询命令,获取状态
            QByteArray cmd = QString("AT+OCCH1=?\r\n").toLatin1();
            if(mDebug)  qDebug()<<" send cmd:"<<cmd;
            socket.write(cmd);
            if(!socket.waitForBytesWritten(3000))
            {
                qDebug()<<"write data to host timeout. error:"<<socket.errorString();
                sleep(5);
                socket.abort();
                isConnect = false;
                continue;
            }
            socket.flush();
            //获取返回的内容
            if(!socket.waitForReadyRead(10000)) continue;
            qint64 byteSize = socket.bytesAvailable();
            QByteArray data = socket.readAll();
            if(data.size() == 0) continue;
            QString str = QString::fromLatin1(data);
            if(!str.isEmpty()) mRecvContentList.append(str);
            //对数据进行分割
            if(mDebug)  qDebug()<<"recv data:"<<data<<" exist:"<<mRecvContentList;
            QStringList res = mRecvContentList.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
            if(res.size() == 0) continue;
            // 检查最后一个选项
            str = res.last();
            if(str.size() == 8)
            {
                if(str == "+OCCH1:1")
                {
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
        } else
        {
            socket.abort();
            isConnect = false;
        }
    }

    socket.abort();

}

CenterControlThread::~CenterControlThread()
{
}
