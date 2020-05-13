#include "controlwidget.h"
#include "ui_controlwidget.h"
#include "extension.h"
#include "relaycontroller.h"
#include "networker.h"
#include "inifile.h"
#include "centercontrolthread.h"
#include "zchxvhfaudioplaythread.h"
#include <QDir>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QDateTime>

extern bool mDebug;
extern bool mTest;

ControlWidget::ControlWidget(QFrame *parent, const ExtensionData extensionData) :
    QFrame(parent),
    mExtensionType(Extension_Unknown),
    mControlHostIP(""),
    mControlHostPort(0),
    mCenterControlThread(0),
    ui(new Ui::ControlWidget),
    mIsTxPressed(false),
    mIsGroup(false),
    mOpenCmdSourceID(""),
    relay_controller_(0),
    extension_(0),
    mAudioPlayThread(0)
{
    ui->setupUi(this);
    isRxEnable = true;
    mExtensionType = IniFile::instance()->ReadKeyValue(QString("%1/ExtensionType").arg(extensionData.number), "0").toInt();
    bool autoEnterMetting = (mExtensionType == Extension_Station_Broadcast ? false : true);
    if(mExtensionType == Extension_Station_Call || mExtensionType == Extension_Center)
    {
        autoEnterMetting = true;
        isRxEnable = true;
    } else
    {
        autoEnterMetting = false;
        isRxEnable = false;
    }

    extension_ = new Extension(autoEnterMetting, nullptr, extensionData);
    extension_->moveToThread(&ExtensionThread);
    connect(&ExtensionThread, &QThread::finished, extension_, &QObject::deleteLater);
    ExtensionThread.start();


    InitRelay();
    InitUI();
    initCenterControl();
    //推送分机状态变化事件
    connect(NetWorker::instance(), &NetWorker::signal_extension_status_changed,
            this, &ControlWidget::slot_extension_status_changed);  

    connect(extension_, &Extension::statusChanged, this, &ControlWidget::RefreshView);
    //是否显示测试话机
    if(!mTest)  ui->testBroadcastFrame->setVisible(false);
}

ControlWidget::~ControlWidget()
{
    disconnect(this);

    ExtensionThread.quit();
    ExtensionThread.wait();
//    if (extension_ != nullptr)
//    {
//        delete extension_;
//        extension_ = nullptr;
//    }

    RelayControllerThread.quit();
    RelayControllerThread.wait();
//    if (relay_controller_ != nullptr)
//    {
//        delete relay_controller_;
//        relay_controller_ = nullptr;
//    }

    if (extension_->isAutoEnter)       
        IniFile::instance()->WriteKeyValue(extension_->getData().number + "/AutoEnter", "1");
    else
        IniFile::instance()->WriteKeyValue(extension_->getData().number + "/AutoEnter", "0");

    delete ui;
}

void ControlWidget::InitRelay()
{
    //检查是不是当前绑定的中心,如果不是,不进行任何处理
    if(isCenter() && extension_->getData().number != IniFile::instance()->GetGroupCenter()) return;
    RelayData relayData;
    relayData.address = IniFile::instance()->ReadKeyValue(QString("%1/RelayAddress").arg(extension_->getData().number));
    if (relayData.address.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), extension_->getData().number + QStringLiteral("继电器IP地址无效，请检查配置文件config"));
        return;
    }
    relayData.port = IniFile::instance()->ReadKeyValue(QString("%1/RelayPort").arg(extension_->getData().number), "12345").toInt();
    relayData.channel = IniFile::instance()->ReadKeyValue(QString("%1/Channel").arg(extension_->getData().number), "10").toInt();
    relayData.autoCloseTime = IniFile::instance()->ReadKeyValue(QString("%1/Duration").arg(extension_->getData().number), "10").toInt();
    relayData.heartInterval = IniFile::instance()->getRelayRequestTimeInterval();
    //    qDebug()<<"replaydata:"<<relayData.address<<relayData.port<<relayData.channel;

    relay_controller_ = new RelayController(nullptr, relayData);
    relay_controller_->moveToThread(&RelayControllerThread);
    connect(&RelayControllerThread, &QThread::finished, relay_controller_, &QObject::deleteLater);
    connect(relay_controller_, &RelayController::signalStatusChanged, this, &ControlWidget::SlotRelayStatusChanged);
//    connect(relay_controller_, &RelayController::SignalConnectionChanged, this, &ControlWidget::RefreshView);
    RelayControllerThread.start();
}

void ControlWidget::initCenterControl()
{
    qDebug()<<GetExtension()->getData().username<<" isCenter:"<<isCenter();
    if(!isCenter()) return;

    mControlHostIP = IniFile::instance()->ReadKeyValue(QString("%1/ControlAddress").arg(extension_->getData().number)).trimmed();
    mControlHostPort = IniFile::instance()->ReadKeyValue(QString("%1/ControlPort").arg(extension_->getData().number), "12345").toInt();
    mControlRequestInterVal = IniFile::instance()->getRelayRequestTimeInterval();

    //如果当前客户端的中心和组的名称相同
    if(extension_->getData().number == IniFile::instance()->GetGroupCenter())
    {

        if((!mControlHostIP.isEmpty()) && mControlHostPort > 0 && mControlRequestInterVal > 0)
        {
            qDebug()<<"start control server connect"<<mControlHostIP<<mControlHostPort<<mControlRequestInterVal;
            mCenterControlThread = new CenterControlThread(mControlHostIP, mControlHostPort, mControlRequestInterVal);
            connect(mCenterControlThread, SIGNAL(signalRecvCmd(int, bool)), this, SLOT(slotRecvControlCmd(int, bool)));
            mCenterControlThread->start();
        } else
        {
            qDebug()<<"no control server connect";
        }
    }
}

void ControlWidget::slotRecvControlCmd(int cmd, bool sts)
{
    if(cmd == 1)
    {
        //打开继电器
        openRelayControl(/*IniFile::instance()->getMicrophoneMaxOpenSeconds()*/0);
        qDebug()<<"tell station to open now."<<this->mControlHostIP<<this->relay_controller_->data.address;
        emit signalOpenStation(GetExtensionNum());
    } else if(cmd == 0)
    {
        if(mIsTxPressed || isPlayAudio())
        {
            qDebug()<<"tx now is used. not process"<<extension_->getData().number;
            return;
        }
        closeRelayControl();
        emit signalCloseStation(GetExtensionNum(), sts);
    }
}

/**
 * @brief ControlWidget::InitUI 初始化显示内容与功能槽的连接
 */
void ControlWidget::InitUI()
{
    //Rx按键功能
    ui->rxButton->setIcon(QIcon(":/ico/vol"));
    connect(ui->rxButton, &QPushButton::clicked, this, &ControlWidget::OnRxButtonClicked);
    connect(ui->rxButton, SIGNAL(doubleClicked()),
            this, SLOT(OnRxButtonDoubleClicked()));
    ui->rxButton->setCheckable(true);
    ui->rxButton->setChecked(false);

    //Tx按键功能
    ui->txButton->setIcon(QIcon(":/ico/mic"));
    connect(ui->txButton, &QPushButton::pressed, this, &ControlWidget::OnTxButtonPressed);
    connect(ui->txButton, &QPushButton::pressed, this, &ControlWidget::SignalTxButtonPressed);
    connect(ui->txButton, &QPushButton::released, this, &ControlWidget::OnTxButtonReleased);
    connect(ui->txButton, &QPushButton::released, this, &ControlWidget::SignalTxButtonReleased);
    ui->txButton->setCheckable(true);
    ui->txButton->setChecked(false);

    //频道编辑
    connect(ui->channel_edit, &QLineEdit::editingFinished, this, &ControlWidget::OnChannelEdited);

    //用户名编辑
    connect(ui->username_edit, &QLineEdit::editingFinished, this, &ControlWidget::OnUsernameEdited);
    ui->edit_button->setIcon(QIcon(":/ico/edit"));
    connect(ui->edit_button, &QToolButton::clicked, [=](){
        ui->username_edit->setEnabled(true);});

    //自动接入会议室
    if(extension_->isMeetingAvailable)
    {
        connect(ui->autoEnterCheckBox, &QCheckBox::clicked, this, &ControlWidget::SlotAutoEnterCheckBoxToggled);
        if( IniFile::instance()->ReadKeyValue(QString("%1/AutoEnter").arg(extension_->getData().number), "0").toInt())
        {
            ui->autoEnterCheckBox->setChecked(true);
            extension_->isAutoEnter = true;
        }
        else
        {
            ui->autoEnterCheckBox->setChecked(false);
            extension_->isAutoEnter = false;
        }
    } else
    {
        ui->autoEnterCheckBox->setEnabled(false);
		//强制退出会议室
		extension_->hang_up();
    }
    //
    connect(ui->playBtn, SIGNAL(clicked(bool)), this, SLOT(slotPlayBtnclicked()));
    connect(ui->closeRelay, SIGNAL(clicked(bool)), this, SLOT(slotCloseRelay()));

	//测试的属性设定删除
    ui->extension_type->addItem(QStringLiteral("未知"), Extension_Unknown);
    ui->extension_type->addItem(QStringLiteral("中心"), Extension_Center);
    ui->extension_type->addItem(QStringLiteral("播报岸站"), Extension_Station_Broadcast);
    ui->extension_type->addItem(QStringLiteral("呼叫岸站"), Extension_Station_Call);
    ui->extension_type->setCurrentIndex(mExtensionType);
    ui->extension_type->setEditable(false);
    ui->extension_type->setEnabled(false);
    ui->propertyFrame->setVisible(false);
    ui->save->setVisible(false);
    connect(ui->save, SIGNAL(clicked(bool)), this, SLOT(slotSave()));
}

void ControlWidget::slotPlayBtnclicked()
{
#if 1
    QString title = QStringLiteral("现在是北京时间");
    ui->broadcastText->setText(title + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
#else
    ui->broadcastText->setText("test");
#endif
    if(ui->broadcastText->text().isEmpty()) return;
    emit signalPlayText(extension_->getData().number, ui->broadcastText->text());
}

/**
 * @brief ControlWidget::RefreshView 刷新文本与状态等可变化内容
 */
void ControlWidget::RefreshView()
{
    qDebug()<<"update extension display:"<<GetExtensionNum();
    //刷新文本显示
    ui->username_edit->setText(extension_->getData().username);


    if(relay_controller_)
    {
        //刷新频道
        if (ui->channel_edit->text().trimmed() != QString::number(relay_controller_->data.channel))
            ui->channel_edit->setText(QString::number(relay_controller_->data.channel));

        //检测继电器连接状态
        if (relay_controller_->isConnected)
            ui->txButton->setEnabled(true);
        else
            ui->txButton->setEnabled(false);
    } else
    {
        ui->txButton->setEnabled(true);
    }

    //检测分机状态
    int status = extension_->getStatus();
    qDebug()<<"extension:"<<GetExtensionNum()<<" ping :"<<extension_->isPingSuccess<<" status:"<<extension_->getStatusStr()<<status;
    if (extension_->isPingSuccess)
    {
        if (status == EXTENSION_STATUS::REGISTER || status == EXTENSION_STATUS::RINGING)
        {
            ui->rxButton->setChecked(false);
            ui->rxButton->setEnabled(true);

//            if (relay_controller_->isConnected)
//                if (!ui->txButton->isEnabled())
//                    ui->txButton->setEnabled(true);
        }
        else if (status == EXTENSION_STATUS::BUSY)
        {
            ui->rxButton->setEnabled(true);
            ui->rxButton->setChecked(true);
            ui->rxButton->setStyleSheet("");

//            if (relay_controller_->isConnected)
//                if (!ui->txButton->isEnabled())
//                    ui->txButton->setEnabled(true);
        }
        else
        {
            ui->rxButton->setStyleSheet("");
            ui->rxButton->setChecked(false);
            ui->rxButton->setEnabled(false);
            ui->txButton->setEnabled(false);
        }
    }
    else
    {
        ui->rxButton->setStyleSheet("");
        ui->rxButton->setChecked(false);
        ui->rxButton->setEnabled(false);
//        ui->txButton->setEnabled(false);
    }

//    ui->rxButton->setText(extension_->getData().status_str);
    if(mTest)
    {
        ui->testBroadcastFrame->setVisible(true);
    } else
    {
        ui->testBroadcastFrame->setVisible(false);
    }
}

/**
 * @brief ControlWidget::OnRxButtonClicked Rx按键点击事件
 */
void ControlWidget::OnRxButtonClicked()
{  
    if(mExtensionType == Extension_Station_Broadcast) return;
    if (!isRxEnable)
        return;

    isRxEnable = false;
    ui->rxButton->setEnabled(false);
//    ui->rxButton->setStyleSheet("background-color: rgba(0,192,0,1);");
    QTimer::singleShot(RX_ENABLE_TIMER, [&](){isRxEnable = true; ui->rxButton->setEnabled(true);});

    int error_num;
    if (ui->rxButton->isChecked())
    {
        qDebug()<<"OnRxButtonClicked is check."<<extension_->getData().number;
        error_num = extension_->dial_up_outto();
    }
    else
    {
        qDebug()<<"OnRxButtonClicked is not check."<<extension_->getData().number;
        error_num = extension_->hang_up();
    }

    if (error_num)
    {
        emit SignalOperationFailed(extension_->getData().number, error_num);
    }

    QTimer::singleShot(3000, [=](){emit to_refresh();});

}

void ControlWidget::OnRxButtonDoubleClicked()
{
    return;
    //网页操作需要登陆控制页面,暂且不做了
    if(mExtensionType == Extension_Station_Broadcast) return;
    //强制退出会议室
    if(!extension_) return;
    //http://192.168.100.51/cgi-bin/ConfigManApp.com?key=F4
    QString url = QString("http://%1/cgi-bin/ConfigManApp.com?key=F4").arg(extension_->getData().address);
    //执行http服务
    QNetworkAccessManager mgr;
    QNetworkReply *reply = mgr.get(QNetworkRequest(QUrl(url)));
    if(!reply)
    {
        qDebug()<<"send commoand f4 to extension failed. with url"<<url;
        return;
    }
    qDebug()<<"send commoand f4 to extension with url"<<url;
    QEventLoop eventloop;
    QTimer* timeout_timer = new QTimer(this);
    connect(timeout_timer, &QTimer::timeout, &eventloop, &QEventLoop::quit);
    connect(reply, SIGNAL(readyRead()), &eventloop, SLOT(quit()));
    timeout_timer->setSingleShot(true);
    timeout_timer->start(REQUEST_TIMEOUT);
    eventloop.exec();
    if(reply->isFinished() && reply->isReadable())
    {
        qDebug()<<"send command finished. with result:"<<QString::fromUtf8(reply->readAll());
    } else
    {
        qDebug()<<"send command with timeout...";
    }
    reply->deleteLater();
    timeout_timer->deleteLater();
}

void ControlWidget::OnTxButtonPressed()
{
    qDebug()<<"start set relay with 1";
    mIsTxPressed = true;
    if(!mIsGroup)
    {
        if(relay_controller_)relay_controller_->setLongPress(true);
        openRelayControl(0);
    } else
    {
        ui->txButton->setChecked(true);
    }

//    ui->txButton->setStyleSheet("QPushButton:pressed{background-color: #00c000}");
//    ui->txButton->setDown(true);
}

void ControlWidget::playAudio(const QString &name, bool async)
{
    if(!isRxAvailable())
    {
        qDebug()<<"rx is unavailable. play audio failed"<<GetExtensionNum();
        emit signalPlayAudioTextFailed(GetExtension()->getData().username + QStringLiteral("话机现在离线,语音播放失败"));
        return;
    }
    //检查当前是否正在播放,如果是直接退出
    if(mAudioPlayThread) return;
    //开启播放线程
    mAudioPlayThread = new zchxVHFAudioPlayThread(name);
    mAudioPlayThread->setExtension(extension_);
    mAudioPlayThread->setRelayController(relay_controller_);
    connect(mAudioPlayThread, SIGNAL(finished()), this, SLOT(slotFinishedAudioPlay()));
    //停止话机和继电器的自动接入
    if(extension_) extension_->isPlayingAudio = true;
    if(relay_controller_) relay_controller_->isPlayingAudio = true;
    //开始播放
    mAudioPlayThread->start();
}

void ControlWidget::OnTxButtonReleased()
{
    qDebug()<<"tx button release now"<<extension_->getData().number;
    mIsTxPressed = false;
    if(!mIsGroup)
    {
        if(relay_controller_) relay_controller_->setLongPress(false);
        closeRelayControl();
    } else
    {
        ui->txButton->setChecked(false);
    }
}

void ControlWidget::openRelayControl(int duration)
{
    if(!relay_controller_) return;
    qDebug()<<"wait relay open:"<<GetExtensionNum();    
    relay_controller_->send_set_status_command(1, duration);
}

void ControlWidget::closeRelayControl()
{
    if(!relay_controller_) return;
    qDebug()<<extension_->getData().number<<"tx isPressed:"<<mIsTxPressed;
    if(!(mIsTxPressed || isPlayAudio()))
    {
        qDebug()<<"tx not press now  process close relay "<<extension_->getData().number;
        relay_controller_->send_set_status_command(0, 0);
    } else
    {
        qDebug()<<"tx press now .not process close relay"<<extension_->getData().number;
    }
}

/**
 * @brief ControlWidget::OnChannelEdited CH文本编辑事件
 */
void ControlWidget::OnChannelEdited()
{
    bool ok = false;
    QString new_channel = ui->channel_edit->text().trimmed();
    int channel = new_channel.toInt(&ok, 10);
    if (ok && channel>=0)
    {
        if(relay_controller_)relay_controller_->data.channel = channel;
        IniFile::instance()->WriteKeyValue(extension_->getData().number + "/Channel", new_channel);
    }
    else
    {
        QMessageBox::critical(this, QStringLiteral("提示"), QStringLiteral("请输入正整数！"));
        ui->channel_edit->setText(IniFile::instance()->ReadKeyValue(extension_->getData().number + "/Channel"));
    }
}

/**
 * @brief ControlWidget::OnUsernameEdited 用户名编辑事件
 */
void ControlWidget::OnUsernameEdited()
{
    QString new_username = ui->username_edit->text().trimmed();
    int error_num = extension_->update_username(new_username);
    if (error_num)
    {
        emit SignalOperationFailed(extension_->getData().number, error_num);
    }

    ui->username_edit->setEnabled(false);
}

/**
 * @brief ControlWidget::slot_extension_status_changed 分机状态变化事件
 * @param extension_num 分机号
 * @param status 变化后状态
 */
void ControlWidget::slot_extension_status_changed(const QString extension_num, const QString status)
{
    qDebug()<<"recv exten status:"<<extension_num<<status;
    if (extension_num == extension_->getData().number)
    {
        qDebug() << extension_num << status;
        ui->status->setText(status);
        extension_->setStatus(status);
    }

    RefreshView();
}

bool ControlWidget::isRxAvailable() const
{
    if(!extension_) return false;
    return extension_->getStatus() != UNAVAILABLE;
}

/**
 * @brief ControlWidget::SlotGroupRxButtonClicked 组呼控件Rx按键点击事件
 * @param isChecked 是否选中
 */
void ControlWidget::SlotGroupRxButtonClicked(bool isChecked)
{
    if (isChecked != ui->rxButton->isChecked())
    {
        ui->rxButton->click();
    }
}

/**
 * @brief ControlWidget::SlotGroupTxButtonPressedOrReleased 组呼控件Tx按键按下或松开事件
 * @param isDown 是否按下
 */
void ControlWidget::SlotGroupTxButtonPressedOrReleased(bool isDown)
{
    if (ui->txButton->isEnabled())
    {
        ui->txButton->setDown(isDown);
        if (isDown)
            emit ui->txButton->pressed();
        else
            emit ui->txButton->released();
    }
}

void ControlWidget::SlotTimeout()
{
    extension_->setStatus("Unavailable");
}

void ControlWidget::SlotTxButtonPressed()
{
    qDebug()<<"notify to  press tx";
//    if (!ui->txButton->isEnabled())
//        return;
    ui->txButton->setDown(true);
    emit ui->txButton->pressed();
    mIsTxPressed = true;
}

void ControlWidget::SlotTxButtonReleased()
{
    qDebug()<<"notify to  release tx";
//    if (!ui->txButton->isEnabled())
//        return;
    ui->txButton->setDown(false);
    emit ui->txButton->released();
    mIsTxPressed = false;
    qDebug()<<"tx button release now"<<extension_->getData().number;
}

void ControlWidget::SlotAutoEnterCheckBoxToggled()
{
    bool checked = ui->autoEnterCheckBox->isChecked();
    extension_->isAutoEnter = checked;
    extension_->autoEnter();
}

void ControlWidget::SlotRelayStatusChanged(RelayControllerStatus sts)
{
    if(mDebug)  qDebug()<<"relay status changed:"<<sts<<" tx press:"<<mIsTxPressed<<extension_->getData().number;
    if(sts == RelayControllerStatus::Status_Unavailable)
    {
        if(mDebug)  qDebug()<<"relay status unavailable";
        ui->txButton->setChecked(false);
        ui->txButton->setEnabled(false);
    } else if(sts == RelayControllerStatus::Status_On)
    {
        if(mDebug)  qDebug()<<"relay status on";
        ui->txButton->setEnabled(true);
        ui->txButton->setChecked(true);
    } else if(sts == RelayControllerStatus::Status_Off)
    {
        if(mDebug)  qDebug()<<"relay status off";
        ui->txButton->setChecked(false);
        ui->txButton->setEnabled(true);
        if(!mOpenCmdSourceID.isEmpty()) mOpenCmdSourceID.clear();
    }
}

void ControlWidget::slotCloseRelay()
{
    if(relay_controller_) relay_controller_->send_set_status_command(0,0);
}

void ControlWidget::slotFinishedAudioPlay()
{
    if(mAudioPlayThread)
    {
        delete mAudioPlayThread;
        mAudioPlayThread = 0;
    }
    if(extension_)
    {
        extension_->isPlayingAudio = false;
        if(extension_->getData().preStatus == EXTENSION_STATUS::BUSY)
        {
            if(extension_->isMeetingAvailable)
            {
                //重新进入会议室
                extension_->dial_up_outto();
            }
            extension_->setPreStatus(EXTENSION_STATUS::UNAVAILABLE);
        }
    }

    if(relay_controller_)
    {
        relay_controller_->isPlayingAudio = false;
    }
}

bool ControlWidget::isPlayAudio() const
{
    return mAudioPlayThread != 0;
}

void ControlWidget::slotSave()
{
    qDebug()<<ui->extension_type->currentText()<<ui->extension_type->currentData().toInt();
    IniFile::instance()->WriteKeyValue(QString("%1/ExtensionType").arg(extension_->getData().number),  QString::number(ui->extension_type->currentData().toInt()));
    emit signalRestart();
}
