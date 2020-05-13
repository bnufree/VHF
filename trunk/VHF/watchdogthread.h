#ifndef WATCHDOGTHREAD_H
#define WATCHDOGTHREAD_H

#include <QThread>

class WatchDogThread : public QThread
{
    Q_OBJECT
public:
    explicit WatchDogThread(QObject *parent = nullptr);
    void run();

signals:

public slots:
};

#endif // WATCHDOGTHREAD_H
