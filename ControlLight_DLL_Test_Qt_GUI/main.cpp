#include "QTCPIPControlTester.h"
#include <QApplication>

#include "main.h"


QTelnetTester *TelnetTester = NULL;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
    QTelnetTester MyTelnetTester;

    MyTelnetTester.show();
    TelnetTester = &MyTelnetTester;
    int ret = a.exec();
    return ret;
}
