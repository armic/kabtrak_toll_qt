#ifndef CABINETMANAGER_H
#define CABINETMANAGER_H

#include <QObject>
#include <QString>

#include "maincontroller.h"
#include "cabinet.h"



#define BitmapLength 19  // in bytes
#define MaxDrawers 10

//class Cabinet;

struct DrawerStatus
{

    bool present;
    bool initialised;
    int drawerNumber;       // from 1 to 10
    uchar currentLdrBitmap[BitmapLength];
    uchar toolsPresentBitmap[BitmapLength];
    uchar statusBitmap;
    bool drawerOpen;
    bool lightOn;
    bool slavePresent;
    bool slaveComms;

};

class CabinetManager : public QObject
{
    Q_OBJECT
public:
    explicit CabinetManager(QObject *parent = 0);

    void initCabinetManager();
    int checkDrawersPresent();


    // cabinet access functions
    QString getFirmwareVersion();
    int checkToolDrawer(uchar* rtnbuf, int rtnbuflen, uchar drawer);
    int lockDrawers();
    int unlockDrawers();
    int checkDrawersLock(uchar *rtnbuf, int rtnbuflen);
    int checkDrawersOpen(uchar* rtnbuf, int rtnbuflen);
    int changeDrawerLights(char draw, char state);
    int setDrawerLightsOn(char draw);
    int setDrawerLightsOff(char draw);
    bool cabinetConnected();

    bool turnLightsOnAndCheck(int dnum);
    bool checkLightsOn(int dnum);

    int getToolNumber(int byt, int bit);
    void fromToolNumber(int toolnum, int* byt, int* bit);

    DrawerStatus* getDrawerStatus(int drwnum);
    void updateDrawerStatus(int drwnum);
    bool isToolAvailable(int toolnum, int drwnum);
    bool drawerExist(int drwnum);
    void disconnectCabinet();



signals:
    void drawerOpenChanged(int dnum, DrawerStatus* drawer);
    void drawerLightChanged(int dnum, DrawerStatus* drawer);
    void drawerSlaveChanged(int dnum, DrawerStatus* drawer);
    void drawerSlaveCommsChanged(int dnum, DrawerStatus* drawer);
    void drawerToolChanged(int dnum, int toolnum, bool out);
    void confirmButtonPushed();
    void connected();
    void disconnected();

public slots:


private slots:
    void drawerStatusChanged(QByteArray ba, int len);
    void confirmButtonActivated(QByteArray ba, int len);
    void cabinetHasConnected();
    void cabinetHasDisconnected();



private:

    DrawerStatus drawerArray[MaxDrawers];
    int initDrawers();
    bool drawersInit;
    int initDrawer(DrawerStatus *drawer, int drwnum);
    Cabinet* cabinetInstance;

#ifdef TRAK_TESTKEYS
public:
    void openDrawerTest(int);
    void closeDrawerTest(int);
    void removeToolTest(int drwnum, int toolnum, int state);
#endif

};


//
// class for iterating through the tools in the drawer status struct
//
class ToolsAvailable
{
public:
    ToolsAvailable(DrawerStatus *drw);
    int first();
    int next();
    int state();

private:
    uchar bit;
    int byt;
    int bitcnt;
    DrawerStatus* drawer;
};

#endif // CABINETMANAGER_H
