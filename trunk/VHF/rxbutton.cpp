#include "rxbutton.h"

RxButton::RxButton(QWidget *parent) : QPushButton(parent)
{
    mClickTimer = 0;
}

void RxButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(!mClickTimer)
    {
        mClickTimer = new QTimer;
        mClickTimer->setInterval(1000);
        mClickTimer->setSingleShot(true);
        connect(mClickTimer, SIGNAL(timeout()), this, SLOT(slotSingleClicked()));
        mClickTimer->start();
    } else
    {
        if(mClickTimer && mClickTimer->isActive())
        {
            mClickTimer->stop();
            delete mClickTimer;
            mClickTimer = 0;
            emit doubleClicked();
        }
    }

}

void RxButton::slotSingleClicked()
{
    if(mClickTimer)
    {
        mClickTimer->stop();
        delete mClickTimer;
        mClickTimer = 0;
    }

    emit clicked();
}
