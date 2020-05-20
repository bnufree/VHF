#include "logindialog.h"
#include "ui_logindialog.h"
#include "networker.h"
#include "inifile.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    mResend(true),
    mCancel(false)
{
    ui->setupUi(this);
    ui->test->setVisible(false);
    connect(ui->test, SIGNAL(clicked(bool)), this, SLOT(slotTestButtonClicked()));
    connect(ui->test, SIGNAL(doubleClicked()), this, SLOT(slotTestButtonDoubleClicked()));

    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(onOkButtonClicked()));
    connect(ui->cancelButton, SIGNAL(clicked()),
            this, SLOT(onCancelButtonClicked()));

    ui->usernameEdit->setText(IniFile::instance()->GetUsername());
    if (IniFile::instance()->GetSavePassword().toInt())
    {
        ui->savePasswordCheckBox->setChecked(true);
        ui->passwordEdit->setText(IniFile::instance()->GetPassword());
    }
    if (IniFile::instance()->GetAutoLogin().toInt())
    {
        ui->autoLoginCheckBox->setChecked(true);
        QTimer::singleShot(0, this, &LoginDialog::onOkButtonClicked);
        ui->okButton->setEnabled(false);
    } 

    connect(ui->autoLoginCheckBox, &QCheckBox::stateChanged, [=](int state){if (state) ui->savePasswordCheckBox->setChecked(true);});
    connect(NetWorker::instance(), SIGNAL(signalSendLoginResult(int)),
            this, SLOT(slotRecvLoginResult(int)));
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::onOkButtonClicked()
{
    QString username = ui->usernameEdit->text().trimmed();
    QString password = ui->passwordEdit->text().trimmed();
    QString passwordMd5;
    QByteArray baMd5 = QCryptographicHash::hash(password.toLocal8Bit(), QCryptographicHash::Md5);
    passwordMd5.append(baMd5.toHex().toLower());

    NetWorker::instance()->signalLogin(username, passwordMd5, true);
}

void LoginDialog::slotRecvLoginResult(int errorNo)
{
    QString username = ui->usernameEdit->text().trimmed();
    QString password = ui->passwordEdit->text().trimmed();

    qDebug() <<"errorNo:"<<errorNo;
    if (!errorNo)
    {
        int IsSavePassword = ui->savePasswordCheckBox->isChecked() ? 1 : 0;
        int IsAutoLogin = ui->autoLoginCheckBox->isChecked() ? 1 : 0;
        IniFile::instance()->SetSavePassword(QString::number(IsSavePassword));
        IniFile::instance()->SetAutoLogin(QString::number(IsAutoLogin));
        IniFile::instance()->SetUsername(username);
        if (IsSavePassword)
            IniFile::instance()->SetPassword(password);
        else
            IniFile::instance()->SetPassword("");
        if(!mCancel)
        {
            accept();
        } else
        {
            reject();
        }
    }
    else
    {
        ui->errorLabel->setText(QStringLiteral("%1").arg(NetWorker::errorNo2String(errorNo)));
        if(errorNo == 1 && mResend && !mCancel)
        {
            onOkButtonClicked();
        } else {
            reject();
        }

    }
}

void LoginDialog::onCancelButtonClicked()
{
    reject();
    close();
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
    mCancel = true;
    mResend = false;
}

void LoginDialog::slotTestButtonClicked()
{
    qDebug()<<__FUNCTION__;
}

void LoginDialog::slotTestButtonDoubleClicked()
{
    qDebug()<<__FUNCTION__;
}
