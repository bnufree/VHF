#ifndef RXBUTTON_H
#define RXBUTTON_H

#include <QPushButton>
#include <QMouseEvent>
#include <QTimer>

class RxButton : public QPushButton
{
    Q_OBJECT
public:
    explicit RxButton(QWidget *parent = nullptr);
protected:
    void mouseReleaseEvent(QMouseEvent* e);

signals:
    void doubleClicked();

public slots:
    void slotSingleClicked();
private:
    QTimer*     mClickTimer;
};

#endif // RXBUTTON_H
