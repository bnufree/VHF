#ifndef CONTROLWIDGET_H
#define CONTROLWIDGET_H
#include "extension.h"
#include "relaycontroller.h"

#include <QFrame>
#include <QJsonObject>
#include <QDebug>
#include <QThread>
#include <QMessageBox>

#define RELAY_RECONNECT_TIMER 60*1000
#define AUTO_ENTER_TIMER 5*1000
#define RX_ENABLE_TIMER 1500

class Extension;
class RelayController;
class QTimer;
class QCheckBox;
class CenterControlThread;
class zchxVHFAudioPlayThread;

namespace Ui {
class ControlWidget;
}

enum  ExtensionType{
    Extension_Unknown = 0,
    Extension_Center = 1,
    Extension_Station_Broadcast,        //播报岸站
    Extension_Station_Call,
    Extension_Reserved,
};

class ControlWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ControlWidget(QFrame *parent = nullptr,const ExtensionData extensionData = ExtensionData());
    ~ControlWidget();
    Extension* GetExtension()
    {
        return extension_;
    }
    RelayController* GetRelayController()
    {
        return relay_controller_;
    }

    void InitRelay();
    void InitUI();
    void RefreshView();
    void RefreshRxStatus();
    void RefreshTxStatus();
    bool isCenter() const {return mExtensionType == Extension_Center;}
    bool isBroadcastStation() const {return mExtensionType == Extension_Station_Broadcast;}
    void initCenterControl();
    bool isTxPressed() const {return mIsTxPressed;}
    void setGroup(bool sts) {mIsGroup = sts;}
    bool isGroup() const {return mIsGroup;}
    void playAudio(const QString& name, bool async = false);  //同步播放还是异步播放,异步的情况自动关闭继电器
    bool isPlayAudio() const/* {return mWaitRelayReady || mWaitPBXPlay || mPlayAudioING;}*/;

    QString GetExtensionNum() {return GetExtension()->getData().number;}

    void setCmdSourceID(const QString& id) {mOpenCmdSourceID = id;}
    QString getCmdSourceID() const {return mOpenCmdSourceID;}
    //获取RX的状态.
    bool isRxAvailable() const;



public slots:
    void OnRxButtonClicked();
    void OnRxButtonDoubleClicked();
    void OnTxButtonPressed();
    void OnTxButtonReleased();
    void OnChannelEdited();
    void OnUsernameEdited();
    void slot_extension_status_changed(const QString extension_num, const QString status);
    void SlotGroupRxButtonClicked(bool isChecked);
    void SlotGroupTxButtonPressedOrReleased(bool isDown);
    void SlotTimeout();
    void SlotTxButtonPressed();
    void SlotTxButtonReleased();
    void SlotAutoEnterCheckBoxToggled();
    void SlotRelayStatusChanged(RelayControllerStatus sts);
    void slotRecvControlCmd(int cmd, bool sts);
    void slotPlayBtnclicked();
    void slotFinishedAudioPlay();
    void slotCloseRelay();    
    void slotSave();

public slots:
    void openRelayControl(int duration);
    void closeRelayControl();


signals:
    void SignalOperationFailed(QString extension_num, int error_num);
    void SignalTxButtonPressed();
    void SignalTxButtonReleased();
    void to_refresh();
    void signalOpenStation(const QString& source);
    void signalCloseStation(const QString& source, bool sts);
    void signalPlayText(const QString& id, const QString& text);
    void signalPlayAudioTextFailed(const QString& errMsg);
    void signalRestart();

private:
    Ui::ControlWidget *ui;
    Extension* extension_;
    QThread ExtensionThread;
    RelayController* relay_controller_;
    QThread RelayControllerThread;
    bool isRxEnable;    //Rx按键可反应标志
    //新添加的功能,分中心需要外部控制器来控制继电器的开关
    int     mExtensionType;
    QString mControlHostIP;
    int     mControlHostPort;
    int     mControlRequestInterVal;
    CenterControlThread* mCenterControlThread;
    bool    mIsTxPressed;
    bool    mIsGroup;
    QString  mOpenCmdSourceID;
    zchxVHFAudioPlayThread *mAudioPlayThread;
    QString mSec;
};

#endif // CONTROLWIDGET_H
