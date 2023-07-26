#ifndef EMAILUTIL_H
#define EMAILUTIL_H

#include <QtNetwork>
#include <QMessageBox>

class EmailUtil : public QObject
{
    Q_OBJECT

public:
    EmailUtil(QObject *parent = nullptr) : QObject(parent)
    {
        smtpSocket = new QSslSocket(this);
        connect(smtpSocket, SIGNAL(connected()), this, SLOT(smtpConnected()));
        connect(smtpSocket, SIGNAL(encrypted()), this, SLOT(smtpEncrypted()));
        connect(smtpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(smtpError(QAbstractSocket::SocketError)));
        connect(smtpSocket, SIGNAL(readyRead()), this, SLOT(smtpReadyRead()));
    }

    void sendEmail(const QString &host, int port, const QString &username, const QString &password, const QString &from, const QString &to, const QString &subject, const QString &body)
    {
        this->host = host;
        this->port = port;
        this->username = username;
        this->password = password;
        this->from = from;
        this->to = to;
        this->subject = subject;
        this->body = body;

        smtpSocket->connectToHostEncrypted(host, port);
        if (!smtpSocket->waitForConnected()) {
            QMessageBox::critical(nullptr, "Error", smtpSocket->errorString());
        } else {
            QMessageBox::information(nullptr, "Email", "Send successed!");
        }
    }

private slots:
    void smtpConnected()
    {
        sendCommand("EHLO localhost");
    }

    void smtpEncrypted()
    {
        // sendCommand("AUTH LOGIN");
        sendCommand("STARTTLS");
    }

    void smtpError(QAbstractSocket::SocketError error)
    {
        QMessageBox::critical(nullptr, "Error", smtpSocket->errorString());
    }

    void smtpReadyRead()
    {
        QByteArray response = smtpSocket->readAll();
        qInfo() << "email response:" << response;
        if (response.startsWith("220"))
        {
            if (!_sendHello)
            {
                _sendHello = true;
                sendCommand("EHLO localhost");
            }
        }
        else if (response.startsWith("250")) // 250之后是334输入账号密码
        {
            if (!_sendAuthLogin)
            {
                _sendAuthLogin = true;
                sendCommand("AUTH LOGIN");
            }
            else if (_sendFrom && !_sendRcptTo)
            {
                _sendRcptTo = true;
                sendCommand("RCPT TO: <" + to + ">");
            }
            else if (_sendFrom && _sendRcptTo && !_sendData)
            {
                _sendData = true;
                sendData(); // 触发 354 End data with <CR><LF>.<CR><LF>
            }
        }
        else if (response.startsWith("334")) // 334两次分别回复账号密码
        {
            if (!_sendUsername)
            {
                _sendUsername = true;
                sendCommand(username.toUtf8().toBase64());
            }
            else if (!_sendPassword)
            {
                _sendPassword = true;
                sendCommand(password.toUtf8().toBase64());
            }
        }
        else if (response.startsWith("235"))  // 235 Authentication successful
        {
            _sendFrom = true;
            sendCommand("MAIL FROM: <" + from + ">");
        }
        else if (response.startsWith("354"))
        {
            // sendData();
            sendCommand("QUIT");
        }
        else if (response.startsWith("221"))
        {
            smtpSocket->disconnectFromHost();
        }
        else if (response.startsWith("5"))
        {
            QMessageBox::critical(nullptr, "Error", response);
        }

        /*if (response.startsWith("220")) {
            sendCommand("EHLO localhost");
        } else if (response.startsWith("235")) {
            sendCommand("MAIL FROM: <" + from + ">");
        } else if (response.startsWith("250")) {
            sendCommand("AUTH LOGIN");
        } else if (response.startsWith("250")) {
            sendCommand("RCPT TO: <" + to + ">");
        } else if (response.startsWith("354")) {
            sendData();
        }  else if (response.startsWith("221")) {
            smtpSocket->disconnectFromHost();
        } else if (response.startsWith("334")) {
            sendCommand(username.toUtf8().toBase64());
        } else if (response.startsWith("503 Send command mailfrom first")) {
            sendCommand("EHLO localhost");
            sendCommand("AUTH LOGIN");
            sendCommand("MAIL FROM: <" + from + ">");
        } else if (response.startsWith("503 Error: need EHLO and AUTH first")) {
        }*/  else {
            // QMessageBox::critical(nullptr, "Error", response);
        }
    }

private:
    void sendCommand(const QString &command)
    {
        qInfo() << "email send cmd:" << command;
        smtpSocket->write(command.toUtf8() + "\r\n");
        smtpSocket->waitForBytesWritten();
    }

    void sendData()
    {
        sendCommand("DATA");

        QString data = "From: " + from + "\r\n"
                       "To: " + to + "\r\n"
                       "Subject: " + subject + "\r\n"
                       "\r\n"
                       + body + "\r\n"
                       ".";
        qInfo() << "email send data:" << data.length();
        smtpSocket->write(data.toUtf8() + "\r\n");
        smtpSocket->waitForBytesWritten();
    }

    QSslSocket *smtpSocket;
    QString host;
    int port;
    QString username;
    QString password;
    QString from;
    QString to;
    QString subject;
    QString body;

    bool _sendHello = false;
    bool _sendAuthLogin = false;
    bool _sendUsername = false;
    bool _sendPassword = false;
    bool _sendFrom = false;
    bool _sendRcptTo = false;
    bool _sendData = false;
    bool _sendSubject = false;
};

#endif // EMAILUTIL_H
