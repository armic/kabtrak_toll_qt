#ifndef BACKEND_H
#define BACKEND_H

#include <QSqlDatabase>
#include <QObject>
#include <QDebug>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QSettings>
//#include <QSqlTableModel>

#include "cabinet.h"
#include "toolstablemodel.h"
//#include "cabinetmanager.h"
#include "trakcore.h"


#define MaxDrawers 10


class DrawerList;


struct User
{

    QString userId;  // this is the tag number
    QString name;
    QString tagNumber;

    int tailId;
    QString tailDesc;
    //bool useWorkOrder;
    //bool useBatch;
    QString currentWorkOrder;
    QString currentBatch;
    int adminPriv;


};

struct Customer
{
    int custId;  // customer database id
    QString companyName;
    QString Address;
    QString Address1;
    QString Address2;
    QString ContactPerson;
    QString ContactNumber;
    QString Notes;
    QString RegistrationCode;
    QString RegistrationID;
    int status;
    QString email;
    QString contact;
    QString jobLabel;
    QString kabID;
    QString modelNumber;
    QString comlogo;
    bool useWorkOrder;
    bool workOrderNumeric;
    int workOrderMax;
    int workOrderMin;
    QString workOrderLabel;
    bool useBatch;
    bool batchNumeric;
    int batchMax;
    int batchMin;
    QString batchLabel;


};

struct CabinetDetails
{
    QString kabId;
    QString description;
    QString location;
    int custId;
    int numberDrawers;
    int numberBins;
    QString serialNumber;
    QString fw;
    int defaultCalibrationAccount;
    int defaultServiceAccount;
    int defaultTransferAccount;
    int defaultTestAccount;
    bool allowReturnByOthers;
    bool allowReturnByAdmin;
    bool allowSounds;
    int closeDrawerTimeOut;
    int openDrawerTimeout;
    int adminTimeOut;
    int issuedToolsTimeout;
    int calibration;
    int calibrationNotify;
    QString url;
    QString updateDir;
    bool calibrationVerify;
    bool calibrationUserModeUpdate;

};

class Tool;
enum class ToolStates;
struct ToolData;

class DataManager : public QObject
{
    Q_OBJECT
public:
    explicit DataManager(QObject *parent = 0);

    friend struct ToolData;

    bool postCabinetOwner();
    bool postCabinetDetails();

    bool databaseActive;
    bool databaseConnected();

    void initDataManager();

    void compareCabinetToDatabase();
    void compareCabinetDrawerToDatabase(int drwnum);

    QSqlQueryModel* toolsOutModel;
    QSqlQueryModel* createToolsOutModel(QObject *parent = 0);
    int getToolsOutCount();
    void refreshToolsOut();
    int toolsOutCountArray[MaxDrawers];
    void toolsOutForUser(QString user);
    void incToolsOutForUser(int drwnum, int sts);

    bool getUserDetails(QString token);

    bool isToolPresent(int drwnum, int num);
    bool isTool(int drwnum, int num);

    QSqlQueryModel *getTailNumbersModel(int custid);
    QSqlQueryModel *getTailNumbersAdminModel(int custid);
    //QSqlQueryModel* tailNumModel;

    void setToolStatus(int drwnum, int toolnum, int sts);


    DrawerList* getDrawerTools(int dnum);
    DrawerList* getTooldisplayList(int drwnum);
    void updateToolsDrawer(int dnum);
    Tool* getToolDetail(int drwnum, int toolnum);
    ToolStates getToolState(int drwnum, int toolnum, bool currentuser);

    QSqlQueryModel* loadTransactionsModelForDrawer(QObject* parent, int drwnum);


    // transaction functions
    void updateToolTransactionStatus(int drwnum, int toolnum, int newtype, QString itemid);
    void updateToolTransaction(int drwnum, int toolnum, int curtype, int newtype);
    void insertToolTransaction(int drwnum, int toolnum, int ttype, QString itemid,  QString remark = QString(""));
    int findToolTransaction(int drwnum, int toolnum, int ttype);

    QString findToolUser(int drwnum, int toolnum, int ttype);
    void updateToolTransactionWithId(int issuedid, int ttype, QString remark);
    void markToolStatusAsIn(int drwnum, int toolnum);
    void markToolStatusAsOut(int drwnum, int toolnum);
    void markToolAsMissing(int drwnum, int toolnum);
    void resetToolStatus(int drwnum, int toolnum, int status);


    User currentUser;
    Customer currentCustomer;
    CabinetDetails currentCabinet;
    QSqlQuery getSelectedEmp(QString token);
    QSqlQuery getSelectedUser(QString token);

    bool updateEmployeeRecord(QString puserId, QString pFirstName, QString pMiddlename, QString pLastName, QString pExtName, QString pPosition, QString pEmail,QString pMobileNumber, QString pUserName, QString pPassword, int pIsUser, int pUserRole,int pisUser_kab, int pDisabled);
    bool updateUserRecord(QString puserId, QString pFirstName, QString pMiddlename, QString pLastName,  int pEnabledKab, int pAccessTypeKab);
    bool insertEmployeeRecord(QString puserId, QString pFirstName, QString pMiddlename, QString pLastName, QString pExtName, QString pPosition, QString pEmail,QString pMobileNumber, QString pUserName, QString pPassword, int pIsUser, int pUserRole,int pisUser_kab, int pDisabled);

    // Cabinet Module
    QSqlQuery getCabinetDetails();

    //Tail
     bool isTailExists(QString token);
     QSqlQuery getSelectedTail(QString token);
     bool insertTailRecord(QString pDescription, QString pRemark, int pSpeedDial, int pDisabled, int pCustID);
     bool updateTailRecord(QString pDescription, QString pRemark, int pSpeedDial, int pDisabled, int pCustID);

     // Transactions

     bool clearTransactions();
     QSqlQuery loadIsssueanceData(const QString sql_statement);
     QSqlQueryModel* loadIssueanceModel(const QString sql_statement);
     QSqlQueryModel* loadAllTransactionModel();

     //Tool
     QSqlQuery getSelectedTool(QString token);

public:
    QSqlQueryModel* loadUserModel(QObject *parent = 0);
    QString getUserName(QString userid);
    int getUserAccessType(QString userid);
    QString getUserNameFromModel(QString userid);
    int getUserAccessTypeFromModel(QString userid);
    void updateUserAccessCount(QString userid);
private:
    QSqlQueryModel* userModel;


signals:

public slots:

private:
    bool connectToDatabase();  // do not want anyone else calling it
    QSqlDatabase db;
    bool loadCabinetOwner();
    bool loadCabinetDetails();

public:
    void buildToolsDrawerArray();
private:
    DrawerList* toolDrawerArray[MaxDrawers];
    DrawerList* buildToolsDrawer(int dnum);
    void setToolTableModelStatusOld(int drwnum, int toolnum, int sts);
    void resetToolTableModelStatus(int drwnum, int toolnum, int sts);
    void setTooldisplayListStatus(int drwnum, int toolnum, int sts);
    void updateToolStatusInDB(int id, int psts, int sts);




    // Database maintenance functions for admin
private:
    QSqlQueryModel* checkTablesModel = NULL;
    QSqlQueryModel* analyzeTablesModel = NULL;
    QSqlQueryModel* optimizeTablesModel = NULL;
    QSqlQueryModel* repairTablesModel = NULL;

public:
    void checkTablesResult();
    void analyzeTablesResult();
    void optimizeTablesResult();
    void repairTablesResult();
    QSqlQueryModel* checkTables();
    QSqlQueryModel* analyzeTables();
    QSqlQueryModel* optimizeTables();
    QSqlQueryModel* repairTables();


private:
    QSqlQueryModel* employeeListModel = NULL;
    QSqlQueryModel* userListModel = NULL;
public:
    QSqlQueryModel* getEmployeeList();
    QSqlQueryModel* getUsersList();
    QSqlQueryModel* getLocationList();
    QSqlQueryModel* getDrawerList();
    QSqlQuery getLocationListQry();
    QSqlQuery getTailListQry();
    QSqlQueryModel* getToolList(int DrawNum);
    QSqlQueryModel* getToolInventoryList();
    QSqlQueryModel* getCabinetToolList();
    QSqlQuery getToolCategoryList();
    QSqlQuery getCustomerData();
    QSqlQuery getCabinetData();
    QSqlQuery getCabToolQry(const int DrawNum, int ToolNum);

    bool toolExists(QString token);


    bool isUserExists(QString token);
    bool isEmployeeExists(QString token);
    bool grantUserAccess(QString UserID, QString Fullname);
    bool revokeUserAccess(QString UserID);
    bool updateEmployeeGrant(QString UserID, int status);
    bool updateCabinetRecord(QString kabId, int CustID, QString description, QString modelnumber, QString firmware, QString notes, QString serialnumber, int location );

    bool updateToolRecord(QString stockCode,  QString description, int isKit , int kitCount, int catid);
    bool insertToolRecord(QString stockCode,  QString description, int isKit , int kitCount, int catid);
    bool isCabToolExists(QString token);
    bool deleteToolRecord(const QString stockCode);
    bool postToPortal(const QString sSQL);
    bool setCabTool(const int drawNum, int toolNum, int serviceable, int calibration, int calibrated, int transferred,  QDate dateCalibration, QDate datelastCalibration );


public:
    // enums and constants
    enum{Issued = 0, Returned = 1 };
    enum{Unserviceable = 0, Serviceable = 1};


    // new type of tool lists
private:
    QList<ToolData*>* toolDataDrawerArray[Trak::MaxDrawerNumber];
public:
    void buildToolsDataArray();
    QList<ToolData*>* buildToolsDataDrawer(int drwnum);
    ToolData* getToolData(int drwnum, int toolnum);
    void updateToolsChanged(int drwnum);
    void updateToolsChangedAllDrawers();
    void updateTool(ToolData* tool);
    bool isToolMissing(int drwnum, int toolnum);
    void setToolMissingStatus(int drwnum, int toolnum, int sts);
    void updateToolStatus(int drwnum, int toolnum, int sts);
    void updateToolMissing(int drwnum, int toolnum, int sts);
    int getToolStatus(int drwnum, int toolnum);
    void updateToolCalibrationDate(ToolData* tool);
    void updateToolCalibrationInfo(ToolData* tool);
    void updateToolServiceable(ToolData* tool);
    void updateToolFromToolData(int drwnum, int toolnum);



};




//-------------------------------------------------------------
//
//  Support classes for tools and drawer data
//
//-------------------------------------------------------------

#include <memory>
#include <vector>

#include <QGraphicsScene>
#include <QtSvg>



class ToolImageTransform
{
public:
    ToolImageTransform();
    ToolImageTransform(float x, float y);

    float x, y, rotation, scale, width, height;
    bool hFlip, vFlip;
};

enum class ToolStates {TOOL_IN, TOOL_OUT, TOOL_MISSING, TOOL_OTHER_USER, TOOL_UNSERVICEABLE, TOOL_TRANSFERRED};

class Tool
{
public:
    //enum class States {TOOL_IN, TOOL_OUT, TOOL_MISSING, TOOL_OTHER_USER, TOOL_UNSERVICEABLE, TOOL_TRANSFERRED};

    Tool();
    Tool(QString name, ToolStates state, QString imageID, ToolImageTransform transform, int toolnum, QString stockcode, int drwnum);

    int toolNumber;
    ToolStates state;
    QString name;
    QString imageID;
    ToolImageTransform transform;
    QString stockCode;
    int drawerNum;

    QGraphicsScene* scene;
    QGraphicsSvgItem* svgItem;
};


class DrawerList
{
public:
    DrawerList();

    void addTool(Tool* tool);

    std::vector<std::unique_ptr<Tool>> tools;

private:
};




struct ToolData
{
    bool changed;
    int id;
    QString binId;
    QString drawerId;
    QString kabId;
    int custId;
    QString itemId;
    int prevstatus;
    int status;
    int toolNumber;
    int drawerNum;
    int missing;
    float cleft;
    float ctop;
    int visible;
    int blockIndex;
    float angle;
    float width;
    float height;
    bool changedCalibration;
    QDate currentCalibrationDate;
    QDate lastCalibrationDate;
    int calibration;
    int serviceable;
    QString serialNumber;
    int calibrated;
    int transferred;
    int flip;
    int reverse;

    QString description;
    QString stockcode;
    int kitcount;
    int isKit;
    QString image_b;

    ToolData(int drwnum, int toolnum);
    ToolData();



};

#endif // BACKEND_H
