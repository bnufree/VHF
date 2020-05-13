#ifndef QUPLOADFILETHREAD_H
#define QUPLOADFILETHREAD_H

#include <QThread>
#include <QString>
#include <QList>
#include <QMutex>
#include "vhfdatadefines.h"

class YeastarNetWork;

class QUploadFileThread : public QThread
{
    Q_OBJECT

public:
    explicit QUploadFileThread(QString strIp, int nPort, QString name, QString strPassword, QObject *parent = nullptr);
    virtual ~QUploadFileThread();

    void addFile(const BroadcastSetting& setting);


    void finishThread();

//public slots:
    //void start(Priority = InheritPriority);
signals:
    void      sigResponse(const QString& recv);
    void      sigSendStatus(const QString& sts);
    void      sigSendFileFaild(const QString& strFileName);
    void      sigSendFileSuccess(const BroadcastSetting& setting);
protected:
    virtual void run();
protected slots:
    void slotResponse(const QString& recv);
private:
    YeastarNetWork*  getVHFServer();
private:
    bool                    m_bFInish;
    QList<BroadcastSetting>          m_listFile;
    QMutex                  m_lock;
    QString                 m_strName;
    QString                 m_strPassword;
    QString                 m_strIP;
    int                     m_nPort;
};

#endif // QUPLOADFILETHREAD_H
