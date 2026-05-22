#ifndef QTCPIPCONTROLTESTER_H
#define QTCPIPCONTROLTESTER_H

#include <QMainWindow>
#include "QTCPIP.h"

namespace Ui
{
	class QTelnetTester;
}

class QTelnetTester : public QMainWindow
{
	Q_OBJECT

	Ui::QTelnetTester *ui;
	QTelnet telnet;

public:
	explicit QTelnetTester(QWidget *parent = 0);
	~QTelnetTester();

private slots:
	void onStateChanged(QAbstractSocket::SocketState s);

	void on_btConnect_clicked();
    void on_btExecuteSequence_clicked();
    void on_btResetSystem_clicked();
    void on_btCycleSequence_clicked();
    void on_btCycleSequenceWithIndividualCommandUpdate_clicked();
	void on_btStopCycling_clicked();

    void onCommand(const QString &cmd);

public slots:
    void setStatusText(const QString &msg, bool onMainWindow = false, bool AddNewLine = true);
	void addText(const char *msg, int count);
};

#endif // QTCPIPCONTROLTESTER_H
