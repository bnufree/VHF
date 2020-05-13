#include "comtextaudiothread.h"
#include "sphelper.h"
#include <QDebug>

#pragma comment(lib, "sapi.lib")
#pragma comment(lib, "ole32.lib")

/**
 * @brief TextToVideo TTL自动播报
 * @param volume 音量   wChar 播报文字  filename 保存的文件名
 * @param 成功返回0 失败返回-1
 */
void ComTextAudioThread::saveTask()
{
    if(mTask.mContent.size() == 0 || mTask.mLocalAudioName.size() == 0)
    {
        qDebug()<<"Error content or filename set."<<"content:"<<mTask.mContent<<"filename:"<<mTask.mLocalAudioName;
        return;
    }
    //将文本开始转换为语音。文本转换为宽字符串
    qDebug() << "voice content:" << mTask.mContent << "file name:" << mTask.mLocalAudioName;

    LPCWSTR filename = (LPCWSTR)(mTask.mLocalAudioName.data());
    WCHAR   wChar[1024];
    memset(wChar ,0,1024);
    MultiByteToWideChar( /*CP_ACP*/CP_UTF8 , 0 , mTask.mContent.toLocal8Bit(), mTask.mContent.toLocal8Bit().size(), wChar , 1024);

    // 初始化语音接口
    ISpVoice *pVoice = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if (SUCCEEDED(hr))
    {
        qDebug()<<"Vol:"<<mTask.mVol;
        pVoice->SetVolume(static_cast<USHORT>(mTask.mVol));     //设置音量

        // 设置当前朗读频率
        CComPtr<ISpAudio>  m_cpOutAudio; //声音输出接口
        SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOOUT,&m_cpOutAudio ); //创建接口
        SPSTREAMFORMAT eFmt = SPSF_8kHz16BitMono;
        CSpStreamFormat Fmt;
        Fmt.AssignFormat(eFmt);
        qDebug() << "CComPtr<ISpAudio>:" << m_cpOutAudio;
        if (m_cpOutAudio)
        {
           hr = m_cpOutAudio->SetFormat(Fmt.FormatId(), Fmt.WaveFormatExPtr() );
        } else {
            qDebug()<<"output audio error occured return";
            return ;
        }

        pVoice->SetOutput( m_cpOutAudio, FALSE);

        // 设置输出到.wav文件里
        CComPtr <ISpStream> cpWavStream;
        CComPtr <ISpStreamFormat> cpOldStream;
        pVoice->GetOutputStream(&cpOldStream);
        CSpStreamFormat originalFmt;
        originalFmt.AssignFormat(cpOldStream);
        hr = SPBindToFile(filename, SPFM_CREATE_ALWAYS, &cpWavStream, &originalFmt.FormatId(), originalFmt.WaveFormatExPtr());
        pVoice->SetOutput(cpWavStream, TRUE);

        pVoice->Speak(wChar, SPF_DEFAULT, nullptr);
        pVoice->WaitUntilDone(1000);
        pVoice->Release();
        pVoice = nullptr;
    } else {
        qDebug()<<"com initialized failed";
    }
}

void ComTextAudioThread::playTask()
{
    if(mTask.mContent.size() == 0)
    {
        qDebug()<<"Error content or filename set."<<"content:"<<mTask.mContent;
        return;
    }
    //将文本开始转换为语音。文本转换为宽字符串
    qDebug() <<"play voice content:" << mTask.mContent;
    WCHAR   wChar[1024];
    memset(wChar ,0,1024);
    MultiByteToWideChar( /*CP_ACP*/CP_UTF8 , 0 , mTask.mContent.toLocal8Bit(), mTask.mContent.toLocal8Bit().size(), wChar , 1024);
    // 初始化语音接口
    ISpVoice *pVoice = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if (SUCCEEDED(hr))
    {
        pVoice->SetVolume(static_cast<USHORT>(mTask.mVol));     //设置音量
        pVoice->Speak(wChar, SPF_DEFAULT, nullptr);
        pVoice->WaitUntilDone(1000);
        pVoice->Release();
        pVoice = nullptr;
    } else {
        qDebug()<<"com initialized failed";
    }
}

ComTextAudioThread::ComTextAudioThread(const BroadcastSetting& task, QObject *parent) : QThread(parent)
{
    mTask = task;
}

void ComTextAudioThread::run()
{
    if(mTask.mContent.isEmpty()) return;
    //Com组件初始化
    if (FAILED(::CoInitialize(nullptr)))
    {
        qDebug()<<"com initialized failed";
    }
    //开始进行文本转语音
    if(mTask.mIsTry)
    {
        playTask();
    } else
    {
        saveTask();
    }
    ::CoUninitialize();
}
