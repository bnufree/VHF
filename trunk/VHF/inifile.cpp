#include "inifile.h"
#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QTextCodec>

bool    mDebug = false;

IniFile* IniFile::minstance = nullptr;
IniFile::MGarbage IniFile::Garbage;
IniFile::IniFile()
{
//    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QString path = QCoreApplication::applicationDirPath();
    m_pMainConfig = new QSettings(QString("%1/config.ini").arg(path), QSettings::IniFormat);
    m_pMainConfig->setIniCodec(QTextCodec::codecForName("System"));
    ReadIniFile();
}

IniFile::~IniFile()
{
    delete m_pMainConfig;
}

IniFile* IniFile::instance()
{
    if(!minstance)
    {
        minstance = new IniFile();
    }
    return minstance;
}

void IniFile::ReadIniFile()
{
    m_strSavePassword = ReadKeyValue("Login/SavePassword", "1");
    m_strAutoLogin = ReadKeyValue("Login/AutoLogin", "1");
    m_strUsername = ReadKeyValue("Login/Username", "api");
    m_strPassword = ReadKeyValue("Login/Password", "Zchx123456");

    m_strPbxServerIp = ReadKeyValue("Configuration/IppbxServerAddress");
    m_strPbxServerPort =ReadKeyValue("Configuration/IppbxServrPort", "8088");
    m_strApiVersion = ReadKeyValue("Configuration/ApiVersion", "v1.0.6");
    m_strListenPort = ReadKeyValue("Configuration/ListenPort", "8260");
    m_strTxTimeout = ReadKeyValue("Configuration/TxTimeout");

    m_strGroupName = ReadKeyValue("Group/Name", "10");
    m_strGroupNumber =ReadKeyValue("Group/Number", "6400");
    m_strGroupChannel = ReadKeyValue("Group/Channel", "10");
    m_strGroupCenter = ReadKeyValue("Group/Center", "1001");
    m_strGroupAutoEnter = ReadKeyValue("Group/AutoEnter", "0");

    m_strVHFServerIP = ReadKeyValue("VhfServer/IP", "192.168.5.150");
    m_strVHFServerPort = ReadKeyValue("VhfServer/Port", "8621");
    m_strVHFUser = ReadKeyValue("VhfServer/User", "admin");
    m_strVHFPWD = ReadKeyValue("VhfServer/Password", "PY[2\\IPlO4K3[YG5PlXmPlHmQFO{P4Tn[li6OoPoQVm?");
    m_strJavaHost = ReadKeyValue("Java/Host", "192.168.30.224");
    m_strJavaTopic = ReadKeyValue("Java/Topic", "vhf");
    m_iJavaPort = ReadKeyValue("Java/Port", "6666").toInt();
    m_bJavaEnable = ReadKeyValue("Java/Enable", "0").toInt();
    m_RelayRequstTimeInterval = ReadKeyValue("Other/RelayRequestInervalMSec", "300").toInt();
    m_MicPhMaxOpenSeconds = ReadKeyValue("Other/MicrophoneMaxOpendTimeSec", "60").toInt();
    mDebug = ReadKeyValue("Other/DebugOutput", "0").toInt();
}

void IniFile::SetAutoLogin(QString AutoLogin)
{
    WriteKeyValue("Login/AutoLogin", AutoLogin);
}

void IniFile::SetPassword(QString Password)
{
    WriteKeyValue("Login/Password", Password);

}

void IniFile::SetUsername(QString Username)
{
    WriteKeyValue("Login/Username", Username);
}

void IniFile::SetSavePassword(QString SavePassword)
{
    WriteKeyValue("Login/SavePassword", SavePassword);
}

void IniFile::WriteKeyValue(QString Key, QString KeyValue)
{
    m_pMainConfig->setValue(Key, KeyValue);
}

QString IniFile::ReadKeyValue(QString Key, QString Default)
{
    QString val = m_pMainConfig->value(Key).toString().trimmed();
    if(val.isEmpty() && !Default.isEmpty())
    {
        WriteKeyValue(Key, Default);
        val = m_pMainConfig->value(Key).toString().trimmed();
    }
    return val;
}
