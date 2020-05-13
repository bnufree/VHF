#include "groupcontrolwidget.h"
#include "ui_controlwidget.h"

#include <QSettings>
#include <QTextCodec>
#include <QDebug>

GroupControlWidget::GroupControlWidget(QFrame *parent) :
    QFrame(parent),
    ui(new Ui::ControlWidget)
{
    ui->setupUi(this);
    ui->testBroadcastFrame->setVisible(false);
    ui->propertyFrame->setVisible(false);
}

void GroupControlWidget::InitGroupUI()
{
    ui->rxButton->setCheckable(true);
    ui->autoEnterCheckBox->setVisible(false);
    ui->edit_button->deleteLater();
    ui->username_edit->setText(tr("Group Control - %1").arg(1));
    ui->username_edit->setStyleSheet("background-color: #00b100;");
    ui->channel_edit->setText(channel);
    ui->rxButton->setIcon(QIcon(":/ico/vol"));
    ui->txButton->setIcon(QIcon(":/ico/mic"));

    connect(ui->channel_edit, &QLineEdit::editingFinished, this, &GroupControlWidget::OnChannelEdited);
    connect(ui->rxButton, &QPushButton::clicked, [=](){
        emit this->SignalGroupRxButtonClicked(ui->rxButton->isChecked());
    });
    connect(ui->txButton, &QPushButton::pressed, [=](){
        ui->txButton->setChecked(true);
        emit this->SignalGroupTxButtonPressedOrReleased(true);
    });
    connect(ui->txButton, &QPushButton::released, [=](){
        ui->txButton->setChecked(false);
        emit this->SignalGroupTxButtonPressedOrReleased(false);
    });
}

void GroupControlWidget::SetRx()
{
    ui->rxButton->setChecked(true);
}

void GroupControlWidget::OnChannelEdited()
{
    QString new_channel = ui->channel_edit->text();

    QString path = QCoreApplication::applicationDirPath();
    QSettings *config = new QSettings(QString("%1/config.ini").arg(path), QSettings::IniFormat);
    config->setIniCodec(QTextCodec::codecForName("System"));

    config->setValue(QString("Group%1/Channel").arg(group), new_channel);

    delete config;

    qDebug() << "Config Group Channel Edited " << group << new_channel;
}

void GroupControlWidget::SlotTimeout()
{
    ui->rxButton->setChecked(false);
    ui->rxButton->setEnabled(false);
    ui->txButton->setEnabled(false);
}

void GroupControlWidget::SlotResume()
{
    ui->rxButton->setEnabled(true);
    ui->txButton->setEnabled(true);
}

