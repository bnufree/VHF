#ifndef TcpSocket_H
#define TcpSocket_H

#include <QTcpSocket>

class TcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit TcpSocket(QObject *parent = 0,int Id =0);
    int getId() { return m_nId; }
    bool operator ==(TcpSocket& );

signals:
    void DisConnected(int Id,QString IP,int Port);
    void Connected(int Id,QString IP,int Port);
	void RecieveData(int Id,QByteArray);
    void DisConnected(QString IP,int Port);
    void Connected(QString IP,int Port);
    void RecieveData(QByteArray);
	
public slots:
    void ReadData();
    void SendData(QByteArray);
    void DisConnected();
    void Connected();
    void disConTcp(int i);//respond slot from server disconect signal
	void SetServer(QString ip,int port);
	void ConnectServer(QString ip,int port);
    void recvError(QAbstractSocket::SocketError error);

private:
    int m_nId;
    QString mIp;
    int     mPort;
};
#endif // TcpSocket_H
