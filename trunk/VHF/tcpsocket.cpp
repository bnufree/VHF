
#include "tcpsocket.h"
#include <QHostAddress>
#ifdef _WINDOWS
#include <windows.h>
#elif _UNIX
#include <unistd.h>
#endif

extern      bool mDebug;
TcpSocket::TcpSocket(QObject *parent,int Id) :
    QTcpSocket(parent)
{
    connect(this,SIGNAL(readyRead()),this,SLOT(ReadData()));
    connect(this,SIGNAL(disconnected()),this,SLOT(DisConnected()));
    connect(this,SIGNAL(connected()),this,SLOT(Connected()));
    connect(this,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(recvError(QAbstractSocket::SocketError)));
    m_nId = Id;
}
bool TcpSocket::operator ==(TcpSocket& s)
{
    if(m_nId == s.m_nId)
        return true;
    else
        return false;
}

void TcpSocket::recvError(QAbstractSocket::SocketError error)
{
    qDebug()<<"recv socket error:"<<error<<this->errorString()<<mIp<<mPort;
    DisConnected();
}

void TcpSocket::ReadData()
{
    QByteArray data = this->readAll();
    if(mDebug)  qDebug()<<"recv data from server:"<<QString::fromLatin1(data)<<mIp<<mPort;
    if(m_nId != 0)
        emit RecieveData(m_nId,data);
    else
        emit RecieveData(data);
}
void TcpSocket::SendData(QByteArray data)
{
    if(this->state() == QAbstractSocket::ConnectedState)
    {
        if(mDebug)  qDebug()<<"TcpSocket::SendData size:"<<data.size()<<data<<mIp<<mPort;
        this->write(data);
        this->flush();
    }
}

void TcpSocket::DisConnected()
{   
    qDebug()<<"disconnect now."<<mIp<<mPort;
    if(m_nId != 0)
    {
        emit DisConnected(m_nId,this->peerAddress().toString(),this->peerPort());
    }
    else
    {
        emit DisConnected(this->peerAddress().toString(),this->peerPort());       
    }
}
void TcpSocket::Connected()
{
    qDebug()<<"socket connected"<<mIp<<mPort;
    if(m_nId != 0)
        emit Connected(m_nId,this->peerAddress().toString(),this->peerPort());
    else
        emit Connected(this->peerAddress().toString(),this->peerPort());
}

void TcpSocket::disConTcp(int i)
{
    if ((i == m_nId) || (i == -1&&m_nId>0))
    {
        this->disconnectFromHost();
        this->deleteLater();
    }
}
void TcpSocket::ConnectServer(QString ip,int port)
{
    qDebug()<<"connect host:"<<mIp<<mPort;
	this->connectToHost(ip,port);
#ifdef _WINDOWS
    Sleep(10);
#elif _UNIX
    sleep(1);
#endif
}

void TcpSocket::SetServer(QString ip,int port)
{
    mIp = ip;
    mPort = port;
    if((this->state()==QAbstractSocket::ConnectedState)
        ||(this->state()==QAbstractSocket::ConnectingState))
	{
        return;
    }
    else
    {
        this->abort();
        ConnectServer(ip,port);
    }
}
