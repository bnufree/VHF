#ifndef COMTEXTAUDIOTHREAD_H
#define COMTEXTAUDIOTHREAD_H

#include <QThread>
#include "vhfdatadefines.h"

//struct TextAudioTask {
//    QString     mContent;       //
//    int         mVolume;
//    QString     mSaveFileName;  //if empty just try playing audio else save audio into file with specified file name

//    TextAudioTask(const QString& content, int vol, const QString& filenm = QString())
//    {
//        mContent =content;
//        mVolume = vol;
//        mSaveFileName = filenm;
//    }

//    TextAudioTask()
//    {
//        mContent = "";
//        mVolume = 0;
//        mSaveFileName = "";
//    }
//};

class ComTextAudioThread : public QThread
{
    Q_OBJECT
public:
    explicit ComTextAudioThread(const BroadcastSetting& task, QObject *parent = nullptr);
    BroadcastSetting task() const {return mTask;}
protected:
    void run();
private:
    void playTask();
    void saveTask();

signals:

public slots:
private:
    BroadcastSetting mTask;
};

#endif // COMTEXTAUDIOTHREAD_H
