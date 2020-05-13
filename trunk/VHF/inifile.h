#ifndef INIFILE_H
#define INIFILE_H

#include<QString>
#include<QVariant>
#include <QMutex>
#include <QVector>
#include <QSettings>

class IniFile
{
public:    
    ~IniFile();
    void ReadIniFile();

    void WriteKeyValue(QString Key, QString KeyValue);
    QString ReadKeyValue(QString Key, QString Default = "");

    static IniFile* instance();

    QString GetSavePassword(){return m_strSavePassword;}
    QString GetAutoLogin(){return m_strAutoLogin;}
    QString GetUsername(){return m_strUsername;}
    QString GetPassword(){return m_strPassword;}
    void SetSavePassword(QString SavePassword);/*{m_strSavePassword = SavePassword;}*/
    void SetAutoLogin(QString AutoLogin);/*{m_strUsername = AutoLogin;}*/
    void SetUsername(QString Username);/*{m_strUsername = Username;}*/
    void SetPassword(QString Password);/*{m_strPassword = Password;}*/

    QString GetPbxServerIp(){return m_strPbxServerIp;}
    QString GetPbxServerPort(){return m_strPbxServerPort;}
    QString GetApiVersion(){return m_strApiVersion;}
    QString GetListenPort(){return m_strListenPort;}
    QString GetTxTimeout(){return m_strTxTimeout;}
    void SetPbxServerIp(QString PbxServerIp){m_strPbxServerIp = PbxServerIp;}
    void SetPbxServerPort(QString PbxServerPort){m_strPbxServerPort = PbxServerPort;}
    void SetApiVersion(QString ApiVersion){m_strApiVersion = ApiVersion;}
    void SetListenPort(QString ListenPort){m_strListenPort = ListenPort;}
    void SetTxTimeout(QString TxTimeout){m_strTxTimeout = TxTimeout;}

    QString GetGroupName(){return m_strGroupName;}
    QString GetGroupNumber(){return m_strGroupNumber;}
    QString GetGroupChannel(){return m_strGroupChannel;}
    QString GetGroupCenter(){return m_strGroupCenter;}
    QString GetGroupAutoEnter(){return m_strGroupAutoEnter;}
    void SetGroupName(QString GroupName){m_strGroupName = GroupName;}
    void SetGroupNumber(QString GroupNumber){m_strGroupNumber = GroupNumber;}
    void SetGroupChannel(QString GroupChannel){m_strGroupChannel = GroupChannel;}
    void SetGroupCenter(QString GroupCenter){m_strGroupCenter = GroupCenter;}
    void SetGroupAutoEnter(QString GroupAutoEnter){m_strGroupAutoEnter = GroupAutoEnter;}

    //VHF
    QString  getVHFServerIP() const {return m_strVHFServerIP;}
    QString  getVHFServerPort() const {return m_strVHFServerPort;}
    QString  getVHFUser() const {return m_strVHFUser;}
    QString  getVHFPwd() const {return m_strVHFPWD;}

    //Java
    QString  getJavaHost() const {return m_strJavaHost;}
    QString  getJavaTopic() const {return m_strJavaTopic;}
    int      getJavaPort() const {return m_iJavaPort;}
    bool     getJavaEnable() const {return m_bJavaEnable;}

    //
    int      getRelayRequestTimeInterval() const {return m_RelayRequstTimeInterval;}
    int      getMicrophoneMaxOpenSeconds() const {return m_MicPhMaxOpenSeconds;}

private:
    int m_nCamereId;
    QSettings *m_pMainConfig = nullptr;

    //login
    QString m_strUsername;
    QString m_strPassword;
    QString m_strSavePassword;
    QString m_strAutoLogin;

    //Configuration
    QString m_strPbxServerIp;
    QString m_strPbxServerPort;
    QString m_strApiVersion;
    QString m_strListenPort;
    QString m_strTxTimeout;

    //Group
    QString m_strGroupName;
    QString m_strGroupNumber;
    QString m_strGroupChannel;
    QString m_strGroupCenter;
    QString m_strGroupAutoEnter;

    //上传文件使用到的VHF
    QString  m_strVHFServerIP;
    QString  m_strVHFServerPort;
    QString  m_strVHFUser;
    QString  m_strVHFPWD;

    //Java数据
    QString  m_strJavaHost;
    QString  m_strJavaTopic;
    int      m_iJavaPort;
    bool     m_bJavaEnable;

    //继电器状态查询时间间隔
    int      m_RelayRequstTimeInterval;                         //继电器查询时间间隔
    int      m_MicPhMaxOpenSeconds;                             //microphone 最长的打开时间

private:
    IniFile();
    static IniFile* minstance;
    class MGarbage
    {
    public:
        ~MGarbage()
        {
            if (IniFile::minstance) delete IniFile::minstance;
        }
    };
    static MGarbage Garbage; // 定义一个静态成员，在程序结束时，系统会调用它的析构函数
};
#endif // INIFILE_H
