#include "zchxvhfaudioplaythread.h"
#include <QDebug>
#include <QDateTime>
#include "networker.h"

zchxVHFAudioPlayThread::zchxVHFAudioPlayThread(const QString& fileName, QObject *parent) : QThread(parent)
{
    mAudioFileName = fileName;
    mExtension = 0;
    mRelayController = 0;
}

void zchxVHFAudioPlayThread::run()
{
    if(mAudioFileName.isEmpty())
    {
        qDebug()<<"audio file name is empty. play return.";
        return;
    }
    //先退出会议室
    qDebug()<<"extension && relay:"<<mExtension<<mRelayController;
    if(!mExtension) return;
    if(!mRelayController) return;

    qDebug()<<"wait for play audio fila:"<<mAudioFileName<<" at extension num:"<<mExtension->getData().number;

    if(mExtension->getStatus()== EXTENSION_STATUS::BUSY)
    {\
        qDebug()<<" extension is busy now. exit metting room first";
        mExtension->setPreStatus(EXTENSION_STATUS::BUSY);
        mExtension->hang_up();
        //等待话机的状态变为空闲
        while (mExtension->getStatus() != EXTENSION_STATUS::REGISTER) {
            msleep(500);
        }
        qDebug()<<" extension is idle now. exit metting room finished";
    } else if(mExtension->getStatus() == EXTENSION_STATUS::UNAVAILABLE)
    {
        qDebug()<<" extension is unavailable. return";
        return;
    } else
    {
        qDebug()<<"extension is idle now.";
    }
    //现在开始打开对应的继电器
    mRelayController->stopResendTimer();
    mRelayController->send_set_status_command(1, 0);
    //检查继电器的状态是否为ON.超时60s默认已经打开继电器
    qint64 t = QDateTime::currentSecsSinceEpoch();
    while (1) {
        if(mRelayController->getStatus() == RelayControllerStatus::Status_On)
        {
            qDebug()<<" relay is on ";
            break;
        }
        if(QDateTime::currentSecsSinceEpoch() - t >= 30)
        {
            qDebug()<<" relay is off but now is time out(60s). defautly on ";
            break;
        }
        msleep(500);
    }
    //继电器已经打开,开始异步播放
    qDebug()<<"relay is normal, wait pbx ring"<<mExtension->getData().number<<mAudioFileName;
    bool waite_pbx_play = false;
    bool pbx_is_playing = false;
    NetWorker::instance()->signalPrompt(mExtension->getData().number, mAudioFileName);
    //检查话机的状态
    t = QDateTime::currentSecsSinceEpoch();
    bool status_change = false;
    int  playend = 0;
    while (1) {
        QString number = mExtension->getData().number;
        NetWorker::instance()->signalQueryExtensionStatus(number);
        int status = mExtension->getStatus();
        if(status == EXTENSION_STATUS::BUSY)
        {
            if(!status_change)
            {
                status_change = true;
                qDebug()<<"now playing audio."<<mExtension->getData().number<<mAudioFileName;
            }
            playend = 0;
        }
        if(status_change && status == EXTENSION_STATUS::REGISTER)
        {
            playend++;
            if(playend >= 3)
            {
                qDebug()<<"play audio finished."<<mExtension->getData().number<<mAudioFileName;
                break;
            }
        }

        if(QDateTime::currentSecsSinceEpoch() - t >= 30)
        {
            qDebug()<<" pbx play audio time out(30s). ";
            mExtension->hang_up();
            break;
        }
        msleep(2000);

    }
    //最后关闭继电器
    t = QDateTime::currentSecsSinceEpoch();
    mRelayController->send_set_status_command(0, 0);
    while (1) {
        if(mRelayController->getStatus() == RelayControllerStatus::Status_Off)
        {
            qDebug()<<" replay is off ";
            break;
        }
        if(QDateTime::currentSecsSinceEpoch() - t >= 30)
        {
            qDebug()<<" replay is not off but now is time out(30s). defautly on ";
            mRelayController->send_set_status_command(0, 0);
            break;
        }
        msleep(500);
    }
    qDebug()<<" pbx play audio finished "<<mExtension->getData().number<<mAudioFileName;

}
