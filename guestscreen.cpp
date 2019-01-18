
#include <QSqlQueryModel>

#include <windows.h>
#include <stdio.h>
#include <psapi.h>

#include "trakcore.h"
#include "guestscreen.h"
#include "ui_guestscreen.h"
#include "datamanager.h"
#include "maincontroller.h"
#include "ui_selecttail.h"
#include <iostream>
#include <fstream>
#include "selecttail.h"
#include "cabinetmanager.h"
#include "tooloutform.h"
#include "accessdenied.h"
#include "version.h"

using namespace std;


#ifdef TRAK_TESTKEYS
#include <QtTest/QtTest>
#endif


GuestScreen::GuestScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GuestScreen)
{

    ui->setupUi(this);

    // setup to displat time and date
    timer =  new QTimer(this);
    connect(timer, SIGNAL(timeout()),this,SLOT(dateTime()));
    timer->start(1000);

    setupScreen();

    setScreen();
    show();
}

GuestScreen::~GuestScreen()
{
    delete ui;
}




//
//  display the time and date on the guest screen
//
void GuestScreen::dateTime()
{
  QTime time = QTime::currentTime();
  QDate date = QDate::currentDate();
  QString time_text = time.toString("h:mm:ss AP");
  QString date_text = date.toString("M/d/yyyy");
  ui->lblTime->setText("Time: " + time_text);
  ui->lblDate->setText("Date: " + date_text);
}





//
//  set initial colour and text of button
//

void GuestScreen::setupScreen()
{
    // setup text fields
    DataManager* dataman = MainController::getDataManagerInstance();
    QString CompanyLogo = "images/%1"; // Customer logo

//    ui->switchButton->setVisible(false);
//    ui->about->setVisible(false);
//    ui->vesionNumber->setVisible(false);

    ifstream file(CompanyLogo.arg(dataman->currentCustomer.comlogo).toStdString());
    if(!file)
        ui->CustomerLogo->setPixmap(QPixmap("images/default_logo.png"));
    else

    ui->CustomerLogo->setPixmap(QPixmap(CompanyLogo.arg(dataman->currentCustomer.comlogo)));

    ui->kabDescription->setText(dataman->currentCabinet.description);
    ui->kabSerial->setText(dataman->currentCabinet.serialNumber);

    ui->FirmwareVersion->setText("Firmware " + dataman->currentCabinet.fw);
    ui->vesionNumber->setText(Version::getVersionString());

    selecttailDialog = new SelectTail(ui->tailWidget);
    ui->tailWidget->setVisible(false);

    ui->toolsStatusButton->setStyleSheet("background-color:rgb(254,0,0);");
    ui->toolsStatusButton->setVisible(false);
    connect(ui->toolsStatusButton, &QPushButton::clicked, this, &GuestScreen::toolButtonClicked);

    // sets device status slots
    connect(MainController::getCabinetManagerInstance(), &CabinetManager::connected, this, &GuestScreen::deviceConnected);
    connect(MainController::getCabinetManagerInstance(), &CabinetManager::disconnected, this, &GuestScreen::deviceDisconnected);

    // set power status info
    powerStatus = new PowerStatus(this);
    connect(powerStatus, &PowerStatus::powerConnectionChanged, this, &GuestScreen::powerStatusChanged);

}

void GuestScreen::setScreen()
{
    DataManager* backend = MainController::getDataManagerInstance();
    backend->refreshToolsOut();
    int cnt = backend->getToolsOutCount();
    if (cnt >0)
    {
        ui->pnlBand->setVisible(false);
        ui->toolsStatusButton->setStyleSheet("background-color:rgb(230,0,0);"
                                             "font-size: 40pt;"
                                             "font-weight: bold;");
        ui->toolsStatusButton->setVisible(true);
        QString str = QString("TOOLS OUT ( %1 ) \n  [ Touch to view ]").arg(QString::number(cnt));
        //ui->toolsStatusButton->setText(tr("TOOLS OUT \n TOUCH TO VIEW"));
        ui->toolsStatusButton->setText(str);
    }
    else
    {
        ui->pnlBand->setVisible(true);
        ui->toolsStatusButton->setVisible(false);
    }
    ui->tailWidget->setVisible(false);   // jic

    powerStatusChanged(powerStatus->powerConnected());
}



//
//  updates the device status display
//
void GuestScreen::deviceConnected()
{
    qDebug() << "GuestScreen::deviceConnected()";
    ui->deviceStatusStrLabel->setText("Hardware connected");
    ui->deviceStatusLabel->setPixmap(QPixmap(":svg/resources/led_on.svg"));

}

void GuestScreen::deviceDisconnected()
{
    qDebug() << "GuestScreen::deviceDisconnected()";
    ui->deviceStatusStrLabel->setText("Hardware disconnected");
    ui->deviceStatusLabel->setPixmap(QPixmap(":svg/resources/led_off.svg"));

}




void GuestScreen::on_switchButton_clicked()
{
    //MainController::getMainViewInstance()->processSwitchClicked();  // just for testing
}


//
//  Displays the tools out table
//
void GuestScreen::toolButtonClicked()
{
    // load tool out form ui
    toolOutForm frmToolOut;
    frmToolOut.setWindowFlags(Qt::FramelessWindowHint);
    frmToolOut.setModal(true);
    frmToolOut.exec();

//    QSqlQueryModel* model = MainController::getDataManagerInstance()->toolsOutModel;

//    QTableView *view = new QTableView(this);

//    view->setWindowTitle("TOOLS OUT");

//    model->setHeaderData(0, Qt::Horizontal, tr("Date"));
//    model->setHeaderData(1, Qt::Horizontal, tr("User Id"));
//    model->setHeaderData(2, Qt::Horizontal, tr("Issued to"));
//    model->setHeaderData(3, Qt::Horizontal, tr("Tool name"));
//    model->setHeaderData(4, Qt::Horizontal, tr("Drawer"));
//    model->setHeaderData(5, Qt::Horizontal, tr("Tool number"));
//    model->setHeaderData(6, Qt::Horizontal, tr("Tail number"));

//    // sorting columns
//    QSortFilterProxyModel *sort_filter = new QSortFilterProxyModel(this);
//    sort_filter->setSourceModel(model);
//    sort_filter->sort (2);
//    view->setModel (sort_filter);

//    //view->setModel(model);
//    view->resizeRowsToContents();
//    view->setAlternatingRowColors(true);

//    view->hideColumn(1);
//    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
//    view->setSortingEnabled(true);

//    // calculate total width of columns
//    int cnt = model->columnCount();
//    int totalwidth = 0;
//    for (int i = 0; i < cnt; i++)
//    {
//        totalwidth += view->columnWidth(i);
//    }
//    totalwidth += view->verticalHeader()->width();
//    totalwidth += 2;
//    view->resize(totalwidth,400);

//    // set timeout for this window
//    // Get timer values from ini file
//    QSettings settings("trak.ini", QSettings::IniFormat);
//    settings.beginGroup("System");
//    int tmr = settings.value("IssuedToolsTimeout", 10).toInt();
//    settings.endGroup();
//    QTimer *timer = new QTimer(this);
//    connect(timer, &QTimer::timeout, view, &QTableView::deleteLater);
//    timer->start(tmr * 1000);

//    view->show();

}







//
//  the standby state keyPressEvent handler
//      Currently used to wait for the token reader string id
//      which is !<idNumber># as keystrokes.
//      Key presses are not passes on but consumed ie return true
//

bool GuestScreen::event(QEvent* event)
{
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        qDebug() << "GuestScreen::event KEY is " << ke->key();

#ifdef TRAK_TESTKEYS
        if (ke->key() == Qt::Key_F1)
        {
            QTest::keyClicks(this, "!52005370C5#");
            QTest::keyClick(this, Qt::Key_Enter);
            return true;
        }
        if (ke->key() == Qt::Key_F2)
        {
            QTest::keyClicks(this, "!00140007#");
            QTest::keyClick(this, Qt::Key_Enter);
            return true;
        }
#endif

        // waiting for token data
        checkForTokenData(event);
        return true;
    }
    else
    {
        return QWidget::event(event);
    }
}




void GuestScreen::checkForTokenData(QEvent* event)
{

    QChar key = static_cast<QKeyEvent *>(event)->key();
    qDebug() << "GuestScreen::checkForTokenData:: key entered" << key;
    if (key <= 0x00 || key > Qt::Key_AsciiTilde)
    {
        return;  // key not in range
    }
    if (key == Qt::Key_Exclam)
    {
        enteringToken = true;
        waitingForReturn = false;
        tokenString.clear();
    }
    else if(enteringToken)
    {
        if (waitingForReturn)
        {
            if (key == Qt::Key_Enter || key == Qt::Key_Return)
            {
                qDebug() << "GuestScreen::checkForTokenData:: token entered" << tokenString;
                processToken();

            }
            else
            {
                //return key shuold be next
                enteringToken = false;
                qDebug() << "GuestScreen::checkForTokenData:: return key was NOT entered";
            }
        }
        if (key == Qt::Key_NumberSign)
        {
            // now have a token now wait for return key
            waitingForReturn = true;
            qDebug() << "GuestScreen::checkForTokenData:: token entered" << tokenString << "now waiting for return key";
        }
        else
        {
            // append char to token string
            if (key != ' ')
            {
                tokenString.append(key);
                if (tokenString.length() > MAXTOKENLENGTH)
                {
                    enteringToken = false;
                    tokenString.clear();
                }
            }
        }
    }
}


//
//  Checks the entered token belongs to a valid user
//
void GuestScreen::processToken()
{

    quint64 memusage = readMemoryUsage();
    qDebug() << "GuestScreen::processToken() PagefileUsage" << memusage;

    // remove spaces from string
    // does this user exist?
    bool result = MainController::getDataManagerInstance()->getUserDetails(tokenString);
    // Valid user?
    if (result)
    {
        qDebug() << "valid user for token";
        addSelectTailWidget();
        MainController::getDataManagerInstance()->updateUserAccessCount(tokenString);
    }
    else {
        // invalid user, display error
        qDebug() << "Invalid user for token";
        invalidUserMessage();
        enteringToken = false;
        tokenString.clear();
    }
}


void GuestScreen::invalidUserMessage()
{
    // Load Employee List window
    accessdenied frmDeniedMessage;
    frmDeniedMessage.setWindowFlags(Qt::FramelessWindowHint);
    frmDeniedMessage.setModal(true);
    frmDeniedMessage.exec();
  //  MyMessageBox msgBox;
    //msgBox.setText("Invalid token or user");

//    Qt::WindowFlags wFlags = msgBox.windowFlags();
//    if(Qt::WindowCloseButtonHint == (wFlags & Qt::WindowCloseButtonHint))
//    {
//        wFlags = wFlags ^ Qt::WindowCloseButtonHint;
//    }
//    wFlags = wFlags | Qt::FramelessWindowHint;
//    msgBox.setWindowFlags(wFlags);
//    msgBox.setStandardButtons(0);
//    msgBox.setIconPixmap(QPixmap(":/images/images/accessdenied.png"));
//    msgBox.setTimeoutValue(5);
//    msgBox.exec();
}


//
//  Displays the select tail widget
//
void GuestScreen::addSelectTailWidget()
{

    selecttailDialog->setWidget();
    ui->tailWidget->setVisible(true);
    ui->tailWidget->setFocus();

//    // setup combo box
//    DataManager* backend = MainController::getDataManagerInstance();

//    QSqlQueryModel *model = backend->getTailNumbersModel(backend->currentCustomer.custId);
//    selecttailDialog->tailComboBox->setModel(model);

        qDebug() << "GuestScreen::addSelectTailWidget";

}



//
//  Read the current process memory usage
//
quint64 GuestScreen::readMemoryUsage()
{
    HANDLE processHandle;
    PROCESS_MEMORY_COUNTERS pmc;

    if ((processHandle = GetCurrentProcess()) != NULL)
    {
        if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc)))
        {

            //qDebug() << "GuestScreen::readMemoryUsage() PagefileUsage" << pmc.PagefileUsage << sizeof(pmc.PagefileUsage);
            return pmc.PagefileUsage;
        }
    }
    return 0;
}



void GuestScreen::powerStatusChanged(bool mainsPower)
{
    qWarning() << "GuestScreen::powerStatusChanged() " << mainsPower;
    if (mainsPower)
    {
        ui->powerSts->setText("AC Power");
        ui->powerSts->setStyleSheet("color: green");
    }
    else
    {
        ui->powerSts->setText("Battery Power");
        ui->powerSts->setStyleSheet("color: red");
    }

}


