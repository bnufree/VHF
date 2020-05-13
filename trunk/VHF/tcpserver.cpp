#include "tcpserver.h"
#include <QHostAddress>
#include <QTcpSocket>

TcpServer::TcpServer(QObject *parent,int numConnections) :
      QTcpServer(parent)
{
    tcpClients = new  QHash<qintptr,TcpSocket *>;
    setMaxPendingConnections(numConnections);
}
TcpServer::~TcpServer()
{
    emit this->sendDisConnect(-1);
    delete tcpClients;
}
void TcpServer::setMaxPendingConnections(int numConnections)
{
    //调用Qtcpsocket函数，设置最大连接数，主要是使maxPendingConnections()依然有效
    this->QTcpServer::setMaxPendingConnections(numConnections);
    this->maxConnections = numConnections;
}
void TcpServer::incomingConnection(qintptr handle)
{
    if (tcpClients->size() > maxPendingConnections())//继承重写此函数后，QTcpServer默认的判断最大连接数失效，自己实现
    {
        QTcpSocket tcp;
        tcp.setSocketDescriptor(handle);
        tcp.disconnectFromHost();
        return;
    }
    TcpSocket *tcpTemp = new TcpSocket(this,handle);
    tcpTemp->setSocketDescriptor(handle);

    connect(tcpTemp,SIGNAL(RecieveData(int,QByteArray)),this,SLOT(ReadData(int,QByteArray)));
    connect(tcpTemp,SIGNAL(DisConnected(int,QString,int)),this,SLOT(DisConnected(int,QString,int)));
    connect(this,&TcpServer::sendDisConnect,tcpTemp,&TcpSocket::disConTcp);//断开信号
    tcpClients->insert(handle,tcpTemp);
    //存储当前连接
    //qDebug()<<handle<<"connected"<<QTime::currentTime();
    CurrentClient = tcpTemp;
    emit ClientConnected(handle, tcpTemp->peerAddress().toString(),tcpTemp->peerPort());
}

void TcpServer::ReadData(int clientID,QByteArray data)
{
    emit ClientReadData(clientID,data);
}

void TcpServer::DisConnected(int clientID,QString IP,int Port)
{
    //qDebug()<<clientID<<"unconnected"<<QTime::currentTime();
    tcpClients->remove(clientID);
    emit ClientDisConnected(clientID,IP,Port);
}

//指定客户端连接发消息
void TcpServer::SendData(int clientID,char *data,int len)
{
    auto t = tcpClients->find(clientID);
    if (t != tcpClients->end())
    {
        t.value()->write(data,len);
    }
}

//对当前连接发送数据
void TcpServer::SendDataCurrent(char* data,int len)
{
    //如果没有一个存在的连接，则不处理
    if (tcpClients->size() < 1) return;
    CurrentClient->write(data,len);
}

//对所有连接发送数据
void TcpServer::SendDataAll(char *data, int PackLen)
{
    QList<TcpSocket *>list = tcpClients->values();
    if(list.isEmpty()) return;
    for(auto t = list.begin();t!= list.end();t++)
    {
        int rv = 0;
        int len = 0;
        while(len < PackLen)
        {
            rv = (*t)->write(data+len,PackLen-len);
            if (rv<1)
            {
                qDebug()<<__LINE__<<"TcpServer::Send Data error;";
                break;
            }
            else
            {
                len+= rv;
                qDebug()<<"len:"<<len;
            }
        }
        if (len > 0)
        {
            qDebug()<<QString("TcpServer::Send Data %1 once;").arg(len);
        }
        (*t)->flush();
    }
}
void TcpServer::clear()
{
    emit this->sendDisConnect(-1);
    tcpClients->clear();
}
