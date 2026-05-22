#include "QTCPIP.h"
#include <QHostAddress>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QApplication>
#include <QThread>
#include <QTime>


QTelnet::QTelnet(QObject *parent) :
	QTcpSocket(parent), m_actualSB(0)
{
    NumberReconnects=0;
    connect( this, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)) );
    connect( this, SIGNAL(readyRead()),		this, SLOT(onReadyRead()) );
    connect(this, &QTcpSocket::disconnected, this, &QTelnet::handleDisconnected);
    standard_timeout_in_seconds = 3;
    m_lastHost = "";
    m_lastPort = 0;
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(3000); // Try every 3 seconds
    connect(m_reconnectTimer, &QTimer::timeout, this, &QTelnet::attemptReconnect);
}

QString QTelnet::peerInfo() const
{
	return QString("%1 (%2):%3").arg(peerName()).arg(peerAddress().toString()).arg(peerPort());
}

bool QTelnet::isConnected(bool no_error_message) const
{
    bool ok = state() == QAbstractSocket::ConnectedState;
    if ((!ok) && (!no_error_message)) {
        QMessageBox msgBox;
        msgBox.setText("QTelnet::isConnected() not connected");
        msgBox.exec();
    }
    return ok;
}

void QTelnet::connectToHost(const QString &host, quint16 port)
{
    m_lastHost = host;
    m_lastPort = port;

    if (!isConnected()) {
        abort();
        QTcpSocket::connectToHost(host, port);
    }
}

void QTelnet::disconnectFromRemote()
{
    qDebug() << "Disconnecting from remote host manually.";
    disconnectFromHost();
}

void QTelnet::handleDisconnected()
{
    qDebug() << "Disconnected. Starting reconnection attempts.";
    m_reconnectTimer->start();
}

void Sleep_ms(int delay_in_milli_seconds)
{
    QTime dieTime= QTime::currentTime().addMSecs(delay_in_milli_seconds);
    while (QTime::currentTime() < dieTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(10);
    }
}

bool QTelnet::verifyConnected() {
    if (state() == QAbstractSocket::UnconnectedState) {
        if (m_lastHost=="") {
            qDebug() << "Reconnection not successfull";
            return false;
        }
        unsigned int attempt = 0;
        qDebug() << "Trying to reconnect to" << m_lastHost << ":" << m_lastPort;
        while ((state() != QAbstractSocket::ConnectedState) && (attempt<30)) {
            Sleep_ms(2000);//QThread::msleep(2000);
            connectToHost(m_lastHost, m_lastPort);
            attempt++;
        }

        if (state() == QAbstractSocket::ConnectedState) {
            qDebug() << "Reconnected successfully!";
            NumberReconnects++;
            return true;
        } else {
            qDebug() << "Could not reconnect, even during 1 minute. Stopping any attempts.";
            return false;
        }
    } else return true;
}


void QTelnet::attemptReconnect()
{
    if (state() == QAbstractSocket::UnconnectedState) {
        if (m_lastHost=="") {
            m_reconnectTimer->stop();
            qDebug() << "Reconnection not successfull";
            return;
        }
        qDebug() << "Trying to reconnect to" << m_lastHost << ":" << m_lastPort;
        connectToHost(m_lastHost, m_lastPort);
        if (state() == QAbstractSocket::ConnectedState) {
            m_reconnectTimer->stop();
            NumberReconnects++;
            qDebug() << "Reconnected successfully!";
        }
    } else if (state() == QAbstractSocket::ConnectedState) {
        m_reconnectTimer->stop();
    }
}


void QTelnet::sendData(const QByteArray &ba)
{
    if( verifyConnected() )
        QTcpSocket::write( ba.constData(), ba.size() );
}

void QTelnet::socketError(QAbstractSocket::SocketError err)
{
	Q_UNUSED(err);
    qWarning() << "Socket error:" << err;
	disconnectFromHost();
}

void QTelnet::write(const char c)
{
    if( verifyConnected() ) {
        QTcpSocket::write( (char*)&c, 1 );
        flush();
    }
}

void QTelnet::writeBuffer(const char* buffer, qint64 buffer_length)
{
    if( verifyConnected() ) {
        QTcpSocket::write( buffer, buffer_length );
        flush();
    }
}


void QTelnet::write(const QString &msg)
{
    if( verifyConnected() ) {
        QTcpSocket::write(msg.toLatin1(), msg.size());
        flush();
    }
}

void QTelnet::writeString(const QString &msg)
{
    if( verifyConnected() ) {
        QString cmd = "*" + msg + "#";
        QTcpSocket::write(cmd.toLatin1(), cmd.size());
        flush();
    }
}

//convert double to string and write it
void QTelnet::writeDouble(double d)
{
	writeString(QString::number(d));
}

//convert long to string and write it
void QTelnet::writeLong(long l)
{
	writeString(QString::number(l));
}

//convert bool to string and write it
void QTelnet::writeBool(bool b)
{
	writeString(b ? "1" : "0");
}


bool QTelnet::readBuffer(unsigned char* buffer, qint64 buffer_length, double timeout_in_seconds) {

    if( !verifyConnected() )
		return false;

    if (timeout_in_seconds < 0.01) timeout_in_seconds = standard_timeout_in_seconds;
    char c;

	flush();
	unsigned long buffer_position = 0;
    QElapsedTimer timer;
    timer.start();
    while ((!timer.hasExpired(timeout_in_seconds * 1000)) && (buffer_position < buffer_length))
    {
        QApplication::processEvents();
        if( read(&c, 1) == 1 )   //for optimization one could read more than one char here
		{
			if (buffer_position < buffer_length) {
            	buffer[buffer_position] = c;
				buffer_position++;
			}
        } else QThread::msleep(1);
	}
    if (buffer_position < buffer_length)
    {
        QMessageBox msgBox;
        //msgBox.setText("QTelnet.cpp: readBuffer timeout");
        //msgBox.exec();
        return false;
    }
    return true;
}

//read and discard input buffer till '*' is encountered
//then read and place characters in buffer till '#' is encountered or till timeout
//return true if success, false otherwise
bool QTelnet::readString(QString &msg, double timeout_in_seconds)
{
    if( !verifyConnected() )
		return false;

    if (timeout_in_seconds == 0) timeout_in_seconds = standard_timeout_in_seconds;
	char c;
	bool bStart = false;
    bool started = false;

    QElapsedTimer timer;
    timer.start();
    flush();
    msg = "";
    while (!timer.hasExpired(timeout_in_seconds * 1000))
    {
        QApplication::processEvents();
		if( read(&c, 1) == 1 )
		{
			if( c == '*' )
			{
				bStart = true;
                started = true;
			}
			else if( c == '#' )
			{
				bStart = false;
				break;
			}
			else if( bStart )
			{
                msg += c;
			}
        } else {
            verifyConnected();
            QThread::msleep(1);
        }
	}
    if (timer.hasExpired(timeout_in_seconds * 1000))
    {
        //QMessageBox msgBox;
        //msgBox.setText("QTelnet.cpp: readString timeout");
        //msgBox.exec();
        qDebug() << "QTelnet.cpp: readString timeout";
        return false;
    }
    return started && (!bStart);
}

//read string and convert it to double if successful
//if successful, return true and place the double in d
//if failed, return false
bool QTelnet::readDouble(double &d, double timeout_in_seconds)
{
	QString msg;
	if( readString(msg, timeout_in_seconds) )
	{
		d = msg.toDouble();
		return true;
	}
	return false;
}

//read string and convert it to long if successful
//if successful, return true and place the double in d
//if failed, return false
bool QTelnet::readLong(long &l, double timeout_in_seconds)
{
	QString msg;
	if( readString(msg, timeout_in_seconds) )
	{
		l = msg.toLong();
		return true;
	}
	return false;
}

//read string and convert it to bool if successful, where string == "1" means true and everything else means false
//if successful, return true and place the double in d
//if failed, return false
bool QTelnet::readBool(bool &b, double timeout_in_seconds)
{
	QString msg;
	if( readString(msg, timeout_in_seconds) )
	{
		b = (msg == "1");
		return true;
	}
	return false;
}




void QTelnet::onReadyRead()
{
/*    qint64 readlength;
    while( (readlength = read(m_buffIncoming, IncommingBufferSize)) != 0 )
    {
        switch( readlength )
		{
		case -1:
			disconnectFromHost();
			break;
		default:
            if( readlength > 0 )
                Q_EMIT(newData(m_buffProcessed, readlength));
			break;
		}
    } */
}

