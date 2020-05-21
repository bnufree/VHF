#include "controlwindow.h"
#include "ui_controlwindow.h"
#include "networker.h"
#include <QDateTime>
#include <QStandardItem>
#include <QThread>
#include "quploadfilethread.h"
#include <QCoreApplication>
#include <QDir>
//#include "zchxdataasyncworker.h"
#include <QButtonGroup>
#include "zchxvhfconvertthread.h"
#include <QShortcut>
#include <QProcess>
#include "watchdogthread.h"
#include <QMessageBox>


#define MIN_TIME 60*1000
extern  bool        mDebug;
bool                mTest = false;

ControlWindow::ControlWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ControlWindow),
    mTaskMgrCenter(nullptr),
    mJavaInfoThread(nullptr)
{
    ui->setupUi(this);
    ui->im_broad_btn->setChecked(true);
    setPlanControlsVisible(false);
    connect(NetWorker::instance(), SIGNAL(signalSendExtensionList(ExtensionDataList,int)),
            this, SLOT(slotRecvExtensionList(ExtensionDataList,int)));

    NetWorker::instance()->startHeart();

    //界面初始化
    CreatView();

    mTaskMgrCenter = new zchxVhfConvertThread;
    connect(mTaskMgrCenter, SIGNAL(signalSendBroadTaskList(TaskList)), this, SLOT(slotRecvBroadTaskJobList(TaskList)));
    connect(mTaskMgrCenter, SIGNAL(signalSendTotalBroadcastList(TaskList)), this, SLOT(slotRecvTotalBroadTaskList(TaskList)));
    connect(mTaskMgrCenter, SIGNAL(signalSendErrorMsg(QString)), this, SLOT(slotRecvErrorMsg(QString)));

    if(IniFile::instance()->getJavaEnable())
    {
        mJavaInfoThread = new JavaSubInfoThread(IniFile::instance()->getJavaHost(),
                                                IniFile::instance()->getJavaTopic(),
                                                IniFile::instance()->getJavaPort());
        connect(mJavaInfoThread, SIGNAL(signalSendJavaSubInfo(JavaInfo)), this, SLOT(slotRecvJavaInfo(JavaInfo)));
        mJavaInfoThread->start();
    }

    QShortcut *shortCut = new QShortcut(QKeySequence(tr("Ctrl+T")), this);
    connect(shortCut, SIGNAL(activated()), this, SLOT(slotOpenTest()));

    //开启监控线程
    WatchDogThread* watch = new WatchDogThread(this);
    watch->start();
}

ControlWindow::~ControlWindow()
{
    if(mTaskMgrCenter) delete mTaskMgrCenter;
    if(mJavaInfoThread) delete mJavaInfoThread;

    DeleteWidget();

    if (nullptr != groupControlWidget)
    {
        delete  groupControlWidget;
        groupControlWidget = nullptr;
    }

    delete ui;
}


void ControlWindow::CreatView()
{
    this->setWindowIcon(QIcon(":/ico/zchx"));
    ui->task_start_time->setDateTime(QDateTime::currentDateTime());
    //状态栏添加刷新按钮
    refresh_button = new QPushButton(QStringLiteral("刷新"));
    connect(refresh_button, &QPushButton::clicked, this, &ControlWindow::Refresh);
    statusBar()->addPermanentWidget(refresh_button);
    refresh_button->setFont(QFont("Airal",15));

    //tab界面初始化
    ui->tab->setStyleSheet("background-color: #f3f3f3;");
    connect(ui->groupTabWidget, &QTabWidget::currentChanged, [=](){
        if (0 != ui->groupTabWidget->currentIndex())
            refresh_button->hide();
        else
            refresh_button->show();
    });

    //增加组界面
    QString groupName = IniFile::instance()->GetGroupName();
    listWidget = new QListWidget(this);
    listWidget->setViewMode(QListWidget::IconMode);      //设置显示模式为图标模式
    listWidget->setLayoutMode(QListWidget::SinglePass);  //设置为一次性布局
    listWidget->setResizeMode(QListWidget::Adjust);      //设置自动适应布局调整 Adjust适应,Fixed不适应
    listWidget->setMovement(QListWidget::Static);        //设置QListWidget中的单元项不可被拖动
    listWidget->setStyleSheet("QListWidget\n{\n	background-color: #f3f3f3;\n}\nQFrame#ControlWidget\n{\n	margin:20px 20px 20px 20px;\n}");
    ui->groupTabWidget->insertTab(0, listWidget, QStringLiteral("主界面"));
    ui->groupTabWidget->setCurrentIndex(0);

    // 设置QTableWidget样式 文字转语音的表格,默认设定15种语音
    ui->qTableWidget->setColumnCount(1);
    ui->qTableWidget->setRowCount(15);
    ui->qTableWidget->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("播报内容")); // 添加横向表头
    ui->qTableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color:lightblue; padding-left:4px; border:1px solid #6c6c6c;}");
    ui->qTableWidget->verticalHeader()->setStyleSheet("QHeaderView::section {background-color:skyblue; padding-left: 4px; border:1px solid #6c6c6c}");
    ui->qTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->qTableWidget->horizontalHeader()->setStretchLastSection(true);
    //ui->qTableWidget->verticalHeader()->setStretchLastSection(true);
    //ui->qTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for (int i=0; i<ui->qTableWidget->rowCount(); i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        ui->qTableWidget->setItem(i,0,item);
        if (i%2 == 0)
        {            
            item->setBackgroundColor(QColor(230,220,210));            
        }
        //读取配置文件的内容
        QString str = IniFile::instance()->ReadKeyValue(QString("Broadcast/row%1").arg(i));
        if (!str.isEmpty())
        {
            ui->qTableWidget->item(i,0)->setText(str);
        }
    }
    //初始化播放列表
    ui->tableWidget->setColumnCount(1);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("播报内容")); // 添加横向表头
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color:lightblue; padding-left:4px; border:1px solid #6c6c6c;}");
    ui->tableWidget->verticalHeader()->setStyleSheet("QHeaderView::section {background-color:skyblue; padding-left: 4px; border:1px solid #6c6c6c}");
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    //ui->tableWidget->verticalHeader()->setStretchLastSection(true);
    //ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->task_del->setVisible(false);

    m_pExtensionModel = new QStandardItemModel(this);
    ui->treeView->setModel(m_pExtensionModel);
    m_pExtensionModel->setColumnCount(1);
    QStringList listHead;
    listHead <<QStringLiteral("岸站名称");
    m_pExtensionModel->setHorizontalHeaderLabels(listHead);
    NetWorker::instance()->signalQueryExtension();
}

void ControlWindow::slotRecvExtensionList(const ExtensionDataList& list,int code)
{
    qDebug()<<"recv extensiion:"<<list.size();
    if (code == Extension_Query_TimeOut)
    {
        this->statusBar()->showMessage(QStringLiteral("接收岸站分机数据超时"));
        return;
    } else if(code == Extension_Query_Failed)
    {
        this->statusBar()->showMessage(QStringLiteral("岸站分机数据查询状态：失败"));
        return;
    } else {
        this->statusBar()->showMessage(QStringLiteral("岸站分机数据更新"));
    }


    //添加单控制控件
    if (singleControlMap.size() != list.size())
    {
        DeleteWidget();
        AddSingleWidget(list);
        //添加分组控制控件
        if (nullptr == groupControlWidget)   AddGroupWidget();

        //添加分机与指挥中心控件的关联
        QString centerNum = IniFile::instance()->GetGroupCenter();
        if (centerNum.isEmpty())
        {
            QMessageBox::critical(this, QStringLiteral("提示"), QStringLiteral("请先设置好指挥中心，请重启软件！"));
            return;
        }

        ControlWidget* centerControlWidget = singleControlMap.value(centerNum);
        for (int i = 0; i < singleControlMap.size(); i++)
        {
            if (singleControlMap.keys().at(i) == centerNum)
                continue;
            ControlWidget* controlWidget = singleControlMap.values().at(i);
            connect(controlWidget, &ControlWidget::SignalTxButtonPressed, centerControlWidget, &ControlWidget::SlotTxButtonPressed);
            connect(controlWidget, &ControlWidget::SignalTxButtonReleased, centerControlWidget, &ControlWidget::SlotTxButtonReleased);
        }
    }
    else
    {
        UpdateStatus(list);
    }


    //更新岸站列表数据
    QStringList keys;
    foreach (ControlWidget* w, singleControlMap) {
        //这里只是添加播报岸站的数据
        if(!w || !(w->isBroadcastStation()))
        {
            continue;
        }
        keys.append(w->GetExtension()->getData().number);
    }
    int nCount = keys.size();
    if(nCount > 0)
    {
        //获取已经添加的岸站数据
        int old_count = m_pExtensionModel->rowCount();
        QStringList idList;
        for (int i=0; i<old_count; i++) {
            QStandardItem *item = m_pExtensionModel->item(i, 0);
            if(item) idList.append(item->data().toString());
        }
        for(int i = 0; i < nCount; ++i)
        {
            QString strID = keys[i];
            ControlWidget *w = singleControlMap.value(strID);
            if(!w) continue;
            QString name = w->GetExtension()->getData().username;
            if(!strID.isEmpty())
            {
                if(name.isEmpty()) name = strID;
                if(idList.contains(strID))
                {
                    idList.removeOne(strID);
                    continue;
                }
                int RowNo = m_pExtensionModel->rowCount();
                QStandardItem* pItem =  new QStandardItem(name);
                pItem->setCheckable(true);
                pItem->setCheckState(Qt::Checked);
                m_pExtensionModel->setItem(RowNo, 0, pItem);
                pItem->setData(strID);
            }
        }
        //删除没有更新的数据
        QList<QStandardItem*> itemlist;
        for (int i=0; i<m_pExtensionModel->rowCount();) {
            QStandardItem *item = m_pExtensionModel->item(i, 0);
            if(item)
            {
                if(idList.contains(item->data().toString()))
                {
                    m_pExtensionModel->removeRow(i);
                    continue;
                }
            }
            i++;
        }

    } else {
        m_pExtensionModel->clear();
    }
}

void ControlWindow::UpdateStatus(const ExtensionDataList& list)
{
    //添加单控制控件
    for(int i = 0; i < list.size(); i++)
    {
        ExtensionData obj = list[i];
        QString targetNum = obj.number;
        if (!singleControlMap.contains(targetNum)) continue;
        ControlWidget* control_widget = singleControlMap.value(targetNum);
        QString status = obj.status_str;
        control_widget->GetExtension()->setStatus(status);
        control_widget->GetExtension()->setUserName(obj.username);
        control_widget->RefreshView();
    }    
}

void ControlWindow::AddSingleWidget(const ExtensionDataList& list)
{
    int BusyNum = 0;
    for (int i=0; i<list.size(); i++)
    {
        ExtensionData obj = list[i];
        if (obj.status_int == EXTENSION_STATUS::BUSY)
        {
            BusyNum++;
        }

        obj.address = IniFile::instance()->ReadKeyValue(QString("%1/Address").arg(obj.number));
        if (obj.address.isEmpty())
            QMessageBox::warning(this, QStringLiteral("提示"),  obj.number + QStringLiteral("分机IP地址无效，请检查配置文件config"));
        obj.meetingNo = IniFile::instance()->GetGroupNumber();

        ControlWidget * controlwidget = new ControlWidget(nullptr,obj);

        connect(controlwidget, &ControlWidget::SignalOperationFailed, this, &ControlWindow::SlotOperationFailed);
        connect(controlwidget, &ControlWidget::to_refresh, this, &ControlWindow::Refresh);
        connect(NetWorker::instance(), &NetWorker::SignalTimeout, controlwidget, &ControlWidget::SlotTimeout);
        connect(controlwidget, SIGNAL(signalOpenStation(QString)), this, SLOT(slotOpenStation(QString)));
        connect(controlwidget, SIGNAL(signalCloseStation(QString, bool)), this, SLOT(slotCloseStation(QString, bool)));
        connect(controlwidget, SIGNAL(signalPlayText(QString,QString)), this, SLOT(slotPlayTextAudio(QString,QString)));
        connect(controlwidget, SIGNAL(signalRestart()), this, SLOT(slotRestartApp()));
        controlwidget->RefreshView();

        QListWidgetItem *widgetItem = new QListWidgetItem();
        widgetItem->setSizeHint(QSize(310, 340));
        listWidget->addItem(widgetItem);
        listWidget->setItemWidget(widgetItem, controlwidget);
        singleControlMap.insert(obj.number, controlwidget);
    }

    if (BusyNum > 2)
    {
        if (groupControlWidget != nullptr)
        {
            groupControlWidget->SetRx();
        }
    }
}

void ControlWindow::AddGroupWidget()
{
    if(singleControlMap.size() == 0) return;
    groupControlWidget = new GroupControlWidget();
    connect(NetWorker::instance(), &NetWorker::SignalTimeout, groupControlWidget, &GroupControlWidget::SlotTimeout);
    connect(NetWorker::instance(), SIGNAL(SignalReconnected()), groupControlWidget, SLOT(SlotResume()));
    for(int i = 0; i < singleControlMap.size(); i++)
    {
        ControlWidget *control_widget = singleControlMap.values().at(i);
        connect(groupControlWidget, &GroupControlWidget::SignalGroupRxButtonClicked,
                control_widget, &ControlWidget::SlotGroupRxButtonClicked);
        connect(groupControlWidget, &GroupControlWidget::SignalGroupTxButtonPressedOrReleased,
                control_widget, &ControlWidget::SlotGroupTxButtonPressedOrReleased);
    }

    QListWidgetItem *widgetItem = new QListWidgetItem();
    widgetItem->setSizeHint(QSize(310, 340));
    listWidget->addItem(widgetItem);
    listWidget->setItemWidget(widgetItem, groupControlWidget);

    groupControlWidget->channel = IniFile::instance()->GetGroupChannel();
    groupControlWidget->InitGroupUI();
}

void ControlWindow::DeleteWidget()
{
    if (singleControlMap.isEmpty())
        return;

    QMap<QString, ControlWidget*>::iterator iter = singleControlMap.begin();
    for (; iter!=singleControlMap.end(); iter++)
    {
        if (iter.value() != nullptr)
        {
            delete iter.value();
            iter.value() = nullptr;
        }
    }
    singleControlMap.clear();
}

void ControlWindow::slotOpenTest()
{
    mTest = !mTest;
    foreach (ControlWidget* w, singleControlMap) {
        w->RefreshView();
    }
}

void ControlWindow::Refresh()
{
    NetWorker::instance()->signalQueryExtension();
}

void ControlWindow::slot_heartbeat_timeup(bool sts, const QString& msg)
{
    this->statusBar()->showMessage(msg);
    if(sts)
    {
        //刷新数据
//        Refresh();
    }
}

void ControlWindow::SlotOperationFailed(QString username, int error_num)
{
    QString message = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ")
            + username
            + tr(" extension operation failed, reason for failure is: ")
            + NetWorker::instance()->errorNo2String(error_num);
    statusBar()->showMessage(message);
}

void ControlWindow::saveTemplateContent()
{
    int index = 0;
    for (int i=0; i<ui->qTableWidget->rowCount(); i++)
    {
        QTableWidgetItem * item = ui->qTableWidget->item(i,0);
        if (item != nullptr)
        {
            QString str = item->text();
            if (!str.isEmpty())
                IniFile::instance()->WriteKeyValue(QString("Broadcast/row%1").arg(index++), str);
        }
    }
    for(int i = index; i<ui->qTableWidget->rowCount(); i++)
    {
        IniFile::instance()->WriteKeyValue(QString("Broadcast/row%1").arg(index++), "");
    }
}

void ControlWindow::closeEvent(QCloseEvent *event)
{
    quint64 appid = QApplication::applicationPid();
    saveTemplateContent();
    if(messageBox(QStringLiteral("退出客户端？"), true) == QMessageBox::Cancel)
    {
        event->ignore();  //忽略退出信号，程序继续运行
    } else
    {
        qDebug()<<"start close";
        foreach (ControlWidget* w, singleControlMap) {
            w->slotCloseRelay();
        }
        QMessageBox msgBox;
        msgBox.setWindowTitle(QStringLiteral("提示"));
        msgBox.setText(QStringLiteral("系统关闭中...."));
        msgBox.setStandardButtons(QMessageBox::NoButton);
        QTimer* timeout_timer = new QTimer(this);
        connect(timeout_timer, &QTimer::timeout, &msgBox, &QMessageBox::accept);
        timeout_timer->setSingleShot(true);
        timeout_timer->start(5* 1000);
        msgBox.exec();
        NetWorker::instance()->signalLogout();
        QProcess process;
        process.startDetached(QString("taskkill /f /pid %1").arg(appid));
    }
}


int ControlWindow::messageBox(const QString& msg, bool cancel)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(QStringLiteral("提示"));
    msgBox.setText(msg);
    QMessageBox::StandardButtons btns = QMessageBox::Ok;
    if(cancel) btns |= QMessageBox::Cancel;
    msgBox.setStandardButtons(btns);
    msgBox.setButtonText (QMessageBox::Ok,QStringLiteral("确定"));
    if(cancel)
    {
        msgBox.setButtonText (QMessageBox::Cancel,QStringLiteral("取消"));
    }

    msgBox.setDefaultButton(QMessageBox::Ok);
    return msgBox.exec();
}

// 删除播报内容
void ControlWindow::on_DeleteBtn_clicked()
{
    // 设置TableWidget双击单元格进行修改
    ui->qTableWidget->setEditTriggers(QAbstractItemView::CurrentChanged);
    QTableWidgetItem * item = ui->qTableWidget->currentItem();
    if (item != nullptr)
    {
        if(messageBox(QStringLiteral("删除选中的模板？"), true) == QMessageBox::Cancel) return;
    }
    int row_cnt = ui->qTableWidget->rowCount();
    ui->qTableWidget->removeRow(item->row());
    ui->qTableWidget->setRowCount(row_cnt);
    saveTemplateContent();
}

// 修改播报内容
void ControlWindow::on_ChangeBtn_clicked()
{    
    // 设置TableWidget可对单元格内容修改
    ui->qTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    QTableWidgetItem * item = ui->qTableWidget->currentItem();
    if(!item) return;
    QString src = IniFile::instance()->ReadKeyValue(QString("Broadcast/row%1").arg(item->row()));
    if(messageBox(QStringLiteral("保存对当前模板的修改？"), true) == QMessageBox::Cancel)
    {
        item->setText(src);
        return;
    }
    QString inform = item->text().trimmed();
    if(inform.trimmed().isEmpty())
    {
        int row_cnt = ui->qTableWidget->rowCount();
        ui->qTableWidget->removeRow(item->row());
        ui->qTableWidget->setRowCount(row_cnt);
        saveTemplateContent();
    } else
    {
        IniFile::instance()->WriteKeyValue(QString("Broadcast/row%1").arg(item->row()), item->text());
    }
}

// 发送播报内容
void ControlWindow::on_SendBtn_clicked()
{
    // 设置TableWidget不能对单元格内容修改
    ui->qTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    QTableWidgetItem * item = ui->qTableWidget->currentItem();
    if(item == nullptr)
    {
        messageBox(QStringLiteral("请选择一个播报模板"), false);
        return;
    }
    QString inform = item->text().trimmed();
    if (inform.isEmpty())
    {
        messageBox(QStringLiteral("选择播报的内容为空，请重新选择"), false);
        return;
    }
    //检查播放是否正确设定
    if(ui->plan_broad_btn->isChecked())
    {
        int Interval = ui->time_interval->text().trimmed().toInt();
        int timers = ui->times->text().trimmed().toInt();
        if (Interval<=0 || timers < 1)
        {
            messageBox(QStringLiteral("发送间隔和次数配置错误，请重新配置"), false);
            return;
        }
        //检查时间是否设定正确
        QDateTime start = ui->task_start_time->dateTime();
        if(start < QDateTime::currentDateTime())
        {
            messageBox(QStringLiteral("播放的开始时间应大于当前的系统时间，请重新配置"), false);
            return;
        }
    }
    //获取要播报的岸站
    if (getBroadCastExtensions().size() <= 0)
    {
        messageBox(QStringLiteral("未选择岸站"), false);
#ifndef CD_TEST
        return;
#endif
    }

    if (messageBox(QStringLiteral("生成播放任务？"), true) == QMessageBox::Cancel) return;
    //开始生成生成播放任务
    BroadcastSetting setting;
    setting.mType = ui->im_broad_btn->isChecked() ? Task_Immediately : Task_Planning;
    setting.mTimes = ui->times->text().toInt();
    setting.mStartTime = ui->task_start_time->dateTime().toMSecsSinceEpoch();
    setting.mTimeGap = ui->time_interval->text().toInt() * 60;
    setting.mContent = ui->qTableWidget->currentItem()->text();
    setting.mBroadExtIdList = getBroadCastExtensions();
    setting.mIsTry = false;
    setting.mVol = ui->volume->getValue();
    //添加任务
    if(mTaskMgrCenter) mTaskMgrCenter->signalRecvBroadSetting(setting);
    return;
}

void ControlWindow::slotRecvTotalBroadTaskList(const TaskList& future)
{
    //首先更新待播发任务列表
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(future.size());
    for (int i=0; i<ui->tableWidget->rowCount(); i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem();
        ui->tableWidget->setItem(i,0,item);
        if (i%2 == 0)
        {
            item->setBackgroundColor(QColor(230,220,210));
        }
        QString content = QString("%1 %2 %3")
                .arg(QDateTime::fromMSecsSinceEpoch(future[i].mTime).toString("yyyy-MM-dd hh:mm:ss"))
                .arg(future[i].mBroadExtIdList.join(","))
                .arg(future[i].mContent);
        ui->tableWidget->item(i,0)->setText(content);
        ui->tableWidget->item(i, 0)->setData(Qt::UserRole, future[i].mDBID);
    }
    if(ui->tableWidget->rowCount() == 0)
    {
        ui->task_del->setVisible(false);
    }
}
void ControlWindow::slotRecvBroadTaskJobList(const TaskList& now )
{
    //开始播放音频
    foreach(BroadcastTask task, now)
    {
        PromptVideo(task.mBroadExtIdList, task.mFileName);
    }

}

void ControlWindow::PromptVideo(const QStringList& phone_list, const QString& filename)
{
    qDebug()<<"play audio:"<<filename<<phone_list;
    if(filename.isEmpty() || phone_list.size() == 0) return;
    for(int i = 0; i < phone_list.size(); ++i)
    {
        ControlWidget* widget = singleControlMap[phone_list[i]];
        if(widget && !widget->isPlayAudio())
        {
            widget->playAudio(filename, false);
        }

    }
}

QStringList ControlWindow::getBroadCastExtensions(bool check)
{
    QStringList list;
    for(int i = 0; i < m_pExtensionModel->rowCount(); ++i)
    {
        if(check && m_pExtensionModel->item(i, 0)->checkState() == Qt::Checked)
        {
            list.push_back(m_pExtensionModel->item(i, 0)->data().toString());
        } else if(!check)
        {
            list.push_back(m_pExtensionModel->item(i, 0)->data().toString());
        }
    }

    return list;
}

void ControlWindow::on_play_clicked()
{
    // 设置TableWidget不能对单元格内容修改
    ui->qTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    QTableWidgetItem * item = ui->qTableWidget->currentItem();
    qDebug() << "item :" << item;
    if (item == nullptr)
    {
        QMessageBox::information(nullptr, QStringLiteral("提示") ,  QStringLiteral("未选择要播报的内容"));
        return;
    }


    QString inform = item->text().trimmed();
    qDebug() << "content:" << inform;
    if (inform.isEmpty())
    {
        QMessageBox::information(nullptr, QStringLiteral("提示") , QStringLiteral("要播报的内容为空"));
        return;
    }

    //开始生成生成播放任务
    BroadcastSetting setting;
    setting.mType = ui->im_broad_btn->isChecked() ? Task_Immediately : Task_Planning;
    setting.mTimes = ui->times->text().toInt();
    setting.mStartTime = ui->task_start_time->dateTime().toMSecsSinceEpoch();
    setting.mTimeGap = ui->time_interval->text().toInt() * 60;
    setting.mContent = ui->qTableWidget->currentItem()->text();
    setting.mBroadExtIdList = getBroadCastExtensions();
    setting.mIsTry = true;
    setting.mVol = ui->volume->getValue();
    //添加任务
    if(mTaskMgrCenter) mTaskMgrCenter->signalRecvBroadSetting(setting);
}

void ControlWindow::slotPlayTextAudio(const QString &id, const QString &text)
{
    qDebug()<<"play test audio now....";
    //开始生成生成播放任务
    BroadcastSetting setting;
    setting.mType = Task_Immediately;
    setting.mTimes = 1;
    setting.mStartTime = QDateTime::currentMSecsSinceEpoch();
    setting.mTimeGap = 60;
    setting.mContent = text;
    setting.mBroadExtIdList.append(id);
    setting.mIsTry = false;
    setting.mVol = 90;
    //添加任务
    if(mTaskMgrCenter) mTaskMgrCenter->signalRecvBroadSetting(setting);
}

void ControlWindow::slotRecvJavaInfo(const JavaInfo &info)
{
    qDebug()<<"info.text:"<<info.text<<info.frequency;
    if(info.text.isEmpty()) return;
    //开始生成播放任务, 这里就是及时播放任务.立即播
    BroadcastSetting setting;
    setting.mType = /*(info.frequency == 1 ? Task_Immediately : Task_Planning)*/Task_Immediately;
    setting.mTimes = 1;
    setting.mStartTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    setting.mTimeGap = info.timeInterval;
    setting.mFileName.clear();
    setting.mContent = info.text;
    setting.mIsTry = false;
    setting.mBroadExtIdList = getBroadCastExtensions(true);
    if(setting.mBroadExtIdList.size() == 0)
    {
        setting.mBroadExtIdList = getBroadCastExtensions(false);
    }
    setting.mVol = ui->volume->getValue();
    //添加任务
    if(mTaskMgrCenter) mTaskMgrCenter->signalRecvBroadSetting(setting);
    return;

}

void ControlWindow::BroadCastTypeChanged(int id)
{
    qDebug()<<"id clicked:"<<id;
    setPlanControlsVisible(ui->plan_broad_btn->isChecked());
}
void ControlWindow::setPlanControlsVisible(bool sts)
{
    ui->plan_widget->setVisible(sts);
#if 0
    ui->label_b_time->setVisible(sts);
    ui->label_times->setVisible(sts);
    ui->label_gap->setVisible(sts);
    ui->time_interval->setVisible(sts);
    ui->task_start_time->setVisible(sts);
    ui->times->setVisible(sts);
#endif
}

void ControlWindow::on_im_broad_btn_clicked(bool checked)
{
    if(checked) setPlanControlsVisible(false);
}

void ControlWindow::on_plan_broad_btn_clicked(bool checked)
{
    setPlanControlsVisible(checked);
}

void ControlWindow::on_task_del_clicked()
{
    QTableWidgetItem* item = ui->tableWidget->currentItem();
    if(item)
    {
        int id = item->data(Qt::UserRole).toInt();
        if(messageBox(QStringLiteral("删除当前选择的播放任务？"), true) == QMessageBox::Cancel) return;
        if(mTaskMgrCenter) mTaskMgrCenter->signalRemoveBroadTask(id);
    }
}

void ControlWindow::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
    if(item)
    {
        ui->task_del->setVisible(true);
    } else
    {
        ui->task_del->setVisible(false);
    }
}

void ControlWindow::slotRecvErrorMsg(const QString& msg)
{
    this->statusBar()->showMessage(msg);
}

void ControlWindow::slotOpenStation(const QString& source)
{
    qDebug()<<"recv notice to open station";
    foreach (ControlWidget* w, singleControlMap) {
//        qDebug()<<"widget:"<<w->GetExtension()->data.username<<w->isCenter();
        if(w->isCenter() || w->isBroadcastStation()) continue;  //让呼叫岸站将声音播放出去
        //打开岸站的继电器
        qDebug()<<"operate station :"<<w->GetExtension()->getData().username;
        if(w->getCmdSourceID().isEmpty())
        {
            qDebug()<<"start open relay now!!!"<<" source:"<<source;
#if 0
            w->setCmdSourceID(source);
#endif
            w->openRelayControl(/*IniFile::instance()->getMicrophoneMaxOpenSeconds()*/0);
        } else
        {
            qDebug()<<"station is already opened now..  not processed";
        }
    }
}

void ControlWindow::slotCloseStation(const QString& source, bool sts)
{
    qDebug()<<"recv notice to close station"<<source;
    foreach (ControlWidget* w, singleControlMap) {
//        qDebug()<<"widget:"<<w->GetExtension()->data.username<<w->isCenter();
        if(w->isCenter() || w->isBroadcastStation()) continue;  //将呼叫岸站关闭
        //关闭岸站的继电器
        qDebug()<<"try close relay now!!!"<<w->GetExtension()->getData().username;
        if(w->isTxPressed())
        {
            qDebug()<<"current tx button is pressed  not process yet.";
        }else
        {
#if 0
            if(w->getCmdSourceID() == source)
            {
#endif
                qDebug()<<"start close relay now!!!"<<" source:"<<source;
                w->closeRelayControl();
                w->setCmdSourceID("");
                if(sts)
                {
                    slotRecvErrorMsg(tr("外设设备异常,强制关闭继电器."));
                }
#if 0
            } else
            {
                qDebug()<<"station is opened by other source:"<<w->getCmdSourceID();
            }
#endif
        }
    }
}

void ControlWindow::slotRestartApp()
{
    NetWorker::instance()->signalLogout();
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    QString workingDirectory = QDir::currentPath();
    QProcess::startDetached(program, arguments, workingDirectory);
    QApplication::exit();
}

