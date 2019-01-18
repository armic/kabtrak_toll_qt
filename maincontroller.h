#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QMessageLogContext>
#include <QMainWindow>
#include <QtDebug>
#include <QSoundEffect>

#include <QKeyEvent>

#include "datamanager.h"
#include "mainview.h"
#include "cabinetmanager.h"



// data structure for illegal return list
struct ToolUser {
    int drawerNum;
    int toolNum;
    int action;
    QString userId;
};




class MainController : public QObject
{
    Q_OBJECT

public:
    MainController(QWidget *parent = 0);
    ~MainController();

    bool allsWell();

    static MainController* getMainControllerInstance();
    static MainView* getMainViewInstance();
    static DataManager* getDataManagerInstance();
    static CabinetManager* getCabinetManagerInstance();
    static void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void messageOutputBeforeInit(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void setDrawerClosed(int dnum, struct DrawerStatus* drawer);
    void setDrawerOpened(int dnum, struct DrawerStatus* drawer);

    void logToolState(int drwnum, int toolnum);
    quint64 readMemoryUsage();



protected:
    void keyPressEvent(QKeyEvent* event);



private:
    static MainController* mainControllerInstance;
    static MainView* mainViewInstance;
    static DataManager* dataManagerInstance;
    static CabinetManager* cabinetManagerInstance;
    //void setDrawerOpened(int dnum, struct DrawerStatus* drawer);
    //void setDrawerClosed(int dnum, struct DrawerStatus* drawer);

    int currentOpenedDrawer;
    int drawersOpenCount;


private slots:
    void onDrawerOpenChanged(int dnum, struct DrawerStatus* drawer);
    void onDrawerLightChanged(int dnum, struct DrawerStatus* drawer);
    void onDrawerToolChanged(int dnum, int toolnum, bool out);
    void changeToolServiceableStatus(ToolData* tool);

private:
    bool returningTool(int dnum, int toolnum);
    bool issuingTool(int dnum, int toolnum);

public slots:
    void onConfirmButton();




// application modes
public:
    enum {initMode, guestMode, userMode, drawerClosedMode, drawerOpenMode, adminMode, toolsAdminMode};

    int appMode;
    int drawerMode;
    void setAppMode(int mode);
    int getAppMode();
    void setDrawerMode(int mode);
    int getDrawerMode();
    QString displayMode(int mode);


// List of tools returned by wrong users.
private:
    QList <ToolUser> illegalReturn;
    void addToIllegalReturnList(int drwnum, int toolnum, QString user, int action);
    void removeFromIllegalReturnList(int drwnum, int toolnum);
    bool isToolOnIllegalList(int drwnum, int toolnum);
    int illegalReturnListCount();
    QLabel illegalReturnMsg;
    void displayIllegalReturnMessage(int dnum, int toolnum, QString tooluser, int action);
    void removeIllegalReturnMessage();
    void displayNextIllegalMessage();
    void exitApp(int reason);

public:
    QSplashScreen* splash;
    void requestExit(int reason);


    // sounds
private:
    QSoundEffect* toolReturnAlarm;
    void illegalReturnAlarmOn();
    void illegalReturnAlarmOff();
    QSoundEffect* toolIssuePing;
    QSoundEffect* toolReturnPing;
    QSoundEffect* toolReturnBleep;
    bool allowSounds;

};



//
//  The main event filter to trap and interrogate keystrokes
//

//class mainEventFilter: public QObject
//{
//public:
//    mainEventFilter();
//    ~mainEventFilter();

//    bool eventFilter(QObject* object,QEvent* event);
//};










#endif // MAINCONTROLLER_H
