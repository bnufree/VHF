#include "zchxvhfconvertthread.h"
#include <QDir>
#include "comtextaudiothread.h"
#include "quploadfilethread.h"
#include "inifile.h"
#include "zchxbroadcasttaskmgr.h"
#include <QApplication>
#include <QDebug>

zchxVhfConvertThread::zchxVhfConvertThread(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<BroadcastSetting>("const BroadcastSetting&");
    connect(this, SIGNAL(signalRecvBroadSetting(BroadcastSetting)), this, SLOT(slotRecvBroadSetting(BroadcastSetting)));
    connect(this, SIGNAL(signalRemoveBroadTask(int)), this, SLOT(slotRemoveBroadTask(int)));
    //开始上传到服务器开始自动播报
    QString strIP = IniFile::instance()->getVHFServerIP();
    int nPort = IniFile::instance()->getVHFServerPort().toInt();
    QString strUser = IniFile::instance()->getVHFUser();
    QString strPassword = IniFile::instance()->getVHFPwd();
    mUploadThread = new QUploadFileThread(strIP, nPort, strUser, strPassword ,this);
    connect(mUploadThread, SIGNAL(sigSendFileSuccess(BroadcastSetting)), this, SLOT(slotRecvUploadFileSuccess(BroadcastSetting)));

    mTaskMgr = new zchxBroadcastTaskMgr;
    connect(mTaskMgr, SIGNAL(signalSendBroadTaskList(TaskList)), this, SIGNAL(signalSendBroadTaskList(TaskList)));
    connect(mTaskMgr, SIGNAL(signalSendTotalBroadcastList(TaskList)), this, SIGNAL(signalSendTotalBroadcastList(TaskList)));

    this->moveToThread(&mWorkThread);
    mWorkThread.start();
    mUploadThread->start();
}

void zchxVhfConvertThread::slotRecvBroadSetting(const BroadcastSetting &setting)
{
    qDebug()<<"slot recv Task setting:"<<setting.mIsTry;
    BroadcastSetting job = setting;
    if(!job.mIsTry)
    {
        //完整的语音播报,需要生成本地的音频文件
        //检查语音文件目录
        QDir dir(QApplication::applicationDirPath() + QString("/zchx_audio"));
        if(!dir.exists())
        {
            dir.mkpath(dir.path());
        }
        //生成语音播放文件名
        QString filename = dir.path() + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".wav";
        job.mLocalAudioName = filename;
    }
    //开始生成语音,进行试听或者生成语音文件
    qDebug()<<"start make voice file:";
    ComTextAudioThread* thread = new ComTextAudioThread(job);
    connect(thread, SIGNAL(finished()), this, SLOT(slotAudioTaskfinished()));
    thread->start();
}

void zchxVhfConvertThread::slotAudioTaskfinished()
{
    ComTextAudioThread* thread = qobject_cast<ComTextAudioThread*>(sender());    
    qDebug()<<"audio task finished."<<thread;
    if(!thread) return;
    //如果是试听,那么结束,否则上传到服务器
    BroadcastSetting task = thread->task();
    qDebug()<<"audio task need upload? "<<(!task.mIsTry);
    if(!task.mIsTry)
    {
        //检查语音文件是否纯在
        QString fileName = task.mLocalAudioName;
        if(QFile::exists(fileName))
        {
            if(mUploadThread)
            {
                qDebug()<<"start upload now..";
                mUploadThread->addFile(task);  //添加文件发送
            } else
            {
                qDebug()<<"upload thread error:"<<mUploadThread;
            }
        } else
        {
            qDebug()<<"can not find audio local file:"<<fileName<<"  upload error";
        }
    }

    delete thread;
}

void zchxVhfConvertThread::slotRecvUploadFileSuccess(const BroadcastSetting& result)
{
    qDebug() << "upload success voice file name:" << result.mContent<<result.mLocalAudioName<<result.mFileName;
    if(!mTaskMgr) return;
    BroadcastSetting job = result;
    int nIndex = job.mFileName.indexOf(".");
    if(nIndex <= 0)
    {
        emit signalSendErrorMsg(QStringLiteral("PBX服务器返回的文件名异常:%1.播放任务终止").arg(job.mFileName));
        return;
    }
    QString result_file = job.mFileName.left(nIndex);
    qDebug()<<"server file name is:"<<result_file;
    //删除源文件
    QDir dir(QCoreApplication::applicationDirPath() + QString("/zchx_audio"));
    if(!dir.exists())
    {
        dir.mkpath(dir.path());
    }
    //获取当前目录下的文件数目,保留最近的10个数据
    QFileInfoList list = dir.entryInfoList(QStringList()<<("*.wav"), QDir::Files, QDir::Time | QDir::Reversed);
#if 0
    {
        for(int i=0; i<list.size();i++)
        {
            qDebug()<<"remove file :"<<list[i].fileName();
        }
    }
#endif
    while (list.size() >= 10)
    {
        QString fileName = list.takeFirst().fileName();
        bool sts = dir.remove(fileName);
        qDebug()<<"remove file :"<<fileName<<sts;

    }

    //开始生成播放任务
    job.mFileName = result_file;
    mTaskMgr->signalAppendTask(job);
}

void zchxVhfConvertThread::slotRemoveBroadTask(int id)
{
    if(mTaskMgr) mTaskMgr->removeTask(id);
}
