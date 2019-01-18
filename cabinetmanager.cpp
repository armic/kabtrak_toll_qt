
#include <QByteArray>

#include "cabinetmanager.h"
//#include "cabinet.h"
#include "datamanager.h"


CabinetManager::CabinetManager(QObject *parent) : QObject(parent)
{



    // connect to the cabinet hardware via usb
    cabinetInstance = new Cabinet();

    if (!cabinetConnected())
    {
        MyMessageBox msgBox;
        msgBox.setText("Could not connect to cabinet. Closing Kabtrak");
        msgBox.setTimeoutValue(5);
        msgBox.exec();
        qCritical() << "CabinetManager::CabinetManager() Bye Bye";
        MainController::getMainControllerInstance()->requestExit(Trak::NoCabinetConnection);
    }


    // connect cabinet signals
    connect(cabinetInstance, &Cabinet::connected, this, &CabinetManager::cabinetHasConnected);
    connect(cabinetInstance, &Cabinet::disconnected, this, &CabinetManager::cabinetHasDisconnected);
    cabinetInstance->cabinetCheckAndConnect();

    // see if we can read the firmware version number
    QString str = getFirmwareVersion();
    qDebug() << "Version number is:" << str;

}


void CabinetManager::cabinetHasConnected()
{
    emit connected();
}

void CabinetManager::cabinetHasDisconnected()
{
    emit disconnected();
}



void CabinetManager::disconnectCabinet()
{
    cabinetInstance->killingCabinet();
    //delete cabinetInstance;
    qCritical() << "CabinetManager::disconnectCabinet() Deleted cabinet instance";
}


//
// init this cabinet manager
//
void CabinetManager::initCabinetManager()
{
    if (initDrawers() < 0)
    {
        drawersInit = false;
        // need to try again later??
    }
    else
    {
        drawersInit = true;
    }

    // set signal/slots for the unsloicited data changes
    bool rsp = connect(cabinetInstance, &Cabinet::drawerStatusChange,this, &CabinetManager::drawerStatusChanged);
    if (!rsp)
        qWarning() << "CabinetManager::initCabinetManager connection to drawStatusChanged failed!!!";
    else
        qDebug() << "CabinetManager::initCabinetManager connection to drawStatusChanged Okay";

    connect(cabinetInstance, &Cabinet::confirmButtonActivation,this, &CabinetManager::confirmButtonActivated);

}




int CabinetManager::checkDrawersPresent()
{
uchar buf[100];

    // Find the drawers present
    if (cabinetInstance->checkDrawersOpen(buf, sizeof(buf)) < 0)
        return -1;
    for (int i = 0; i < MaxDrawers; i++)
    {
        if (buf[i+1] == 0xff)
        {
            drawerArray[i].present = false;
            drawerArray[i].initialised = false;
            qDebug() << "CabinetManager::checkDrawersPresent() draw does not exist " << i+1;
        }
        else
        {
            // drawer present so setup tool status
            drawerArray[i].present = true;
            drawerArray[i].initialised = false;
            qDebug() << "CabinetManager::checkDrawersPresent() draw does exists    " << i+1;
        }
    }
    return 0;
}





//
//  Initialise the drawer and tool status for each drawer
//
int CabinetManager::initDrawers()
{
uchar buf[100];

    // Find the drawers present
    if (cabinetInstance->checkDrawersOpen(buf, sizeof(buf)) < 0)
        return -1;
    for (int i = 0; i < MaxDrawers; i++)
    {
        if (buf[i+1] == 0xff)
        {
            drawerArray[i].present = false;
            drawerArray[i].initialised = false;
        }
        else
        {
            // drawer present so setup tool status
            drawerArray[i].present = true;
            int res = initDrawer(&drawerArray[i], i+1);
            if (res < 0)
            {
                // problem initialising so mark as not present
                drawerArray[i].present = false;
                drawerArray[i].initialised = false;
            }
            QThread::msleep(500);
        }
    }
    return 0;
}


int CabinetManager::initDrawer(DrawerStatus* drawer, int drwnum)
{
uchar buf[100];

    if (drwnum < 1 || drwnum > MaxDrawers)
    {
        // error error
        return -1;
    }

    drawer->drawerNumber = drwnum;

    // turn drawer lights on
//    if (CabinetManager::setDrawerLightsOn(drwnum) < 0)
//    {
//        return -1;
//    }
//    // turn drawer lights on
//    if (turnLightsOnAndCheck(drwnum) == false)
//    {
//        qWarning() << "CabinetManager::initDrawer() drawer lights did not turn on, try once more";
//        QThread::sleep(2);
//        if (turnLightsOnAndCheck(drwnum) == false)
//        {
//            qWarning() << "CabinetManager::initDrawer() drawer lights did not turn on AGAIN, exit";
//            return -1;
//        }
//        qDebug() << "CabinetManager::initDrawer() drawer lights turned on";
//    }
    // turn drawer lights on
    setDrawerLightsOn(drwnum);
    QThread::msleep(500);
    if (checkLightsOn(drwnum) == false)
    {
        qWarning() << "CabinetManager::initDrawer() drawer lights did not turn on, try once more";
        setDrawerLightsOn(drwnum);
        QThread::sleep(1);
        if (checkLightsOn(drwnum) == false)
        {
            qWarning() << "CabinetManager::initDrawer() drawer lights did not turn on AGAIN, exit";
            return -1;
        }
        qDebug() << "CabinetManager::initDrawer() drawer lights turned on";
    }

    int res = checkToolDrawer(buf, sizeof(buf), drwnum);
    if (res > 0)
    {
        QByteArray array((const char*)buf, 20);
        qDebug() << "CabinetManager::initDrawer Data received: " << QString(array.toHex());

        // copy the array to the current ldr buffer
        memcpy(drawer->currentLdrBitmap, buf+3, BitmapLength);

        // set the status vars
        drawer->statusBitmap = buf[2];
        if (drawer->statusBitmap & 0x01)
            drawer->drawerOpen = true;
        else
            drawer->drawerOpen = false;

        if (drawer->statusBitmap & 0x10)
            drawer->lightOn = true;
        else
            drawer->lightOn = false;

        if (drawer->statusBitmap & 0x20)
            drawer->slavePresent = true;
        else
            drawer->slavePresent = false;

        if (drawer->statusBitmap & 0x40)
            drawer->slaveComms = true;
        else
            drawer->slaveComms = false;



        // setup the tools present bitmap
        // by checking the database
        // this is a slow and ineficient method
        // and should be changed latter
        memset(drawer->toolsPresentBitmap, 0, BitmapLength);


        // Create tool data model with current tool state
        DrawerList* list = MainController::getDataManagerInstance()->getDrawerTools(drwnum);

        for(int i = 0; i < list->tools.size(); ++i)
        {
            Tool *tool = list->tools[i].get();
            int toolnum = tool->toolNumber;
            int byteofst;
            int bitofst;
            fromToolNumber(toolnum, &byteofst, &bitofst);
//            int byteofst = toolnum / 8;
//            int bitofst = 7-((toolnum-1)%8);
            // tool present, update bitmap
            drawer->toolsPresentBitmap[byteofst] |= (0x01 << bitofst);
//            qDebug() << "CabinetManager::initDrawer toolnum/byte/bit " << toolnum << byteofst << bitofst;
        }
    }
    else
    {
        CabinetManager::setDrawerLightsOff(drwnum);   // jic
        return -1;
    }

    CabinetManager::setDrawerLightsOff(drwnum);
    return 0;
}


//
//  returns the drawer status structure
//

DrawerStatus* CabinetManager::getDrawerStatus(int drwnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
    {
        // error error
        return NULL;
    }
    return &drawerArray[drwnum - 1];

}


//
//  returns the drawer status structure
//

bool CabinetManager::drawerExist(int drwnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
    {
        return false;
    }
    return drawerArray[drwnum - 1].present;

}



//
//  Update the drawer status flags and bits
//      No events are emitted
//
void CabinetManager::updateDrawerStatus(int drwnum)
{
    uchar buf[100];

    if (checkToolDrawer(buf, sizeof(buf), drwnum) < 0)
            return;

    bool sts;
    DrawerStatus* drawer;
    int dnum;

    // check for drawer number is correct
    dnum = buf[1];
    if (dnum != drwnum)
    {
        // out of range
        return;
    }
    drawer = &drawerArray[drwnum-1];

    //
    // check drawer status bits
    //

    // is there a change in drawer open state
    drawer->statusBitmap = buf[2];
    if (drawer->statusBitmap & 0x01)
        sts = true;
    else
        sts = false;
    if (drawer->drawerOpen != sts)
    {
        // drawer status change has changed,
        drawer->drawerOpen = sts;
    }

    // is there a change in light state
    if (drawer->statusBitmap & 0x10)
        sts = true;
    else
        sts = false;
    if (drawer->lightOn != sts)
    {
        drawer->lightOn = sts;
    }

    // is there a change in slave state
    if (drawer->statusBitmap & 0x20)
        sts = true;
    else
        sts = false;
    if (drawer->slavePresent != sts)
    {
        drawer->slavePresent = sts;
    }


    // is there a change in slave state
    if (drawer->statusBitmap & 0x40)
        sts = true;
    else
        sts = false;
    if (drawer->slaveComms != sts)
    {
        drawer->slaveComms = sts;
    }

    // only process if light
    if (drawer->lightOn == true)
    {
        memcpy(drawer->currentLdrBitmap, buf+3, BitmapLength);
    }
}




//
//  returns true if the requested tool is in cabinet
//
bool CabinetManager::isToolAvailable(int toolnum, int drwnum)
{
    DrawerStatus* drawer = &drawerArray[drwnum-1];
    int byteofst;
    int bitofst;
    fromToolNumber(toolnum, &byteofst, &bitofst);
    qDebug() << "CabinetManager::isToolAvailable() toolnum/byte/bit " << toolnum << byteofst << bitofst;
//    int byteofst = toolnum / 8;
//    int bitofst = 7-((toolnum-1)%8);
    if (drawer->toolsPresentBitmap[byteofst] & (0x01 << bitofst) &&
        drawer->currentLdrBitmap[byteofst] & (0x01 << bitofst))
        return true;
    else
        return false;

}



//
// Unsolicited message received from the cabinet hardware
//  Check what has changed and emit the related signal.
//
void CabinetManager::drawerStatusChanged(QByteArray ba, int len)
{

    bool sts;
    DrawerStatus* drawer;
    int dnum;
    bool lightsOffToOn = false;
    uchar* buf = (uchar*)ba.data();

    // check correct message command
    if (buf[0] != CabinetCommands::DrawStatusChange)
    {
        qCritical() << "CabinetManager::drawerStatusChanged() Not status change message " << buf[0];
        return;
    }

    // check for drawer number
    dnum = buf[1];
    qDebug() << "CabinetManager::drawerStatusChanged data for drawer " << dnum;
    if (dnum < 1 || dnum > MaxDrawers)
    {
        // out of range
        qCritical() << "CabinetManager::drawerStatusChanged() Drawer number out of range " << buf[1];
        return;
    }
    drawer = &drawerArray[dnum-1];



    //
    // check drawer status bits
    //

    // is there a change in drawer open state
    drawer->statusBitmap = buf[2];
    if (drawer->statusBitmap & 0x01)
        sts = true;
    else
        sts = false;
    if (drawer->drawerOpen != sts)
    {
        // drawer status change has changed, update drawer status before emiting change
        drawer->drawerOpen = sts;
        qDebug() << "CabinetManager::drawerStatusChanged Drawer " << dnum << " has just been " << (drawer->drawerOpen ? "opened" : "closed");
        emit drawerOpenChanged(dnum, drawer);
    }


    // is there a change in light state
    if (drawer->statusBitmap & 0x10)
        sts = true;
    else
        sts = false;
    if (drawer->lightOn != sts)
    {
        drawer->lightOn = sts;
        emit drawerLightChanged(dnum, drawer);
        if (sts == true)
        {
            // lights just turned on
            lightsOffToOn = true;
            qDebug() << "CabinetManager::drawerStatusChanged() Lights just turned on, ignore tool bitmap changes";
        }
    }


    // is there a change in slave state
    if (drawer->statusBitmap & 0x20)
        sts = true;
    else
        sts = false;
    if (drawer->slavePresent != sts)
    {
        drawer->slavePresent = sts;
        emit drawerSlaveChanged(dnum, drawer);
    }


    // is there a change in slave state
    if (drawer->statusBitmap & 0x40)
        sts = true;
    else
        sts = false;
    if (drawer->slaveComms != sts)
    {
        drawer->slaveComms = sts;
        emit drawerSlaveCommsChanged(dnum, drawer);
    }

    // only process if light on and the drawer open
    if (drawer->lightOn == true && drawer->drawerOpen == true && !lightsOffToOn)
    {
        uchar bmp[BitmapLength];
        memcpy(bmp, buf+3, BitmapLength);

        QByteArray array((const char*)bmp, BitmapLength);
        qDebug() << "CabinetManager::drawerStatusChanged: " << QString(array.toHex()) << "New data";
        QByteArray array2((const char*)drawer->currentLdrBitmap, BitmapLength);
        qDebug() << "CabinetManager::drawerStatusChanged: " << QString(array2.toHex()) << "Current data";
        QByteArray array3((const char*)drawer->toolsPresentBitmap, BitmapLength);
        qDebug() << "CabinetManager::drawerStatusChanged: " << QString(array3.toHex()) << "Tools present";



        if (memcmp(bmp, drawer->currentLdrBitmap, BitmapLength) != 0)
        {
            qDebug() << "CabinetManager::drawerStatusChanged: bitmap HAS change!!!";
            uchar currbm[BitmapLength];
            memcpy(currbm, drawer->currentLdrBitmap, BitmapLength);
            memcpy(drawer->currentLdrBitmap, bmp, BitmapLength);
            // there is a change in the bitmap
            // is this a change in a present tool?
            for (int i=0; i < BitmapLength; i++)
            {
                //uchar newbm = bmp[i];
                //uchar currbm = drawer->currentLdrBitmap[i];
                //qDebug() << "CabinetManager::drawerStatusChanged: newbm/currbm byte " << newbm << currbm;
                uchar newbmbyt = (bmp[i] &  drawer->toolsPresentBitmap[i]);
                uchar currbmbyt = (currbm[i] & drawer->toolsPresentBitmap[i]);
                if (newbmbyt != currbmbyt)
                {
                    // yes there has been a change
                    // find which bit
                    uchar b = 0x01;
                    for (int j=0; j < 8; j++)
                    {
                        if ((newbmbyt ^ currbmbyt) & b)
                        {
                            int tn = getToolNumber(i, j);
                            bool out = currbmbyt & b;
                            qDebug() << "CabinetManager::drawerStatusChanged tool changed" << tn << out;
                            emit drawerToolChanged(dnum, tn, out);
                        }
                        //drawer->toolsPresentBitmap[j] &= 1 << i;
                        b = b << 1;
                    }
                }
            }
            // found changes now set current bitmap to new bitmap
            qDebug() << "CabinetManager::drawerStatusChanged copy new bitmap to current";
            //memcpy(drawer->currentLdrBitmap, bmp, BitmapLength);
        }
        else
        {
            qDebug() << "CabinetManager::drawerStatusChanged: no bitmap change!!!";
        }
    }
}




//
// Unsolicited confirm button message received from the cabinet hardware
//  Two messages are received, ignore the message with draw number zero.
//  just emit a signal.
//

void CabinetManager::confirmButtonActivated(QByteArray ba, int len)
{

    uchar* buf = (uchar*)ba.data();
    // check for drawer number
    qDebug() << "CabinetManager::confirmButtonActivated data " << buf[0] << buf[1];
    // check command byte jic
    if (buf[0] != CabinetCommands::ConfirmButton)
    {
        qCritical() << "CabinetManager::confirmButtonActivated() Not confirm button message " << buf[0];
        return;
    }
    if (buf[1] == 1)
    {
        emit confirmButtonPushed();
    }
    else
    {
        qDebug() << "CabinetManager::confirmButtonActivated() Ignore pushbutton release";
    }
}




//
//  returns the tool number from the bitmap
//  this is used to access the tool data in the database
//

int CabinetManager::getToolNumber(int byt, int bit)
{
    return byt*8 + 8-bit;
}

void CabinetManager::fromToolNumber(int toolnum, int* byt, int* bit)
{
    *byt = (toolnum-1) / 8;
    *bit = 7-((toolnum-1)%8);

}



//----------------------------------------------------------------------------
//
//  Cabinet access functions
//  These are the public access functions for the drawer hardware
//
//----------------------------------------------------------------------------

bool CabinetManager::cabinetConnected()
{
    return cabinetInstance->cabinetConnected();
}


QString CabinetManager::getFirmwareVersion()
{
    QString str;
    uchar buf[20];

    int res = cabinetInstance->getFirmwareVersion(buf, sizeof(buf));
    if (res > 0)
    {
        str = QString("%1.%2").arg(buf[1]).arg(buf[2]);
    }
    else
    {
        str = "";
    }
    return str;
}


int CabinetManager::checkToolDrawer(uchar* rtnbuf, int rtnbuflen, uchar drawer)
{
    int res =  cabinetInstance->checkToolDrawer(rtnbuf, rtnbuflen, drawer);
    return res;

}


int CabinetManager::lockDrawers()
{

    return cabinetInstance->lockDrawers();

}


int CabinetManager::unlockDrawers()
{

    return cabinetInstance->unlockDrawers();

}


int CabinetManager::checkDrawersLock(uchar *rtnbuf, int rtnbuflen)
{
    return cabinetInstance->checkDrawersLock(rtnbuf, rtnbuflen);

}


int CabinetManager::checkDrawersOpen(uchar* rtnbuf, int rtnbuflen)
{
    return cabinetInstance->checkDrawersOpen(rtnbuf, rtnbuflen);

}


int CabinetManager::setDrawerLightsOn(char drawer)
{
    return cabinetInstance->setDrawerLightsOn(drawer);

}


int CabinetManager::setDrawerLightsOff(char drawer)
{
    return cabinetInstance->setDrawerLightsOff(drawer);

}



bool CabinetManager::checkLightsOn(int dnum)
{
    uchar rtnbuf[100];
    int res =  checkToolDrawer(rtnbuf, sizeof(rtnbuf), dnum);

    if (rtnbuf[2] & 0x10)
        return true;
    else
        return false;

}

bool CabinetManager::turnLightsOnAndCheck(int dnum)
{
    setDrawerLightsOn(dnum);
    return checkLightsOn(dnum);
}







ToolsAvailable::ToolsAvailable(DrawerStatus* drw)
{
    drawer = drw;
}


int ToolsAvailable::first()
{
    byt = 0;
    bit = 1;
    bitcnt = 0;
    for (; byt < BitmapLength; byt++, bit = 1, bitcnt = 0)
    {
        if (drawer->toolsPresentBitmap[byt])
        {
            //find bit
            for (; bitcnt < 8; bitcnt++, bit = bit << 1)
            {
                if (drawer->toolsPresentBitmap[byt] & bit)
                {
                    // tool present
                    int tn1 =  MainController::getCabinetManagerInstance()->getToolNumber(byt, bitcnt);
                    int tn2 = byt*8 + 8-bitcnt;
//                    qDebug() << "ToolsAvailable::next()" << tn1 << tn2;
                    if (tn1 != tn2)
                        qDebug() << "ToolsAvailable::next() ERROR ERROR ERROR" << tn1 << tn2;
                    return byt*8 + 8-bitcnt;
                }
            }
        }
    }
    return -1;
}

int ToolsAvailable::next()
{
    bitcnt++;
    bit = bit << 1;
    if (bitcnt >= 8)
    {
        byt++;
        bit = 1;
        bitcnt = 0;
    }

    for (; byt < BitmapLength; byt++, bit = 1, bitcnt = 0)
    {
        if (drawer->toolsPresentBitmap[byt])
        {
            //find bit
            for (; bitcnt < 8; bitcnt++, bit = bit << 1)
            {
                if (drawer->toolsPresentBitmap[byt] & bit)
                {
                    // tool present
                    int tn1 =  MainController::getCabinetManagerInstance()->getToolNumber(byt, bitcnt);
                    int tn2 = byt*8 + 8-bitcnt;
//                    qDebug() << "ToolsAvailable::next()" << tn1 << tn2;
                    if (tn1 != tn2)
                        qDebug() << "ToolsAvailable::next() ERROR ERROR ERROR" << tn1 << tn2;
                    return byt*8 + 8-bitcnt;
                }
            }
        }
    }
    return -1;

}

int ToolsAvailable::state()
{
    return ((drawer->currentLdrBitmap[byt] & bit) > 0 ? 1 : 0);
}


#ifdef TRAK_TESTKEYS

void CabinetManager::openDrawerTest(int drwnum)
{
    cabinetInstance->openDrawer(drwnum);
}

void CabinetManager::closeDrawerTest(int drwnum)
{
    cabinetInstance->closeDrawer(drwnum);
}


void CabinetManager::removeToolTest(int drwnum, int toolnum, int state)
{
    cabinetInstance->toolTest(drwnum, toolnum, state);
}
#endif


