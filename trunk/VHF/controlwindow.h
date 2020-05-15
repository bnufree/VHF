#ifndef CONTROLWINDOW_H
#define CONTROLWINDOW_H

#include "controlwidget.h"
#include "groupcontrolwidget.h"
#include "inifile.h"
#include <QMainWindow>
#include <QPushButton>
#include <QMap>
#include <QListWidget>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCloseEvent>
#include <QStandardItemModel>
#include "javasubinfothread.h"
#include "vhfdatadefines.h"


namespace Ui {
class ControlWindow;
}

class zchxDataAsyncWorker;
class QTableWidgetItem;
class JavaSubInfoThread;
class zchxVhfConvertThread;

class ControlWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ControlWindow(QWidget *parent = nullptr);
    ~ControlWindow();

private slots:
    void saveTemplateContent();
public slots:
    void slotOpenTest();
    void Refresh();
	void on_DeleteBtn_clicked();
    void on_ChangeBtn_clicked();
    void on_SendBtn_clicked();
    void SlotOperationFailed(QString usrname, int error_num);
    void slot_heartbeat_timeup(bool sts, const QString& msg);
    void BroadCastTypeChanged(int id);
    void setPlanControlsVisible(bool sts);
    void slotRecvTotalBroadTaskList(const TaskList& future);
    void slotRecvBroadTaskJobList(const TaskList& now );
    void slotRecvJavaInfo(const JavaInfo& info);
    void slotRecvErrorMsg(const QString& msg);
    void slotOpenStation(const QString& source);
    void slotCloseStation(const QString& source, bool sts);
    void slotPlayTextAudio(const QString& id, const QString& text);
    void slotRestartApp();
    void slotRecvExtensionList(const ExtensionDataList& list,int code);

protected:
    virtual void closeEvent(QCloseEvent* event);

    int         messageBox(const QString& msg, bool cancel );
private slots:
    void on_play_clicked();

    void on_im_broad_btn_clicked(bool checked);

    void on_plan_broad_btn_clicked(bool checked);

    void on_task_del_clicked();

    void on_tableWidget_itemClicked(QTableWidgetItem *item);

private:
    void CreatView();
    void AddSingleWidget(const ExtensionDataList& list);
    void DeleteWidget();
    void AddGroupWidget();
    void UpdateStatus(const ExtensionDataList& list);
    //int  TextToVideo(const int volume, const WCHAR* wChar, const LPCWSTR &filename);
    void PromptVideo(const QStringList& phone_list, const QString& filename);
    QStringList getBroadCastExtensions(bool check = true);

private:
    Ui::ControlWindow *ui;
    QPushButton* refresh_button;
    QListWidget* listWidget;
    QMap<QString, ControlWidget*> singleControlMap;
    GroupControlWidget* groupControlWidget = nullptr;
    int                                 mFileTimeLen;
    QStandardItemModel*                 m_pExtensionModel; //分机列表树形结构
//    zchxDataAsyncWorker*                mDataAsyncWorker;
    JavaSubInfoThread*                  mJavaInfoThread;
    zchxVhfConvertThread*               mTaskMgrCenter;
};

#endif // CONTROLWINDOW_H
