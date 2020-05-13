#ifndef ZCHXVHFAUDIOPLAYTHREAD_H
#define ZCHXVHFAUDIOPLAYTHREAD_H

#include <QThread>
#include "extension.h"
#include "relaycontroller.h"

class zchxVHFAudioPlayThread : public QThread
{
    Q_OBJECT
public:
    explicit zchxVHFAudioPlayThread(const QString& fileName, QObject *parent = nullptr);
    void setExtension(Extension* data) {mExtension = data;}
    void setRelayController(RelayController* data) {mRelayController = data;}

public :
    void run();

signals:

public slots:
private:
    QString   mAudioFileName;
    Extension*      mExtension;
    RelayController* mRelayController;
};

#endif // ZCHXVHFAUDIOPLAYTHREAD_H
