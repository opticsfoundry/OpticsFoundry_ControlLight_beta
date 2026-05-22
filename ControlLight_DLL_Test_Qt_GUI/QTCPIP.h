#ifndef QTCPIP_H
#define QTCPIP_H

#include <QObject>
#include <QTcpSocket>
#include <QSize>
#include <QString>
#include <QTimer>

#define IncommingBufferSize (1500)


extern void Sleep_ms(int delay_in_milli_seconds);

class QTelnet : public QTcpSocket
{
    Q_OBJECT

public:
    enum SocketStatus
    {
        Disconnected,
        Resolving,
        Connecting,
        Connected
    };
    unsigned int NumberReconnects;

private:
    char m_buffIncoming[IncommingBufferSize];
    char m_buffProcessed[IncommingBufferSize];
    QByteArray m_buffSB;
    int m_actualSB;

    QString m_lastHost;
    quint16 m_lastPort;
    QTimer* m_reconnectTimer;

    void emitEndOfRecord()              { Q_EMIT(endOfRecord());      }
    void emitEchoLocal(bool bEcho)      { Q_EMIT(echoLocal(bEcho));   }

    void sendTelnetControl(char codigo);
    void handleSB(void);
    void transpose(const char *buf, int iLen);

    void willsReply(char action, char reply);
    void wontsReply(char action, char reply);
    void doesReply(char action, char reply);
    void dontsReply(char action, char reply);

    void sendSB(char code, char *arr, int iLen);
    qint64 doTelnetInProtocol(qint64 buffSize);

public:
    double standard_timeout_in_seconds;
public:
    explicit QTelnet(QObject *parent = 0);

    virtual void connectToHost(const QString &host, quint16 port);
    void sendData(const QByteArray &ba);

    void write(const char c);
    void write(const QString &msg);
    void writeBuffer(const char* buffer, qint64 buffer_length);
    void writeString(const QString &msg);
    void writeDouble(double d);
    void writeLong(long l);
    void writeBool(bool b);

    bool readBuffer(unsigned char* buffer, qint64 buffer_length, double timeout_in_seconds = 10);
    bool readString(QString &msg, double timeout_in_seconds = 0);
    bool readDouble(double &d, double timeout_in_seconds = 0);
    bool readLong(long &l, double timeout_in_seconds = 0);
    bool readBool(bool &b, double timeout_in_seconds = 0);

    bool isConnected(bool no_error_message = true) const;
    void disconnectFromRemote();
    void attemptReconnect();
    bool verifyConnected();
    QString peerInfo() const;

signals:
    void newData(const char *buff, int len);
    void endOfRecord();
    void echoLocal(bool echo);

private slots:
    void socketError(QAbstractSocket::SocketError err);
    void onReadyRead();
    void handleDisconnected();
};

#endif // QTCPIP_H
