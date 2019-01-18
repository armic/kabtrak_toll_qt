
#include <windows.h>
#include <stdio.h>
#include <psapi.h>

#include "cabinet.h"

#ifndef nocabinet

Cabinet::Cabinet(QObject *parent) : QObject(parent)
{

    hidHandle = NULL;

    usbConnectDontWait();
    if (!cabinetConnected())
    {
        qCritical() << "Cabinet::Cabinet() Could not connect to usb";
        return;
    }

    killingUsb = false;

    // setup connection  events
    // this thread runs continuously waiting for responses and unsolicited data from usb
    connect(this, &Cabinet::dataReceived,this, &Cabinet::onDataReceived);
    connect(this, &Cabinet::usbReadError,this, &Cabinet::detectedUsbError);
    usbWait = QtConcurrent::run(this, &Cabinet::waitForUsbDataThread);


}


Cabinet::~Cabinet()
{
    if (hidHandle)
    {
        hid_close(hidHandle);
        hid_exit();
    }
    hidHandle = NULL;

    usbWait.cancel();

}

//
//  Read the current process memory usage
//
quint64 Cabinet::readMemoryUsage()
{
    HANDLE processHandle;
    PROCESS_MEMORY_COUNTERS pmc;

    if ((processHandle = GetCurrentProcess()) != NULL)
    {
        if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc)))
        {
            //qDebug() << "Cabinet::readMemoryUsage() PagefileUsage" << pmc.PagefileUsage << sizeof(pmc.PagefileUsage);
            return pmc.PagefileUsage;
        }
    }
    return 0;
}



void Cabinet::killingCabinet()
{
    if (hidHandle)
    {
        hid_close(hidHandle);
        hid_exit();
    }
    hidHandle = NULL;
    killingUsb = true;
}


hid_device* Cabinet::usbConnect()
{
    return usbConnectWait();
}



hid_device* Cabinet::usbConnectDontWait()
{

    if (hidHandle)
    {
        hid_close(hidHandle);
    }
    hidHandle = NULL;

    qInfo() << "Cabinet::usbConnectDontWait() Trying to connect";
    emit disconnected();


    int res;

    // initialise usb
    // We use hidapi for access to usb
    hidHandle = hid_open(VID, PID, 0);
    if (!hidHandle)
    {
        qWarning() << "Cabinet::usbConnectDontWait() usb open failed";
        //emit disconnected();
    }
    else
    {
        qInfo() << "Cabinet::usbConnectDontWait() usb device opened";
        emit connected();

        // set blocking, ie wait for response
        res = hid_set_nonblocking(hidHandle, 0);
        if (res < 0)
        {
            qWarning() << "Cabinet::usbConnectDontWait() Failed to set blocking" << res << QString::fromWCharArray(hid_error(hidHandle));
        }
        else {
            qDebug() << "Cabinet::usbConnectDontWait() blocking set ";
        }
    }
    return hidHandle;
}





hid_device* Cabinet::usbConnectWait()
{

    if (hidHandle)
    {
        hid_close(hidHandle);
    }
    hidHandle = NULL;

    qInfo() << "Cabinet::usbConnect() Trying to connect";
    emit disconnected();

    while (hidHandle == NULL)
    {

        int res;

        // initialise usb
        // We use hidapi for access to usb
        hidHandle = hid_open(VID, PID, 0);
        if (!hidHandle)
        {
            //qWarning() << "Cabinet::usbConnect() usb open failed";
            //emit disconnected();
        }
        else
        {
            qDebug() << "Cabinet::usbConnect() usb device opened";
            emit connected();

            // set blocking, ie wait for response
            res = hid_set_nonblocking(hidHandle, 0);
            // set nonblocking, ie do not wait for response
    //        res = hid_set_nonblocking(hidHandle, 1);
            if (res < 0)
            {
                qWarning() << "Cabinet::usbConnect() Failed to set blocking" << res << QString::fromWCharArray(hid_error(hidHandle));
            }
            else {
                qDebug() << "Cabinet::usbConnect() blocking set ";
            }
        }
        QThread::msleep(100);
        QCoreApplication::processEvents();
    }
    qInfo() << "Cabinet::usbConnect() Connected";
    return hidHandle;
}



bool Cabinet::cabinetCheckAndConnect()
{
    if (hidHandle)
    {
        return true;
    }
    else if (!usbConnect())
    {
        return false;
    }
    return true;
}



bool Cabinet::cabinetConnected()
{
    if (hidHandle)
    {
        return true;
    }
    else
    {
        return false;
    }
}



// write data to usb in cabinet
int Cabinet::writeUsb(unsigned char* buffer, int buflen)
{
(void)buflen;

    QByteArray array((const char*)buffer, buflen < 23 ? buflen :23);
    qInfo() << "Cabinet::writeUsb()       >>>>>>>> " << QString(array.toHex());

    if (!hidHandle)
    {
        qWarning() << "Cabinet::writeUsb() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    int res = hid_write(hidHandle,buffer,ReportLength);
    if (res < 0){
        // can not write, try and reconnect
        qWarning() << "writeUsb:Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
    }
    qDebug() << "Cabinet::writeUsb() returned from hid_write, result: " << res;
    return res;
}



//
//  wait for data from usb. This function runs in another thread
//
void Cabinet::waitForUsbDataThread()
{

    if (!hidHandle)
    {
        emit usbReadError();
        return;     // not initialised yet.
    }

    int error = 0;
    while(error == 0)
    {
        int res = 0;
        //qDebug() << "Cabinet::waitForUsbDataThread(): read request";
        if (hidHandle == NULL)
        {
            break;
        }
        res = hid_read(hidHandle,threadbuf,ReportLength);
        //qDebug() << "Cabinet::waitForUsbDataThread(): read return" << res;
        if (res == 0)
        {
            //qDebug() << ("Cabinet::waitForUsbDataThread:waiting...");
        }
        if (res < 0)
        {
            qWarning() << "Cabinet::waitForUsbDataThread:Failed to read";
            emit usbReadError();
            error = 1;
        }
        // return data read
        if (res > 0)
        {
            // send data to slot
            // This should make a deep copy of the threadbuf data
            QByteArray ba = QByteArray((const char*)&threadbuf, sizeof(threadbuf));
            emit dataReceived(ba, res);
        }
    }
    qWarning() << "Cabinet::waitForUsbDataThread:unexpected exit";
}



void Cabinet::detectedUsbError()
{
    qDebug() << "Cabinet::detectedUsbError()";
    usbWait.cancel();
    emit disconnected();
    if (usbConnect() > 0)
    {
        qDebug() << "Cabinet::detectedUsbError() WaitForFinished";
        usbWait.waitForFinished();
        qDebug() << "Cabinet::detectedUsbError() run thread";
        usbWait = QtConcurrent::run(this, &Cabinet::waitForUsbDataThread);
    }
    qDebug() << "Cabinet::detectedUsbError() Exit";
}



void Cabinet::onDataReceived(QByteArray ba, int len)
{
    // get ptr to data in QByteArray
    uchar* buf = (uchar*)ba.data();

    // received data from usb
    QByteArray array((const char*)buf, len < 23 ? len :23);
    qInfo() << "Cabinet::onDataReceived() <<<<<<<< " << QString(array.toHex());

    quint64 memusage = readMemoryUsage();
    qDebug() << "Cabinet::onDataReceived() PagefileUsage" << memusage;

    // check if it is unsolicited
    if (*buf == DrawStatusChange)
    {
        //process unsolicited message
        qDebug() << "Cabinet::onDataReceived:received unsol data";
        // emit signal
        emit drawerStatusChange(ba, len);
    }
    else if (*buf == ConfirmButton)
    {
        //process unsolicited message
        qDebug() << "Cabinet::onDataReceived:received confirm button ";
        // emit signal
        emit confirmButtonActivation(ba, len);
    }
    else
    {
        // response from a request, we hope
        qDebug() << "Cabinet::onDataReceived:received response data";

        // copy the response data before emitting finish
        memcpy (responseBuf, buf, len);
        responseLen = len;
        emit finished();        // trigger waitForReturn event loop
    }
}



int Cabinet::waitForReturn(uchar* buf, int len)
{

QEventLoop eloop;
QTimer tmr;

    tmr.setSingleShot(true);
    connect(&tmr, &QTimer::timeout, &eloop, &QEventLoop::quit);
    connect(this, &Cabinet::finished, &eloop, &QEventLoop::quit);


    tmr.start(5000); // 5s timeout
    eloop.exec();

    if(tmr.isActive()){
        // received message
        qDebug() << "Cabinet::waitForReturn:received data";
        // return response data
        memcpy(buf, responseBuf, len > responseLen ? responseLen : len);
        tmr.stop();     // stop timer
        return responseLen;
    } else {
       // timeout
        qWarning() << "Cabinet::waitForReturn:timeout";
        return -1;
    }
}


void Cabinet::delay()
{
    QThread::msleep(10);
}





//
// Command - Get Firmaware version
//
int Cabinet::getFirmwareVersion(uchar* rtnbuf, int rtnbuflen)
{
uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::getFirmwareVersion() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }
    memset(buf, 0, rtnbuflen);
    buf[0] = 0x00;
    buf[1] = CheckVersion;

    int res = writeUsb(buf,ReportLength);
    if (res < 0){
        // can not write, display warning
        qWarning() << "Cabinet::getFirmwareVersion() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }

    res = waitForReturn(rtnbuf, rtnbuflen);

    return res;
}




//
// Command - Check tools draw
//
int Cabinet::checkToolDrawer(uchar* rtnbuf, int rtnbuflen, uchar draw)
{
uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::checkToolDrawer() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    memset(buf, 0, rtnbuflen);
    buf[0] = 0x00;
    buf[1] = CheckTools;
    buf[2] = draw;

    int res = writeUsb(buf,ReportLength);
    if (res < 0){
        // can not write
        qWarning() << "Cabinet::checkToolDrawer() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    return res;
}



//
//  lock the draws
//
int Cabinet::lockDrawers()
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::lockDrawers() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LockDraws;

    int res = writeUsb(buf,2);
    return res;
}


//
//  lock the draws
//
int Cabinet::unlockDrawers()
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::unlockDrawers() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = UnlockDraws;

    int res = writeUsb(buf,2);
    return res;
}


//
//  check if draws are locked and return result.
//      TODO: should return bool ???
//
int Cabinet::checkDrawersLock(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::checkDrawersLock() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = CheckDrawsLock;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::checkDrawersLock() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}



//
//  check if draws are locked and return result.
//      TODO: should return bool ???
//
int Cabinet::checkDrawersOpen(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::checkDrawersOpen() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = CheckDrawOpen;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::checkDrawersOpen() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}



//
//  set the tool sensors on or off.
//      TODO: should return bool ???
//
int Cabinet::changeDrawerLights(char draw, char state)
{
uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::changeDrawerLights() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LightDraw;
    buf[2] = draw;
    buf[3] = state;

    int res = writeUsb(buf, 4);
    return res;
}


int Cabinet::setDrawerLightsOn(char draw)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::setDrawerLightsOn() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LightDraw;
    buf[2] = draw;
    buf[3] = 1;

    int res = writeUsb(buf, 4);
    return res;
}

int Cabinet::setDrawerLightsOff(char draw)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::setDrawerLightsOff() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LightDraw;
    buf[2] = draw;
    buf[3] = 0;

    int res = writeUsb(buf, 4);
    return res;
}


//
//  Buzzer on
//      TODO: should return bool ???
//
int Cabinet::buzzerOn(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::buzzerOn() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = BuzzerOn;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::buzzerOn() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}


//
//  Buzzer off
//      TODO: should return bool ???
//
int Cabinet::buzzerOff(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::buzzerOff() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = BuzzerOff;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::buzzerOff() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}



//
//  user switch
//      TODO: should return bool ???
//
int Cabinet::userSwitch(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::userSwitch() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = UserSwitch;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::userSwitch() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}


#else

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
//  Simulate usb driver
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

static bool drawerslocked = true;
static uchar drawerstatus[10] = {0,0,0,0,0,0,0,0,0,0};
static uchar draweropen[10] = {0xff,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff};
static bool buzzeron = false;
static uchar userSw1 = 0;
static uchar userSw2 = 0;
static uchar userSw3 = 0;
struct bm {
    uchar drwbm[19];
};

static bm toolsbitmap[10];
static int drawernumber;
static QTimer* tmr;


Cabinet::Cabinet(QObject *parent) : QObject(parent)
{

    hidHandle = NULL;
    usbConnect();

    // setup connection  events
    // this thread runs continuously waiting for responses and unsolicited data from usb
    //connect(this, &Cabinet::dataReceived,this, &Cabinet::onDataReceived);
    //QFuture<void> usbWait = QtConcurrent::run(this, &Cabinet::waitForUsbDataThread);

    // filll bitmap buffers
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 19; j++)
        {
            toolsbitmap[i].drwbm[j] = 0xff;
        }
    }
}

Cabinet::~Cabinet()
{

}

void Cabinet::killingCabinet()
{
    if (hidHandle)
    {
        hid_close(hidHandle);
        hid_exit();
    }
    hidHandle = NULL;
    killingUsb = true;
}




hid_device* Cabinet::usbConnect()
{


    hidHandle = (hid_device*)1;
    return hidHandle;
}



bool Cabinet::cabinetCheckAndConnect()
{
    if (hidHandle)
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool Cabinet::cabinetConnected()
{
    if (hidHandle)
    {
        return true;
    }
    else
    {
        return false;
    }
}




void Cabinet::sendUnsolMsg(int drwnum)
{
    QByteArray array;
    qDebug() << "Simulator::Cabinet::sendUnsolMsg() drawer number " << drwnum;
    responseLen = ReportLength;
    memset(threadbuf, 0x00, responseLen);
    memset(threadbuf, 0xff, 22);
    threadbuf[0] = DrawStatusChange;
    threadbuf[1] = drwnum;
    threadbuf[2] = drawerstatus[drwnum-1];
    if ((drawerstatus[drwnum-1] & 0x10) > 0)
    {
        // leds on
        memcpy (threadbuf+3, toolsbitmap[drwnum-1].drwbm, 19);
    }
    array.append((const char*)threadbuf, 22);
    qDebug() << "Simulator::Cabinet::sendUnsolMsg() DrawStatusChange response " << QString(array.toHex());

    QByteArray ba = QByteArray((const char*)&threadbuf, sizeof(threadbuf));
    onDataReceived(ba, responseLen);
}


void Cabinet::openDrawer()
{
    int drwnum = drawernumber;
    qDebug() << "Simulator::Cabinet::openDrawer()  " << drwnum;

    draweropen[drwnum-1] = 1;
    drawerstatus[drwnum-1] = drawerstatus[drwnum-1] | 0x01;
    sendUnsolMsg(drwnum);
}

void Cabinet::openDrawer(int drwnum)
{
    qDebug() << "Simulator::Cabinet::openDrawer()  " << drwnum;

    draweropen[drwnum-1] = 1;
    drawerstatus[drwnum-1] = drawerstatus[drwnum-1] | 0x01;
    sendUnsolMsg(drwnum);
}

void Cabinet::closeDrawer(int drwnum)
{
    draweropen[drwnum-1] = 0;
    drawerstatus[drwnum-1] = drawerstatus[drwnum-1] & 0xfe;
    sendUnsolMsg(drwnum);
}



void Cabinet::toolTest(int drwnum, int toolnum, int state)
{
    int byteofst;
    int bitofst;
    byteofst = (toolnum-1) / 8;
    bitofst = 7-((toolnum-1)%8);
    // tool present, update bitmap
    toolsbitmap[drwnum-1].drwbm[byteofst] ^= (0x01 << bitofst);
    //struct bm bitm = toolsbitmap[drwnum-1];
    //uchar byt = bitm[byteofst];
// |= (0x01 << bitofst)
    sendUnsolMsg(drwnum);
}


// write data to usb in cabinet
int Cabinet::writeUsb(unsigned char* buffer, int buflen)
{
(void)buflen;

//    static bool drawerslocked = true;
//    static uchar drawerstatus[10] = {0,0,0,0,0,0,0,0,0,0};
//    static uchar draweropen[10] = {0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
//    static bool buzzeron = false;
//    static uchar userSw1 = 0;
//    static uchar userSw2 = 0;
//    static uchar userSw3 = 0;
//    struct bm {
//        uchar drwbm[19];
//    };

//    static bm toolsbitmap[10];

    int cmd = buffer[1];
    int drawernum = buffer[2];
    QByteArray array;
    //QTimer tmr;

    switch(cmd)
    {
        case LockDraws :
            qDebug() << "Simulator::Cabinet::writeUsb() LockDrawers ";
            drawerslocked = true;
            break;

        case UnlockDraws :
            qDebug() << "Simulator::Cabinet::writeUsb() UnlockDrawers ";
            drawerslocked = false;

//            tmr = new QTimer(this);
//                tmr->setSingleShot(true);
//                drawernumber = 6;
//                connect(tmr, &QTimer::timeout, this, &Cabinet::openDrawer);
                //connect(tmr, SIGNAL(timeout()), this, SLOT(openDrawer(2)));
//                tmr->start(5000); // 5s timeout
            //QTimer::singleShot(4000, this, SLOT(openDrawer(drawernum)));
            //openDrawer(2);
            break;

        case CheckDrawsLock :
            responseLen = 2;
            memset(responseBuf, 0x00, responseLen);
            memset(responseBuf, 0xff, 22);
            responseBuf[0] = CheckDrawsLock;
            if (drawerslocked)
                responseBuf[1] = 1;
            else
                responseBuf[1] = 0;
            qDebug() << "Simulator::Cabinet::writeUsb() CheckDrawsLock Drawers are " << (drawerslocked ? "locked" : "unlocker");
            break;

        case CheckTools :
                responseLen = ReportLength;
                memset(responseBuf, 0x00, responseLen);
                memset(responseBuf, 0xff, 22);
                responseBuf[0] = CheckTools;
                responseBuf[1] = drawernum;
                responseBuf[2] = drawerstatus[drawernum-1];
                if ((drawerstatus[drawernum-1] & 0x10) > 0)
                {
                    // lights on
                    memcpy (responseBuf+3, &toolsbitmap[drawernum-1].drwbm[0], 19);
                }
                array.append((const char*)responseBuf, 22);
                qDebug() << "Simulator::Cabinet::writeUsb() CheckTools response " << QString(array.toHex());
                break;

        case CheckDrawOpen :
                responseLen = 11;
                memset(responseBuf, 0x00, responseLen);
                memset(responseBuf, 0xff, 22);
                responseBuf[0] = CheckDrawOpen;
                responseBuf[1] = draweropen[0];
                responseBuf[2] = draweropen[1];
                responseBuf[3] = draweropen[2];
                responseBuf[4] = draweropen[3];
                responseBuf[5] = draweropen[4];
                responseBuf[6] = draweropen[5];
                responseBuf[7] = draweropen[6];
                responseBuf[8] = draweropen[7];
                responseBuf[9] = draweropen[8];
                responseBuf[10] = draweropen[9];
                array.append((const char*)responseBuf, 11);
                qDebug() << "Simulator::Cabinet::writeUsb() CheckDrawOpen response " << QString(array.toHex());
                break;

        case DrawStatusChange :
                responseLen = ReportLength;
                memset(responseBuf, 0x00, responseLen);
                memset(responseBuf, 0xff, 22);
                responseBuf[0] = DrawStatusChange;
                responseBuf[1] = drawernum;
                responseBuf[2] = drawerstatus[drawernum-1];
                if ((drawerstatus[drawernum-1] & 0x10) > 0)
                {
                    // leds on
                    memcpy (responseBuf+3, toolsbitmap[drawernum-1].drwbm, 19);
                }
                array.append((const char*)responseBuf, 22);
                qDebug() << "Simulator::Cabinet::writeUsb() DrawStatusChange response " << QString(array.toHex());
                break;

        case CheckVersion :
                responseLen = ReportLength;
                memset(responseBuf, 0x00, responseLen);
                memset(responseBuf, 0xff, 22);
                responseBuf[0] = CheckVersion;
                responseBuf[1] = 0x03;
                responseBuf[2] = 0x02;
                array.append((const char*)responseBuf, 3);
                qDebug() << "Simulator::Cabinet::writeUsb() CheckVersion response " << QString(array.toHex());
                break;

        case BuzzerOn :
            buzzeron = true;
            qDebug() << "Simulator::Cabinet::writeUsb() BuzzerOn ";
            break;

        case BuzzerOff :
            buzzeron = false;
            qDebug() << "Simulator::Cabinet::writeUsb() BuzzerOff ";
            break;

        case LightDraw :
            if (buffer[3] == 0)
            {
                // light off
                drawerstatus[drawernum-1] = drawerstatus[drawernum-1] & 0xef;
            }
            else
            {
                // light on
                drawerstatus[drawernum-1] = drawerstatus[drawernum-1] | 0x10;
            }
            qDebug() << "Simulator::Cabinet::writeUsb() LightDraw Drawer" << drawernum << " are turned " << (buffer[3] == 0 ? "off" : "on");
            break;

        case UserSwitch :
                responseLen = ReportLength;
                memset(responseBuf, 0x00, responseLen);
                memset(responseBuf, 0xff, 22);
                responseBuf[0] = UserSwitch;
                responseBuf[1] = userSw1;
                responseBuf[2] = userSw2;
                responseBuf[3] = userSw3;
                array.append((const char*)responseBuf, 4);
                qDebug() << "Simulator::Cabinet::writeUsb() UserSwitch response " << QString(array.toHex());
                break;

    }
    return responseLen;

}



//
//  wait for data from usb. This function runs in another thread
//
void Cabinet::waitForUsbDataThread()
{

}



void Cabinet::detectedUsbError()
{
    qDebug() << "Cabinet::detectedUsbError()";
    qDebug() << "Cabinet::detectedUsbError() Exit";
}



void Cabinet::onDataReceived(QByteArray ba, int len)
{
    // received data from usb
    // get ptr to data in QByteArray
    uchar* buf = (uchar*)ba.data();
    QByteArray array((const char*)buf, len);
    qDebug() << "Cabinet::onDataReceived:Data received: " << QString(array.toHex());

    // check if it is unsolicited
    if (*buf == DrawStatusChange)
    {
        //process unsolicited message
        qDebug() << "Cabinet::onDataReceived:received unsol data";
        // emit signal
        emit drawerStatusChange(ba, len);
    }
    else if (*buf == ConfirmButton)
    {
        //process unsolicited message
        qDebug() << "Cabinet::onDataReceived:received confirm button ";
        // emit signal
        emit confirmButtonActivation(ba, len);
    }
    else
    {
        // response from a request, we hope
        qDebug() << "Cabinet::onDataReceived:received response data";

        // copy the response data before emitting finish
//        memcpy (responseBuf, buf, len);
//        responseLen = len;
//        emit finished();        // trigger waitForReturn event loop
    }



}



int Cabinet::waitForReturn(uchar* buf, int len)
{


    // return response data
    memcpy(buf, responseBuf, len > responseLen ? responseLen : len);
    return responseLen;
}


void Cabinet::delay()
{
    QThread::msleep(10);
}





//
// Command - Get Firmaware version
//
int Cabinet::getFirmwareVersion(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

        if (!hidHandle)
        {
            qWarning() << "Cabinet::getFirmwareVersion() Not connecteded to USB";
            if (!usbConnect())
                return -1;
        }
        memset(buf, 0, rtnbuflen);
        buf[0] = 0x00;
        buf[1] = CheckVersion;

        int res = writeUsb(buf,ReportLength);
        if (res < 0){
            // can not write, display warning
            qWarning() << "Cabinet::getFirmwareVersion() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
            return res;
        }

        res = waitForReturn(rtnbuf, rtnbuflen);
        return res;

}




//
// Command - Check tools draw
//
int Cabinet::checkToolDrawer(uchar* rtnbuf, int rtnbuflen, uchar draw)
{
uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::checkToolDrawer() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    memset(buf, 0, rtnbuflen);
    buf[0] = 0x00;
    buf[1] = CheckTools;
    buf[2] = draw;

    int res = writeUsb(buf,ReportLength);
    if (res < 0){
        // can not write
        qWarning() << "Cabinet::checkToolDrawer() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    return res;
}



//
//  lock the draws
//
int Cabinet::lockDrawers()
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::lockDrawers() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LockDraws;

    int res = writeUsb(buf,2);
    return res;
}


//
//  lock the draws
//
int Cabinet::unlockDrawers()
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::unlockDrawers() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = UnlockDraws;

    int res = writeUsb(buf,2);
    return res;
}


//
//  check if draws are locked and return result.
//      TODO: should return bool ???
//
int Cabinet::checkDrawersLock(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::checkDrawersLock() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = CheckDrawsLock;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::checkDrawersLock() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}



//
//  check if draws are locked and return result.
//      TODO: should return bool ???
//
int Cabinet::checkDrawersOpen(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::checkDrawersOpen() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = CheckDrawOpen;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::checkDrawersOpen() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}



//
//  set the tool sensors on or off.
//      TODO: should return bool ???
//
int Cabinet::changeDrawerLights(char draw, char state)
{
uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::changeDrawerLights() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LightDraw;
    buf[2] = draw;
    buf[3] = state;

    int res = writeUsb(buf, 4);
    return res;
}


int Cabinet::setDrawerLightsOn(char draw)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::setDrawerLightsOn() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LightDraw;
    buf[2] = draw;
    buf[3] = 1;

    int res = writeUsb(buf, 4);
    return res;
}

int Cabinet::setDrawerLightsOff(char draw)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::setDrawerLightsOff() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = LightDraw;
    buf[2] = draw;
    buf[3] = 0;

    int res = writeUsb(buf, 4);
    return res;
}


//
//  Buzzer on
//      TODO: should return bool ???
//
int Cabinet::buzzerOn(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::buzzerOn() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = BuzzerOn;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::buzzerOn() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}


//
//  Buzzer off
//      TODO: should return bool ???
//
int Cabinet::buzzerOff(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::buzzerOff() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = BuzzerOff;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::buzzerOff() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}



//
//  user switch
//      TODO: should return bool ???
//
int Cabinet::userSwitch(uchar* rtnbuf, int rtnbuflen)
{
    uchar buf[256];

    if (!hidHandle)
    {
        qWarning() << "Cabinet::userSwitch() Not connecteded to USB";
        if (!usbConnect())
            return -1;
    }

    buf[0] = ReportId;
    buf[1] = UserSwitch;

    int res = writeUsb(buf,2);

    if (res < 0){
        // can not write
        qWarning() << "Cabinet::userSwitch() Failed write" << res << QString::fromWCharArray(hid_error(hidHandle));
        return res;
    }
    res = waitForReturn(rtnbuf, rtnbuflen);
    // check response number is same as request

    return res;
}



#endif

