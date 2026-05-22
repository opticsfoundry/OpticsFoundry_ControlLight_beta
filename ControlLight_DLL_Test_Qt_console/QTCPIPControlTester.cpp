#include "QTCPIPControlTester.h"
#include "ui_QTCPIPControlTester.h"
#include <QScrollBar>

#include "TestSequence.h"

#include "ControlAPI.h"

extern CControlAPI CA;

QTelnetTester::QTelnetTester(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::QTelnetTester),
	telnet(this)
{
	ui->setupUi(this);
    ui->leAddr->setText("192.168.1.155"); //Irene's place:192.168.0.106
	connect( &telnet, SIGNAL(newData(const char*,int)), this, SLOT(addText(const char*,int)) );
	connect( &telnet, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onStateChanged(QAbstractSocket::SocketState)) );
	connect( ui->cbCmd, SIGNAL(command(QString)), this, SLOT(onCommand(QString)));
    InitializeSequencer(&telnet);
}

QTelnetTester::~QTelnetTester()
{
	telnet.disconnectFromHost();
	delete ui;
	ui = Q_NULLPTR;
}

void QTelnetTester::onStateChanged(QAbstractSocket::SocketState s)
{
	switch( s )
	{
	case QAbstractSocket::UnconnectedState:
		setStatusText( tr("Disconnected"), true );
        if (ui) ui->btConnect->setText( tr("Reconnect") );
		break;
	case QAbstractSocket::HostLookupState:
		setStatusText( tr("Resolving DNS %1").arg(telnet.peerName()), true );
		ui->btConnect->setText( tr("Cancel") );
		break;
	case QAbstractSocket::ConnectingState:
		setStatusText( tr("Connected to %1").arg(telnet.peerInfo()), true );
		ui->btConnect->setText( tr("Cancel") );
		break;
	case QAbstractSocket::ConnectedState:
		setStatusText( tr("Connected to %1").arg(telnet.peerInfo()), true );
		ui->btConnect->setText( tr("Disconnect") );
		break;
	case QAbstractSocket::BoundState:
		setStatusText( tr("Bound"), true );
		ui->btConnect->setText( tr("- - - -") );
		break;
	case QAbstractSocket::ListeningState:
		setStatusText( tr("Listening"), true );
		ui->btConnect->setText( tr("- - - -") );
		break;
	case QAbstractSocket::ClosingState:
		setStatusText( tr("Closing"), true );
		ui->btConnect->setText( tr("Forcing close") );
		break;

	}
}

void QTelnetTester::setStatusText(const QString &msg, bool onQTelnetTester, bool AddNewLine)
{
	if( onQTelnetTester )
	{
        addText(msg.toLatin1(), msg.size());
        if ( AddNewLine && ( !msg.endsWith("\n") ))
			addText("\n", 1);
	}
    if (ui) ui->statusBar->showMessage(msg);
}

void QTelnetTester::onCommand(const QString &cmd)
{
	if( telnet.isConnected() )
	{
		telnet.sendData(cmd.toLatin1());
		telnet.sendData("\n");
	}
}

void QTelnetTester::on_btConnect_clicked()
{
	if( telnet.isConnected() )
        telnet.disconnectFromRemote();
	else
        telnet.attemptReconnect();//ui->leAddr->text(), ui->sbPort->value());
}

void QTelnetTester::on_btResetSystem_clicked()
{
    if( CA.isConnected() )
        ResetSystem();
}

void QTelnetTester::on_btExecuteSequence_clicked()
{
    if( CA.isConnected() )
        ExecuteTestSequence();
}

void QTelnetTester::on_btCycleSequence_clicked()
{
    if( CA.isConnected() )
        CycleSequence();
}

void QTelnetTester::on_btCycleSequenceWithIndividualCommandUpdate_clicked()
{
    if( CA.isConnected() )
        CycleSequenceWithIndividualCommandUpdate();
}

void QTelnetTester::on_btStopCycling_clicked()
{
    if( CA.isConnected() )
        StopCyclingButtonPressed();
}


void QTelnetTester::addText(const char *msg, int count)
{
    if (ui) {
        ui->teOutput->insertPlainText( QByteArray(msg, count) );
        ui->teOutput->verticalScrollBar()->setValue(0xFFFFFFF);
    }
}
