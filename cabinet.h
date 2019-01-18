#ifndef CABINET_H
#define CABINET_H

#include "trakcore.h"

#include <QObject>
#include <QCoreApplication>
#include <QtDebug>
#include <QTimer>
#include <QDate>
#include <QTime>
#include <qtconcurrentrun.h>
#include <QEventLoop>

#include "hidapi.h"

class CabinetManager;

enum CabinetCommands {
    LockDraws = 0x01,
    UnlockDraws = 0x02,
    CheckDrawsLock = 0x03,
    CheckTools = 0x04,
    CheckDrawOpen = 0x07,
    DrawStatusChange = 0x10,
    CheckVersion = 0x11,
    BuzzerOn = 0x12,
    BuzzerOff = 0x13,
    LightDraw = 0x14,
    ConfirmButton = 0x15,
    UserSwitch = 0x17,
};

class Cabinet : public QObject
{
    Q_OBJECT
public:
    explicit Cabinet(QObject *parent = 0);
    ~Cabinet();

    friend class CabinetManager;


signals:
    void finished();
    void dataReceived(QByteArray ba, int len);
    void drawerStatusChange(QByteArray ba, int len);
    void confirmButtonActivation(QByteArray ba, int len);
    void connected();
    void disconnected();
    void usbReadError();

public slots:


private slots:
    void onDataReceived(QByteArray ba, int len);
    int waitForReturn(uchar* buf, int len);
    void detectedUsbError();

private:

    int writeUsb(unsigned char* buffer, int buflen);
    void waitForUsbDataThread();
    QFuture<void> usbWait;

    int getFirmwareVersion(uchar *rtnbuf, int rtnbuflen);
    int checkToolDrawer(uchar *rtnbuf, int rtnbuflen, uchar draw);
    int lockDrawers();
    int unlockDrawers();
    int checkDrawersLock(uchar *rtnbuf, int rtnbuflen);
    int checkDrawersOpen(uchar* rtnbuf, int rtnbuflen);
    int changeDrawerLights(char draw, char state);
    int setDrawerLightsOn(char draw);
    int setDrawerLightsOff(char draw);
    int buzzerOn(uchar* rtnbuf, int rtnbuflen);
    int buzzerOff(uchar* rtnbuf, int rtnbuflen);
    int userSwitch(uchar* rtnbuf, int rtnbuflen);
    hid_device* usbConnect();
    hid_device* usbConnectWait();
    hid_device* usbConnectDontWait();
    bool cabinetCheckAndConnect();
    bool cabinetConnected();

    quint64 readMemoryUsage();

    void killingCabinet();
    bool killingUsb;


    hid_device* hidHandle;
    ushort vid;
    ushort pid;
    uchar threadbuf[256];
    uchar responseBuf[256];
    int responseLen;

    void delay();


#define VID 0x4d8
#define PID 0x3f
#define ReportLength 65
#define ReportId 0

#ifdef nocabinet
private:
    void sendUnsolMsg(int drwnum);
public slots:
    void openDrawer();
    void openDrawer(int drwnum);
    void closeDrawer(int drwnum);
    void toolTest(int drwnum, int toolnum, int state);

#endif

};

#endif // CABINET_H
