#include "javasubinfothread.h"
#include "zmq.h"
#include "zmq.hpp"
#include "zmq_utils.h"
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>

JavaSubInfoThread::JavaSubInfoThread(const QString& host, const QString topic, int port, QObject *parent) : QThread (parent)
  , mHost(host)
  , mPort(port)
  , mTopic(topic)
{
    qRegisterMetaType<JavaInfo>("const JavaInfo&");
}

void JavaSubInfoThread::run()
{
    void *pCtx = 0;
    void *pSock = 0;
    int timeout = 5 * 1000; //5s超时限制，没有收到消息就退出
    QString sAddress = QString("tcp://%1:%2").arg(mHost).arg(mPort);
    bool isConnected = false;
    int no_recv_num = 0;
    char* filter = 0;

    //创建context，zmq的socket 需要在context上进行创建
    if((pCtx = zmq_ctx_new()) == 0) goto END;
    //创建zmq socket ，socket目前有6中属性 ，这里使用SUB方式
    //具体使用方式请参考zmq官方文档（zmq手册）



    while(true)
    {
        //检查是否已经连接上
        if(!isConnected)
        {
            if((pSock = zmq_socket(pCtx, ZMQ_SUB)) == 0) goto END;
            filter = mTopic.toLatin1().data();
            if(zmq_setsockopt(pSock, ZMQ_SUBSCRIBE, filter, strlen(filter)) != 0) goto END;
            //设置消息接收等待时间
            zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

            if(zmq_connect(pSock, sAddress.toLatin1().data()) == 0)
            {
                isConnected = true;
            } else
            {
                //没有连接上, 重新连接
                isConnected = false;
                QThread::sleep(2);
                continue;
            }
        }

        //已经连接上,开始接收消息
        QByteArrayList recvlist;
        while (1) {
            int64_t more = 0;
            size_t more_size = sizeof (more);
            zmq_msg_t msg;
            zmq_msg_init(&msg);
            zmq_recvmsg(pSock, &msg, 0);
            QByteArray bytes2 = QByteArray((char*)zmq_msg_data(&msg),(int)(zmq_msg_size(&msg)));
            zmq_msg_close(&msg);
            if(bytes2.length() > 0)
            {
                recvlist.append(bytes2);
            }
            zmq_getsockopt (pSock, ZMQ_RCVMORE, &more, &more_size);
            if (more == 0)      //判断是否是最后消息
            {
                break; // 已到达最后一帧
            }
        }
        if(recvlist.length() < 2)
        {
            no_recv_num++;
            if(no_recv_num >= 10)
            {
                //这里清除掉旧的记录
                isConnected = false;
                QThread::sleep(2);
                no_recv_num = 0;
                if(pSock)
                {
                    zmq_close(pSock);
                    pSock = 0;
                }
                continue;
            }
        } else
        {
            no_recv_num = 0;

            //开始解析数据
            //1)topic检查
            QString topic = QString::fromUtf8(recvlist[0].data());
            if(topic != mTopic) continue;
            //2)符合要求的数据
            QJsonParseError error;
            error.error = QJsonParseError::NoError;
            QJsonDocument doc = QJsonDocument::fromJson(recvlist[1], &error);
            if(error.error != QJsonParseError::NoError) continue;
            if(!doc.isObject()) continue;
            JavaInfo info(doc.object());
            qDebug()<<info.text;
            emit signalSendJavaSubInfo(info);
        }
    }
END:
    if(pSock)   zmq_close(pSock);
    if(pCtx)    zmq_ctx_destroy(pCtx);
}
