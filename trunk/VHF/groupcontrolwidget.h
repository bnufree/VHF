#ifndef GROUPCONTROLWIDGET_H
#define GROUPCONTROLWIDGET_H

#include <QFrame>

namespace Ui {
class ControlWidget;
}

class GroupControlWidget : public QFrame
{
    Q_OBJECT

public:
    explicit GroupControlWidget(QFrame *parent = nullptr);
//    ~GroupControlWidget();

    void InitGroupUI();
    void SetRx();
    int group;
    QString channel;

public slots:
    void OnChannelEdited();
    void SlotTimeout();
    void SlotResume();


signals:
    void SignalGroupRxButtonClicked(bool isChecked);
    void SignalGroupTxButtonPressedOrReleased(bool isDown);


private:
    Ui::ControlWidget *ui;
};

#endif // GROUPCONTROLWIDGET_H
