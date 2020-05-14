#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QtNetwork/QNetworkReply>
#include <QDebug>
#include <QTimer>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
protected:
    virtual void closeEvent(QCloseEvent* event);
private slots:
    void slotRecvLoginResult(int code);

signals:
    void signalLoginSuccess();

private slots:
    void onOkButtonClicked();
    void onCancelButtonClicked();

    void slotTestButtonClicked();
    void slotTestButtonDoubleClicked();

private:
    Ui::LoginDialog *ui;
    bool    mResend;
    bool    mCancel;
};

#endif // LOGINDIALOG_H
