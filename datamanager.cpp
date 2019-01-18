
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QTableView>
#include <QString>
#include <QSqlRecord>
#include <QHeaderView>


#include "datamanager.h"
#include "maincontroller.h"


//
//  DataManager constructor
//      Connect to the database
//

DataManager::DataManager(QObject *parent) : QObject(parent)
{
    // connect to database
    databaseActive = connectToDatabase();

    if (databaseActive)
    {
        // check the integrity of the db
        loadCabinetOwner();
        loadCabinetDetails();
    }
    else {
        // if no database what do we do???
        // Terminate kabtrak and inform end user.
        //********* This is not working very well ***************
        qCritical() << "DataManager::DataManager() Could not connect to database";
        MyMessageBox msgBox;
        msgBox.setText("Could not connect to database. Closing kabtrak");
        msgBox.setTimeoutValue(5);
        msgBox.exec();
        qCritical() << "DataManager::DataManager() Bye Bye";
        MainController::getMainControllerInstance()->requestExit(Trak::NoDatabaseConnection);

    }
}




//
//  Initialises the mysql database
//  the connection parameters should come from a config file.
//

bool DataManager::connectToDatabase()
{

    // get connection details from ini file
    QSettings settings("trak.ini", QSettings::IniFormat);
    settings.beginGroup("Database");

        QString database = settings.value("Database").toString();
        QString schema = settings.value("Schema").toString();
        QString port = settings.value("Port").toString();
        QString user = settings.value("Username").toString();
        QString password = settings.value("Password").toString();
        QString server = settings.value("Server").toString();

    settings.endGroup();

    qInfo() << " database connection: " << database << schema << port << user << password;

    // connect to database
    qDebug() << " Adding database";
    db = QSqlDatabase::addDatabase(database);
    db.setHostName(server);
    db.setDatabaseName(schema);
    db.setUserName(user);
    db.setPassword(password);
    db.setConnectOptions("MYSQL_OPT_RECONNECT=TRUE");
    qDebug() << "Connecting to database";
    bool result = db.open();
    if (!result)
        qWarning() << "Could not open database" << db.lastError().databaseText();

    databaseActive = result;
    return result;
}


bool DataManager::databaseConnected()
{
    return databaseActive;
}


void DataManager::initDataManager()
{
    // create tools table model
    //loadToolsTableModel(this);
    //buildToolsTableArray();

    buildToolsDrawerArray();


    toolsOutModel = createToolsOutModel(this);
    qDebug() << "DataManager::initDataManager() tools out " << getToolsOutCount();

    loadUserModel(this);

}




//
//  get user details from user table.
//      Populates the currentUser structure and returns true
//      else returns false if user not found.
//          NOTE. the sql part could be replaced by using the userModel functions.
//
bool DataManager::getUserDetails(QString token)
{

    // build query string
    QString str = QString("select fullName, accessTypeKab from users where userId = '%1' and kabId = '%2' and custId = %3 and enabledKab = 1 and kabtrak = 1");
    str = str.arg(token).arg(currentCabinet.kabId).arg(currentCustomer.custId);
    qDebug() << "DataManager::getUserDetails sql: " << str;

    QSqlQuery query;
    query.prepare(str);

    qDebug() << "DataManager::getUserDetails " << token << currentCustomer.custId << currentCabinet.kabId;
    qDebug() << str;
    query.exec();
    qDebug() << "DataManager::getUserDetails: query size" << query.size();
    qDebug() << query.executedQuery();
    if (query.size() == 1)
    {
        // one and only one token/user exists
        query.next();
        currentUser.userId = token;
        currentUser.name = getUserName(token);
        currentUser.tagNumber = token;
        currentUser.tailId = -1;
        currentUser.tailDesc.clear();
        currentUser.currentWorkOrder.clear();
        currentUser.currentBatch.clear();
        currentUser.adminPriv = query.value(1).toInt();
        return true;
    }
    return false;

}





//
//  Loads the cabinet owner details from the config file and the database
//      The important field is the customer id (cusId)
//
bool DataManager::loadCabinetOwner()
{

    QString logo;
    // get the id from the ini file
    QSettings settings("trak.ini", QSettings::IniFormat);
    if (settings.status() != QSettings::NoError)
        qWarning() << "DataManager::loadCabinetOwner error in reading settings file";

    settings.beginGroup("Customer");
        int custId = settings.value("ID").toInt();
        currentCustomer.custId = custId;
        qDebug() << "DataManager::loadCabinetOwner custId is " << custId;

        // get other details from customer ini file
        currentCustomer.email = settings.value("Email").toString();
        currentCustomer.contact = settings.value("Contact").toString();
        currentCustomer.jobLabel = settings.value("JobLabel").toString();
        currentCustomer.kabID = settings.value("kabID").toString();
        currentCustomer.modelNumber = settings.value("ModelNumber").toString();
        currentCustomer.comlogo = logo.setNum(custId) + ".png";
        // work order
        if (settings.value("UseWorkOrder").toInt() == 1)
            currentCustomer.useWorkOrder = true;
        else
            currentCustomer.useWorkOrder = false;
        if (settings.value("workOrderNumeric", "0").toInt() == 1)
            currentCustomer.workOrderNumeric = true;
        else
            currentCustomer.workOrderNumeric = false;
        currentCustomer.workOrderMax = settings.value("workOrderMax", "0").toInt();
        currentCustomer.workOrderMin = settings.value("workOrderMin", "0").toInt();
        currentCustomer.workOrderLabel = settings.value("workOrderLabel", "Work Order No.").toString();
        // batch number
        if (settings.value("UseBatch").toInt() == 1)
            currentCustomer.useBatch = true;
        else
            currentCustomer.useBatch = false;
        if (settings.value("batchNumeric", "0").toInt() == 1)
            currentCustomer.batchNumeric = true;
        else
            currentCustomer.batchNumeric = false;
        currentCustomer.batchMax = settings.value("batchMax", "0").toInt();
        currentCustomer.batchMin = settings.value("batchMin", "0").toInt();
        currentCustomer.batchLabel = settings.value("batchLabel", "Batch No.").toString();

    settings.endGroup();


    // now get the rest of the data from customer table in database
    // build query string
    QString str = QString("select companyName, address, address1, address2,contactPerson, telNo, email,notes,jobOrderLabel,registrationCode, registrationId, status from customer where id = %1 ");
    str = str.arg(custId);

    QSqlQuery query;
    query.prepare(str);
    query.exec();
    qDebug() << query.executedQuery();
    if (query.lastError().isValid())
        qDebug() << "Database ERROR: " << query.lastError();
    qDebug() << "DataManager::loadCabinetOwner: query size" << query.size();
    if (query.size() == 1)
    {
        query.first();
        //currentCustomer.custId = custId;
        currentCustomer.companyName = query.value(0).toString();
        currentCustomer.Address = query.value(1).toString();
        currentCustomer.Address1 = query.value(2).toString();
        currentCustomer.Address2 = query.value(3).toString();
        currentCustomer.ContactPerson = query.value(4).toString();
        currentCustomer.ContactNumber = query.value(5).toString();
        currentCustomer.email = query.value(6).toString();
        currentCustomer.Notes = query.value(7).toString();
        currentCustomer.jobLabel = query.value(8).toString();
        currentCustomer.RegistrationCode = query.value(9).toString();
        currentCustomer.RegistrationID = query.value(10).toString();
        currentCustomer.status = query.value(11).toInt();
        qDebug() << "DataManager::loadCabinetOwner: details" << currentCustomer.custId << currentCustomer.companyName;
        return true;
    }
    qDebug() << "DataManager::loadCabinetOwner: Error should be one record found" << query.size();
    return false;
}




bool DataManager::postCabinetOwner()
{
    // Update data
    // build query string
    QString str = QString("UPDATE customer SET companyName = \'%1\', address = \'%2\', address1 = \'%3\', address2 = \'%4\',"
                          "telNo = \'%5\', email= \'%6\', contactPerson = \'%7\', notes = \'%8\', jobOrderLabel = \'%9\', "
                          "registrationCode = \'%10\', registrationId = \'%11\' WHERE id = %12 ");
    str = str.arg(currentCustomer.companyName).arg(currentCustomer.Address).arg(currentCustomer.Address1)
            .arg(currentCustomer.Address2).arg(currentCustomer.ContactNumber).arg(currentCustomer.email)
            .arg(currentCustomer.ContactPerson).arg(currentCustomer.Notes).arg(currentCustomer.jobLabel)
            .arg(currentCustomer.RegistrationCode).arg(currentCustomer.RegistrationID).arg(currentCustomer.custId);

    QSqlQuery query;
    query.prepare(str);
    query.exec();
    if (query.lastError().isValid()) {
        qDebug() << "Database ERROR: " << query.lastError();
        return false;
    }
    else {
        postToPortal(str);
        qDebug() << "DataManager::postCabinetOwner posted successfully";
        return true;
    }
}






//
//  Loads the cabinet details from the config file and the database
//      The important field is the cabinet id (kabId)
//

bool DataManager::loadCabinetDetails()
{
    QSettings settings("trak.ini", QSettings::IniFormat);
    settings.beginGroup("Customer");
    int custid = settings.value("ID").toInt();
    settings.endGroup();

    // build query string
    QString str = QString("select kabId, description, location, custId, numberDrawers, numberBins, serialNumber from itemkabs where custId = %1 ");
    str = str.arg(custid);

    // get the data from itemkab table in database
    QSqlQuery query;
    query.prepare(str);
    query.exec();
    if (query.lastError().isValid())
        qDebug() << "Database ERROR: " << query.lastError();
    qDebug() << "DataManager::loadCabinetDetails: query size" << query.size();
    if (query.size() == 1)
    {
        query.first();
        currentCabinet.kabId = query.value(0).toString();
        currentCabinet.description = query.value(1).toString();
        currentCabinet.location = query.value(2).toString();
        currentCabinet.custId = query.value(3).toInt();
        currentCabinet.numberDrawers = query.value(4).toInt();
        currentCabinet.numberBins = query.value(5).toInt();
        currentCabinet.serialNumber = query.value(6).toString();
        qDebug() << "DataManager::loadCabinetDetails: details" << currentCabinet.kabId << currentCabinet.description;
    }
    else
    {
        qDebug() << "DataManager::loadCabinetDetails: Error should be one record found" << query.size();
        return false;
    }
    // read firmware version
    QString fw = MainController::getCabinetManagerInstance()->getFirmwareVersion();
    currentCabinet.fw = fw;

    settings.beginGroup("Cabinet");
    {
        currentCabinet.defaultCalibrationAccount = settings.value("DefaultCalibrationAccount").toInt();
        currentCabinet.defaultServiceAccount = settings.value("DefaultServiceAccount").toInt();
        currentCabinet.defaultTransferAccount = settings.value("DefaultTransferAccount").toInt();
        currentCabinet.defaultTestAccount = settings.value("DefaultTestAccount").toInt();

        int vall = settings.value("allowSounds", 0).toInt();
        currentCabinet.allowSounds = vall == 1 ? true : false;
        currentCabinet.calibration = settings.value("Calibration").toInt();
        currentCabinet.calibrationNotify = settings.value("CalibrationNotification").toInt();
        vall = settings.value("CalibrationVerify", 1).toInt();
        currentCabinet.calibrationVerify = vall == 1 ? true : false;
        vall = settings.value("CalibrationUserModeUpdate", 0).toInt();
        currentCabinet.calibrationUserModeUpdate = vall == 1 ? true : false;

        int val = settings.value("AllowReturnByOthers", 0).toInt();
        currentCabinet.allowReturnByOthers = val == 1 ? true : false;
        val = settings.value("AllowReturnByAdmin", 0).toInt();
        currentCabinet.allowReturnByAdmin = val == 1 ? true : false;
    }
    settings.endGroup();

    settings.beginGroup("System");
    {
        currentCabinet.closeDrawerTimeOut = settings.value("CloseDrawerToLogoff", 30).toInt();
        currentCabinet.openDrawerTimeout = settings.value("OpenDrawerToAlarm", 30).toInt();
        currentCabinet.adminTimeOut = settings.value("AdminToLogOff", 30).toInt();
        currentCabinet.issuedToolsTimeout = settings.value("IssuedToolsTime", 30).toInt();
    }
    settings.endGroup();



    return true;
}


bool DataManager::postCabinetDetails()
{
    return true;
}




//----------------------------------------------------------------------
//
//  Database and cabinet integrity functions
//
//----------------------------------------------------------------------


//
//  checks the database matches the cabinet
//
void DataManager::compareCabinetToDatabase()
{

    for (int drwnum = 1; drwnum <= MaxDrawers; drwnum++)
    {
        qDebug() << "DataManager::compareCabinetToDatabase() Drawer number" << drwnum ;
        compareCabinetDrawerToDatabase(drwnum);
    }

}




/*-----------------
//
// checks cabinet draw matches database and fixes any problems
//
void DataManager::compareCabinetDrawerToDatabasexxx(int drwnum)
{

    int state;
    int dbstate;
    int issuedid;

    qDebug() << "DataManager::compareCabinetDrawerToDatabase() Drawer" << drwnum;
        DrawerStatus* drawer = MainController::getCabinetManagerInstance()->getDrawerStatus(drwnum);
        if (drawer == NULL) return;
        if (!drawer->present) return;

        ToolsAvailable tools(drawer);
        for (int toolnum = tools.first(); toolnum != -1; toolnum = tools.next())
        {
            state = tools.state();  // 1 = in, 0 = out
            dbstate = getToolStatusFromToolsTableModel(drwnum, toolnum);  // 1 = in, 0 = out
            issuedid = findToolTransaction(drwnum, toolnum, Issued);  // -1 if not issued
            qDebug() << "DataManager::compareCabinetDrawerToDatabase() Drawer" << drwnum << toolnum << "status" << state << dbstate << issuedid;

            if (state == 1 && dbstate == 1)
            {
                // cabinet and db drawer match
                qDebug() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "state matches" << state << dbstate;
                if (issuedid != -1)  // currently issued
                {
                    // thinks it is issued, mark transaction returned offline
                    //return offline
                    qWarning() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "Transaction tool issued";
                    updateToolTransactionWithId(issuedid, DataManager::Returned, "Returned offline");
                }
                else
                {
                    // all okay
                    qDebug() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "All okay";
                }
            }
            if (state == 1 && dbstate == 0)
            {
                // cabinet in but db out
                // set db status as in
                qWarning() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "cabinet in but db out" << state << dbstate;
                markToolStatusAsIn(drwnum, toolnum);
                if (issuedid != -1)  // currently issued
                {
                    // thinks it is issued, mark transaction returned offline
                    qWarning() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "Transaction tool issued";
                    updateToolTransactionWithId(issuedid, DataManager::Returned, "Returned offline");
                }
                else
                {
                    // ignore
                    qDebug() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "Tool not issued, ignore";
                }
            }
            if (state == 0 && dbstate == 1)
            {
                // cabinet out but db in
                qWarning() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "Cabinet out but db in" << state << dbstate;
                markToolStatusAsOut(drwnum, toolnum);
                if (issuedid != -1)  // currently issued
                {
                    //ignore
                    qDebug() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "Tool issued, ignore";
                }
                else
                {
                    // mark missing
                    qWarning() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "tool not issued, mark missing";
                    markToolStatusAsMissing(drwnum, toolnum);
                }
            }
            // cabinet and db as out
            if (state == 0 && dbstate == 0)
            {
                //okay
                qDebug() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "state and db out, okay" << state << dbstate;
                if (issuedid != -1)  // currently issued
                {
                    //okay
                    qDebug() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "tool issued, okay";
                }
                else
                {
                    // mark missing
                    qWarning() << "DataManager::compareCabinetToDatabase() Drawer/toolnum" << drwnum << toolnum << "tool not issued, mark missing";
                    markToolStatusAsMissing(drwnum, toolnum);
                }
            }
        }


}
----*/

/*----
//
// checks cabinet draw matches database and fixes any problems
//
void DataManager::compareCabinetDrawerToDatabase(int drwnum)
{

    int cabstate;
    int dbstate;
    int issuedid;
    bool change = false;

    qDebug() << "DataManager::compareCabinetDrawerToDatabase() Drawer" << drwnum;
    DrawerStatus* drawer = MainController::getCabinetManagerInstance()->getDrawerStatus(drwnum);
    if (drawer == NULL) return;
    if (!drawer->present) return;

    ToolsAvailable tools(drawer);
    for (int toolnum = tools.first(); toolnum != -1; toolnum = tools.next())
    {
        cabstate = tools.state() == 1 ? Returned : Issued;  // 1 = in, 0 = out
        dbstate = getToolStatusFromToolsTableModel(drwnum, toolnum);  // 1 = in, 0 = out
        issuedid = findToolTransaction(drwnum, toolnum, Issued);  // -1 if not issued
        qDebug() << "DataManager::compareCabinetDrawerToDatabase() Drawer" << drwnum << toolnum << "status" << cabstate << dbstate << issuedid;

        if (cabstate == Returned && issuedid != -1)
        {
            // thinks it is issued, mark transaction returned offline
            qWarning() << "DataManager::compareCabinetDrawerToDatabase() Drawer/toolnum" << drwnum << toolnum << "Transaction tool issued";
            updateToolTransactionWithId(issuedid, DataManager::Returned, "Returned offline");
            change = true;
        }
        else if (cabstate == Issued && issuedid == -1)
        {
            // mark missing
            qWarning() << "DataManager::compareCabinetDrawerToDatabase() Drawer/toolnum" << drwnum << toolnum << "tool not issued, mark missing";
            markToolAsMissing(drwnum, toolnum);
            change = true;
        }

        if (cabstate != dbstate)
        {
            // reset db status
            qWarning() << "DataManager::compareCabinetDrawerToDatabase() Drawer/toolnum" << drwnum << toolnum << "cabinet and database state no the same";
            resetToolStatus(drwnum, toolnum, cabstate);
            change = true;
        }
    }
    if (change)
        refreshToolsOut();
}
----*/
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//
// checks cabinet draw matches database and fixes any problems
//
void DataManager::compareCabinetDrawerToDatabase(int drwnum)
{

    int cabstate;
    int dbstate;
    int issuedid;
    bool change = false;
    bool missing;

    qDebug() << "DataManager::compareCabinetDrawerToDatabase() Drawer" << drwnum;
    DrawerStatus* drawer = MainController::getCabinetManagerInstance()->getDrawerStatus(drwnum);
    if (drawer == NULL) return;
    if (!drawer->present) return;

    QByteArray array((const char*)drawer->toolsPresentBitmap, 19);
    qDebug() << "DataManager::compareCabinetDrawerToDatabase() toolsPresentBitmap " << QString(array.toHex());

    ToolsAvailable tools(drawer);   // iterator for tools in tools status bitmap.
    for (int toolnum = tools.first(); toolnum != -1; toolnum = tools.next())
    {
        cabstate = tools.state() == 1 ? Returned : Issued;  // 1 = in, 0 = out
        dbstate = getToolStatus(drwnum, toolnum);  // 1 = in, 0 = out
        issuedid = findToolTransaction(drwnum, toolnum, Issued);  // -1 if not issued
        missing = isToolMissing(drwnum,toolnum);
//        qDebug() << "DataManager::compareCabinetDrawerToDatabase() Drawer" << drwnum << toolnum << "status" << cabstate << dbstate << issuedid << missing;

        if (cabstate == Returned && dbstate == Issued)
        {
            // thinks it is issued, mark transaction returned offline
            qWarning() << "DataManager::compareCabinetDrawerToDatabase() Drawer/toolnum" << drwnum << toolnum << "Tool returned???";
            resetToolStatus(drwnum, toolnum, cabstate);
            if (issuedid != -1)
            {
                // issued transaction returned offline
                qWarning() << "DataManager::compareCabinetDrawerToDatabase() Drawer/toolnum" << drwnum << toolnum << "Tool returned offline";
                updateToolTransactionWithId(issuedid, DataManager::Returned, "Returned offline");
            }
            change = true;
        }
        else if (cabstate == Issued && dbstate == Returned)
        {
            // mark missing
            qWarning() << "DataManager::compareCabinetDrawerToDatabase() Drawer/toolnum" << drwnum << toolnum << "tool not issued, mark missing";
            markToolAsMissing(drwnum, toolnum);
            change = true;
        }
        if (cabstate == Returned && missing)
        {
            qWarning() << "DataManager::compareCabinetDrawerToDatabase() Drawer/toolnum" << drwnum << toolnum << "tool now not missing";
            updateToolMissing(drwnum, toolnum, Trak::ToolNotMissing);
        }
    }
    if (change)
        refreshToolsOut();
}




//====================================================================


void DataManager::markToolStatusAsIn(int drwnum, int toolnum)
{

}

void DataManager::markToolStatusAsOut(int drwnum, int toolnum)
{

}


//
//  Sets the tool missing field in the toolTableModel and the database
//
void DataManager::markToolAsMissing(int drwnum, int toolnum)
{
    updateToolMissing(drwnum, toolnum, Trak::ToolMissing);
    updateToolStatus(drwnum, toolnum, Trak::Issued);
}

//
//  Sets the tool status in the toolTableModel and the database
//
void DataManager::resetToolStatus(int drwnum, int toolnum, int status)
{
    updateToolStatus(drwnum, toolnum, status);
    setTooldisplayListStatus(drwnum, toolnum, status);
}






//----------------------------------------------------------------------
//
//  Tail code model and functions
//
//----------------------------------------------------------------------


//
//  Create a model of the tail ids for the current customer
//

QSqlQueryModel* DataManager::getTailNumbersModel(int custid)
{
    static QSqlQueryModel* tailNumModel = NULL;

    if (tailNumModel == NULL)
    {
        // create model
        tailNumModel = new QSqlQueryModel;
        qDebug() << "DataManager::getTailNumbersModel Created model";

    }
    // build query string
    QString str = QString("select trailId, description, speeddial from jobs where custId = %1 ");
    //QString str = QString("select trailId, description, remark, CASE speeddial WHEN 1 THEN 'YES' ELSE 'NO' END AS spDial, CASE disabled WHEN 1 THEN 'YES' ELSE 'NO' END AS active from jobs where custId = %1 ");
    str = str.arg(custid);

    qDebug() << "DataManager::getTailNumbersModel setting query";
    tailNumModel->setQuery(str);
    if (tailNumModel->lastError().isValid())
        qDebug() << tailNumModel->lastError();

    // add column for shortened tail text
    int num = tailNumModel->columnCount();
    qDebug() << "DataManager::getTailNumbersModel column count " << num;
    if (!tailNumModel->insertColumn(num))
        qDebug() << "DataManager::getTailNumbersModel Could not insert new column";

    return tailNumModel;
}



QSqlQueryModel* DataManager::getTailNumbersAdminModel(int custid)
{
    static QSqlQueryModel* tailNumModel = NULL;

    if (tailNumModel == NULL)
    {
        // create model
        tailNumModel = new QSqlQueryModel;
        qDebug() << "DataManager::getTailNumbersAdminModel Created model";

    }
    // build query string
    //QString str = QString("select trailId, description, speeddial from jobs where custId = %1 ");
    QString str = QString("select trailId, description, remark, CASE speeddial WHEN 1 THEN 'YES' ELSE 'NO' END AS spDial, CASE disabled WHEN 1 THEN 'YES' ELSE 'NO' END AS active from jobs where custId = %1 ");
    str = str.arg(custid);

    qDebug() << "DataManager::getTailNumbersAdminModel setting query";
    tailNumModel->setQuery(str);
    if (tailNumModel->lastError().isValid())
        qDebug() << tailNumModel->lastError();


    return tailNumModel;
}







//
//  Sets the status in the tool display list for the requested tool
//

void DataManager::setTooldisplayListStatus(int drwnum, int toolnum, int sts)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
        return;

    // The code here should be in an update function.
    DrawerList* drawer = toolDrawerArray[drwnum - 1];
    for(int i = 0; i < drawer->tools.size(); ++i)
    {
        Tool *tool = drawer->tools[i].get();
        if (tool->toolNumber == toolnum)
        {
            tool->state = getToolState (drwnum, tool->toolNumber, true);

//            bool missing = isToolMissing(drwnum, toolnum);
//            if (missing)
//            {
//                tool->state = ToolStates::TOOL_MISSING;
//            }
//            else if (sts == Issued)
//            {
//                // if issued then need to check if by this user!
//                tool->state = ToolStates::TOOL_OUT;
//            }
//            else // if (sts == Returned)
//            {
//                tool->state = ToolStates::TOOL_IN;
//            }
            qDebug() <<"DataManager::setTooldisplayListStatus() set drawer/tool to status" << drwnum << toolnum << sts << (int)tool->state;
//            qDebug() << "DataManager::setTooldisplayListStatus() updating status " <<
//                        (tool->state == ToolStates::TOOL_MISSING ? "TOOL_MISSING" :
//                        tool->state == ToolStates::TOOL_OUT ? "TOOL_OUT" :
//                        tool->state == ToolStates::TOOL_OTHER_USER ? "TOOL_OTHER_USER" :
//                        tool->state == ToolStates::TOOL_IN ? "TOOL_IN" : "UNKNOWN STATE");

        }
    }

}


//
//  sets the tool status in the tool display list
//  Called when:
//      a tool has been removed or returned, onDrawerToolChanged()
//
void DataManager::setToolStatus(int drwnum, int toolnum, int sts)
{
    setTooldisplayListStatus(drwnum, toolnum, sts);
}




//
//  returns the tool details in TOOL
//
Tool* DataManager::getToolDetail(int drwnum, int toolnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
        return NULL;

    // The code here should be in an update function.
    DrawerList* drawer = toolDrawerArray[drwnum - 1];
    for(int i = 0; i < drawer->tools.size(); ++i)
    {
        Tool *tool = drawer->tools[i].get();
        if (tool->toolNumber == toolnum)
        {
            qDebug() <<"DataManager::getToolDetail() " << drwnum << toolnum << tool->name;
            return tool;
        }
    }
    qWarning() <<"DataManager::getToolDetail() Tool does not exist" << drwnum << toolnum;
    return NULL;
}




//
//  does the actual database update to the tool status
//

void DataManager::updateToolStatusInDB(int id, int psts, int sts)
{
    QString sql = "update itemkabdrawerbins \
        set status = %1, prevstatus = %2 \
        where id = %3 ";
    sql = sql.arg(sts).arg(psts).arg(id);

    QSqlQuery query(sql);
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolStatusInDB() Update failed" << query.lastError();
    }
    else
    {
        postToPortal(sql);
    }
    qDebug() << "DataManager::updateToolStatusInDB() updated id/status/prevstatus " << id << sts << psts;
}




//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  Tool transaction table functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------


void DataManager::updateToolTransactionStatus(int drwnum, int toolnum, int newtype, QString itemid)
{

    qDebug() << "DataManager::updateToolTransactionStatus() drawer/tool/status" << drwnum, toolnum, newtype;
    if (newtype == DataManager::Returned)
    {
        // does issued transaction exist?
        int id = findToolTransaction(drwnum, toolnum, Issued);
        if (id == -1)
        {
            // no transaction
            qWarning() << "updateToolTransactionStatus() No transaction exists for drawernum/toolnum " << drwnum << toolnum;
            insertToolTransaction(drwnum, toolnum, DataManager::Returned, itemid, "Tool returned without being issued");
            return;
        }
        // update transaction if it exists
        updateToolTransaction(drwnum, toolnum, DataManager::Issued, DataManager::Returned);
    }
    else  // issued
    {
        // insert new transaction
        insertToolTransaction(drwnum, toolnum, DataManager::Issued, itemid);
    }
}




void DataManager::updateToolTransaction(int drwnum, int toolnum, int curtype, int newtype)
{

    // update transaction if it exists
    QSqlQuery query;
    QString sql = "update kabemployeeitemtransactions \
                    set inDate = CURDATE(), inTime = CURTIME(), transType = %1 \
                    where kabId = '%2' and custId = %3 and transType = %4 and drawerNum = %5 and toolNum = %6";
    sql = sql.arg(newtype).arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(curtype).arg(drwnum).arg(toolnum);

    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolTransaction() sql update error " << query.lastError();

        // table maybe not updated with inTime field, this should be deleted when all DB's updated
        qWarning() << "DataManager::updateToolTransaction() Try update without inTime field ";
        QString sql = "update kabemployeeitemtransactions \
                        set inDate = CURDATE(), transType = %1 \
                        where kabId = '%2' and custId = %3 and transType = %4 and drawerNum = %5 and toolNum = %6";
        sql = sql.arg(newtype).arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(curtype).arg(drwnum).arg(toolnum);

        query.exec(sql);

        if (query.lastError().isValid())
        {
            qWarning() << "DataManager::updateToolTransaction() sql update error, 2nd try " << query.lastError();
        }
        else
        {
            postToPortal(sql);
        }


    }
    else
    {
        postToPortal(sql);
    }
}




void DataManager::insertToolTransaction(int drwnum, int toolnum, int ttype, QString itemid,  QString remark)
{

    QString wo;
    QString bn;
    if (currentCustomer.useWorkOrder)
        wo = currentUser.currentWorkOrder;
    else
        wo = "NONE";
    if (currentCustomer.useBatch)
        bn = currentUser.currentBatch;
    else
        bn = "NONE";


    QString sql = "insert into  kabemployeeitemtransactions \
                    (userId, transType, itemId, transDate, transTime, remarks, trailId, custId, drawerNum, toolNum, kabId, confirmed, outDate, WO, BN, transId) \
                    values ('%1', %2, '%3', CURDATE(), CURTIME(), '%4', '%5', %6, %7, %8, '%9', %10, CURDATE(), '%11', '%12', '007') ";
    sql = sql.arg(currentUser.userId)
            .arg(ttype)
            .arg(itemid)
            .arg(remark)
            .arg(currentUser.tailDesc)
            .arg(currentCustomer.custId)
            .arg(drwnum)
            .arg(toolnum)
            .arg(currentCabinet.kabId)
            .arg(0)
            .arg(wo)
            .arg(bn);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::insertToolTransaction() sql insert error " << query.lastError();
    }
    else
     {
         postToPortal(sql);
     }
}


//
//  finds the tool transaction and returns the id
// if no transaction found returns -1
//

int DataManager::findToolTransaction(int drwnum, int toolnum, int ttype)
{

    if (drwnum < 1 || drwnum > MaxDrawers)
        return -1;

    QString sql = "select id from kabemployeeitemtransactions \
                    where kabId = '%1' and custId = %2 and transType = %3 and drawerNum = %4 and toolNum = %5";
    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(ttype).arg(drwnum).arg(toolnum);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::findToolTransaction() sql select error " << query.lastError();
        return -1;
    }

    int cnt = query.size();
    if (cnt == 0)
    {
        return -1;
    }
    else if (cnt > 1)
    {
        qWarning() << "DataManager::findToolTransaction() more than one transaction selected " << cnt;
    }

    query.next();
    int id = query.value("id").toInt();
    qDebug() << "DataManager::findToolTransaction() Transaction id " << id;
    return id;

}




//
//  finds the tool user and returns the user id
// if no transaction found returns empty string
//

QString DataManager::findToolUser(int drwnum, int toolnum, int ttype)
{

    QString id = QString("");

    QString sql = "select id, userId from kabemployeeitemtransactions \
                    where kabId = '%1' and custId = %2 and transType = %3 and drawerNum = %4 and toolNum = %5";
    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(ttype).arg(drwnum).arg(toolnum);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::findToolUser() sql select error " << query.lastError();
        return NULL;
    }

    int cnt = query.size();
    if (cnt == 0)
    {
        return id;   // empty string
    }
    else if (cnt > 1)
    {
        qWarning() << "DataManager::findToolUser() more than one transaction selected " << cnt;
    }

    query.next();
    id = query.value("userId").toString();
    qDebug() << "DataManager::findToolUser() Transaction id " << id;
    return id;

}


//
//  updates a tool transaction with transaction and remark using record id.
//
void DataManager::updateToolTransactionWithId(int issuedid, int ttype, QString remark)
{
    QSqlQuery query;
    QString sql = "update kabemployeeitemtransactions \
                    set inDate = CURDATE(), inTime = CURTIME(), transType = %1, remarks = '%2' \
                    where id = '%3' ";
    sql = sql.arg(ttype).arg(remark).arg(issuedid);

    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolTransactionWithId() sql update error " << query.lastError();

        // table maybe not updated with inTime field, this should be deleted when all DB's updated
        qWarning() << "DataManager::updateToolTransactionWithId() Try update without inTime field ";
        QString sql = "update kabemployeeitemtransactions \
                        set inDate = CURDATE(), transType = %1, remarks = '%2' \
                        where id = '%3' ";
        sql = sql.arg(ttype).arg(remark).arg(issuedid);

        query.exec(sql);

        if (query.lastError().isValid())
        {
            qWarning() << "DataManager::updateToolTransactionWithId() sql update error 2nd time " << query.lastError();
        }
        else
         {
             postToPortal(sql);
         }

    }
    else
     {
         postToPortal(sql);
     }
}


//
//  Create a tools transaction model for requested drawer.
//      Currently not used
//

QSqlQueryModel* DataManager::loadTransactionsModelForDrawer(QObject* parent, int drwnum)
{
    (void)drwnum;  // not used

    QSqlQueryModel *model = new QSqlQueryModel(parent);

    QString sql = "select * from kabemployeeitemtransactions \
    where kabId = '%1' and custId = %2 and transType = %3 ";

    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(Issued);

    QSqlQuery query(sql);
    model->setQuery(query);

    if (model->lastError().isValid())
    {
        qDebug() << model->lastError();
        delete model;
        model = NULL;
    }
    return model;
}



//
//  create a toolsOut model and return the tools missing
//

QSqlQueryModel* DataManager::createToolsOutModel(QObject* parent)
{

    qDebug() << "DataManager::createToolsOutModel()";

//    QString sql = "select "
//                  "kabemployeeitemtransactions.outDate, "
//                  "kabemployeeitemtransactions.transTime, "
//                  "kabemployeeitemtransactions.UserId, "
//                  "users.fullName, "
//                  "tools.description, "
//                  "kabemployeeitemtransactions.drawerNum, "
//                  "kabemployeeitemtransactions.toolNum, "
//                  "kabemployeeitemtransactions.trailId "
//                  "from kabemployeeitemtransactions "
//                  "INNER JOIN tools ON tools.partNo = kabemployeeitemtransactions.itemId "
//                  "INNER JOIN users ON users.userId = kabemployeeitemtransactions.userId and "
//                  "  users.kabId = kabemployeeitemtransactions.kabId and "
//                  "  users.custid = kabemployeeitemtransactions.custId "
//                  "INNER JOIN itemkabdrawerbins ON itemkabdrawerbins.custId = kabemployeeitemtransactions.custId and "
//                  "  itemkabdrawerbins.kabId = kabemployeeitemtransactions.kabId and "
//                  "  itemkabdrawerbins.toolNumber = kabemployeeitemtransactions.toolNum and "
//                  "  itemkabdrawerbins.drawerNum = kabemployeeitemtransactions.drawerNum "
//                  "where  "
//                  "kabemployeeitemtransactions.kabId = '%1' and "
//                  "kabemployeeitemtransactions.custId = %2 and "
//                  "kabemployeeitemtransactions.transType = %3 and "
//                  "users.kabtrak = 1 and users.enabledKab = 1 and "
//                  "  itemkabdrawerbins.serviceable = 1 "
//                  "ORDER BY kabemployeeitemtransactions.outDate,kabemployeeitemtransactions.transTime DESC ";



    QString sql = "select "
                  "kabemployeeitemtransactions.outDate, "
                  "kabemployeeitemtransactions.transTime, "
                  "kabemployeeitemtransactions.UserId, "
                  "users.fullName, "
                  "tools.description, "
                  "kabemployeeitemtransactions.drawerNum, "
                  "kabemployeeitemtransactions.toolNum, "
                  "kabemployeeitemtransactions.trailId "
                  "from kabemployeeitemtransactions "
                  "INNER JOIN tools ON tools.partNo = kabemployeeitemtransactions.itemId "
                  "INNER JOIN users ON users.userId = kabemployeeitemtransactions.userId "
                  "where  "
                  "kabemployeeitemtransactions.kabId = '%1' and "
                  "kabemployeeitemtransactions.custId = %2 and "
                  "kabemployeeitemtransactions.transType = %3 and "
                  "users.kabtrak = 1 and users.enabledKab = 1 "
                  "ORDER BY kabemployeeitemtransactions.outDate,kabemployeeitemtransactions.transTime DESC ";

    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(Issued);

    QSqlQueryModel *model = new QSqlQueryModel(parent);
    model->setQuery(sql);
    if (model->lastError().isValid())
    {
        qWarning() << "DataManager::createToolsOutModel() sql error " << model->lastError();
    }

    model->setHeaderData(0, Qt::Horizontal, tr("Out Date"));
    model->setHeaderData(1, Qt::Horizontal, tr("Out Time"));
    model->setHeaderData(2, Qt::Horizontal, tr("Issued to"));
    model->setHeaderData(3, Qt::Horizontal, tr("Tool name"));
    model->setHeaderData(4, Qt::Horizontal, tr("Drawer"));
    model->setHeaderData(5, Qt::Horizontal, tr("Tool number"));
    model->setHeaderData(6, Qt::Horizontal, tr("Tail number"));

    toolsOutModel = model;
    return model;
}



//
//  returns the number of tools out.
//

int DataManager::getToolsOutCount()
{
    return toolsOutModel->rowCount();
}




//
// Refreshes the tools out model
//

void DataManager::refreshToolsOut()
{

    qDebug() << "DataManager::refreshToolsOut() Refreshing";
    QString queryStr = toolsOutModel->query().executedQuery();
    toolsOutModel->clear();
    toolsOutModel->query().clear();
    toolsOutModel->setQuery(queryStr);

}


//
//  Calculates the tools out for a given user and saves the count
//  in the toolsOutCountArray. The data is taken from the toolsOutModel.
//  Called when a user logs in.
//

void DataManager::toolsOutForUser(QString user)
{

    // clear array
    for (int j = 0; j < MaxDrawers; j++)
    {
        toolsOutCountArray[j] = 0;
    }

    QSqlQueryModel *model = toolsOutModel;
    for (int i = 0; i < model->rowCount(); ++i) {
        QString usr = model->record(i).value("userId").toString();
        if (usr == user)
        {
            // current user
            int drwnum = model->record(i).value("drawerNum").toInt();
            if (drwnum >= 1 && drwnum <= MaxDrawers)
            {
                toolsOutCountArray[drwnum-1] += 1;   // inc count
                qDebug() << "DataManager::toolsOutForUser() " << toolsOutCountArray[drwnum-1];
            }
        }
    }
}


//
//  Changes the tools out count when a tool is removed or replaced.
//

void DataManager::incToolsOutForUser(int drwnum, int sts)
{
    qDebug() << "DataManager::incToolsOutForUser() drwnum/status " << drwnum << sts;
    if (sts == Issued)
        toolsOutCountArray[drwnum-1] += 1;
    else
        toolsOutCountArray[drwnum-1] -= 1;
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  user model functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------


//
//  Load the user model
//

QSqlQueryModel* DataManager::loadUserModel(QObject* parent)
{

    qDebug() << "DataManager::loadUserModel";
    // build query string
    QString sql = QString("select * from users where kabId = '%1' and custId = %2 and enabledKab = 1 and kabtrak = 1");
    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId);

    QSqlQueryModel *model = new QSqlQueryModel(parent);

    QSqlQuery query(sql);
    model->setQuery(query);

    if (model->lastError().isValid())
    {
        qDebug() << model->lastError();
        delete model;
        model = NULL;
    }
    userModel = model;
    return model;
}


QString DataManager::getUserName(QString userid)
{
    qDebug() <<"DataManager::getUserName() for user Id " << userid;
    QString name = "";

    QString sql = QString("select fullName from users where kabId = '%1' and custId = %2 and enabledKab = 1 and kabtrak = 1 and userId = '%3'");
    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(userid);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::findToolUser() sql select error " << query.lastError();
        return "";
    }

    int cnt = query.size();
    if (cnt == 0)
    {
        return "";   // empty string
    }
    else if (cnt > 1)
    {
        qWarning() << "DataManager::getUserName() more than one transaction selected " << cnt;
    }

    query.next();
    name = query.value("fullName").toString();
    qDebug() <<"DataManager::getUserName() " << name;
    return name;
}



QString DataManager::getUserNameFromModel(QString userid)
{
    qDebug() <<"DataManager::getUserName() for user Id " << userid;
    QSqlQueryModel* model = userModel;
    QString name = "";
    if (model)
    {
        for (int i = 0; i < model->rowCount(); ++i) {
            QString uid = model->record(i).value("userId").toString();
            if (uid == userid)
            {
                name = model->record(i).value("fullName").toString();
                qDebug() <<"DataManager::getUserName() " << name;
            }
        }
    }
    return name;
}




int DataManager::getUserAccessType(QString userid)
{
    qDebug() <<"DataManager::getUserAccessType() for user Id " << userid;
    int type = 0;

    QString sql = QString("select accessTypeKab from users where kabId = '%1' and custId = %2 and enabledKab = 1 and kabtrak = 1 and userId = '%3'");
    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(userid);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::getUserAccessType() sql select error " << query.lastError();
        return type;
    }

    int cnt = query.size();
    if (cnt == 0)
    {
        qWarning() << "DataManager::getUserAccessType() no transaction selected, return zero ";
        return type;   // zero
    }
    else if (cnt > 1)
    {
        qWarning() << "DataManager::getUserAccessType() more than one transaction selected " << cnt;
    }

    query.next();
    type = query.value("accessTypeKab").toInt();
    qDebug() <<"DataManager::getUserAccessType() " << type;
    return type;
}



int DataManager::getUserAccessTypeFromModel(QString userid)
{
    qDebug() <<"DataManager::getUserAccessType() for user Id " << userid;
    QSqlQueryModel* model = userModel;
    int type = 0;
    if (model)
    {
        for (int i = 0; i < model->rowCount(); ++i) {
            QString uid = model->record(i).value("userId").toString();
            if (uid == userid)
            {
                type = model->record(i).value("accessTypeKab").toInt();
                qDebug() <<"DataManager::getUserAccessType() " << type;
            }
        }
    }
    return type;
}



void DataManager::updateUserAccessCount(QString userid)
{

    QString sql = QString("update users set accessCountKab = accessCountKab + 1, lastAccess_kab = NOW() "
                          " where userId = '%1' and kabId = '%2' and custId = %3 and enabledKab = 1 and kabtrak = 1");
    sql = sql.arg(userid).arg(currentCabinet.kabId).arg(currentCustomer.custId);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateUserAccessCount() database error updating user " << userid;
        qWarning() << query.lastError();
    }
    else
    {
        postToPortal(sql);
    }

}



//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  Database maintenance functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------


//
//  creates a checkTable model
//
QSqlQueryModel* DataManager::checkTables()
{

    static QSqlQueryModel* checkTablesModel = NULL;

    QSqlQueryModel *model = checkTablesModel;
    if (checkTablesModel == NULL)
    {
        // create model
        checkTablesModel = new QSqlQueryModel;
        qDebug() << "DataManager::checkTables() Created model";

        QString sql = "check table  "
                      "cloudupdate, customer, employees, "
                "itemkabdrawerbins, itemkabdrawers, itemkabs, jobs, "
                "kabemployeeitemtransactions, kabemployeeitemtransactions_history, "
                "kabtransactionlog, location, soapuser, "
                "toolcategories, tools, transfertool, users ";

        //sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(Issued);

        model = checkTablesModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::checkTables() SQL error " << model->lastError();

        model->setHeaderData(0, Qt::Horizontal, tr("Table"));
        model->setHeaderData(1, Qt::Horizontal, tr("Op"));
        model->setHeaderData(2, Qt::Horizontal, tr("Msg type"));
        model->setHeaderData(3, Qt::Horizontal, tr("Msg text"));
    }
    else
    {
        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);
    }

    return model;
}


//
//  creates a checkTable results view
//
void DataManager::checkTablesResult()
{

    QSqlQueryModel* model = checkTables();

    QTableView *view = new QTableView;
    view->setModel(model);
    //view->resizeRowsToContents();
    view->setSortingEnabled(true);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->show();

}



//
//  creates a analyzeTable model
//
QSqlQueryModel* DataManager::analyzeTables()
{

    static QSqlQueryModel* analyzeTablesModel = NULL;

    QSqlQueryModel *model = analyzeTablesModel;
    if (analyzeTablesModel == NULL)
    {
        // create model
        analyzeTablesModel = new QSqlQueryModel;
        qDebug() << "DataManager::analyzeTables() Created model";

        QString sql = "analyze table  "
                      "cloudupdate, customer, employees, "
                "itemkabdrawerbins, itemkabdrawers, itemkabs, jobs, "
                "kabemployeeitemtransactions, kabemployeeitemtransactions_history, "
                "kabtransactionlog, location, soapuser, "
                "toolcategories, tools, transfertool, users ";


        model = analyzeTablesModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::analyzeTables() SQL error " << model->lastError();

        model->setHeaderData(0, Qt::Horizontal, tr("Table"));
        model->setHeaderData(1, Qt::Horizontal, tr("Op"));
        model->setHeaderData(2, Qt::Horizontal, tr("Msg type"));
        model->setHeaderData(3, Qt::Horizontal, tr("Msg text"));
    }
    else
    {
        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);
    }

    return model;
}


//
//  creates a analyzeTable results view
//
void DataManager::analyzeTablesResult()
{

    QSqlQueryModel* model = analyzeTables();

    QTableView *view = new QTableView;
    view->setModel(model);
    //view->resizeRowsToContents();
    view->setSortingEnabled(true);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->show();

}



//
//  creates a optimizeTables model
//
QSqlQueryModel* DataManager::optimizeTables()
{

    static QSqlQueryModel* optimizeTablesModel = NULL;

    QSqlQueryModel *model = optimizeTablesModel;
    if (optimizeTablesModel == NULL)
    {
        // create model
        optimizeTablesModel = new QSqlQueryModel;
        qDebug() << "DataManager::optimizeTables() Created model";

        QString sql = "optimize table  "
                      "cloudupdate, customer, employees, "
                "itemkabdrawerbins, itemkabdrawers, itemkabs, jobs, "
                "kabemployeeitemtransactions, kabemployeeitemtransactions_history, "
                "kabtransactionlog, location, soapuser, "
                "toolcategories, tools, transfertool, users ";

        //sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(Issued);

        model = optimizeTablesModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::optimizeTables() SQL error " << model->lastError();

        model->setHeaderData(0, Qt::Horizontal, tr("Table"));
        model->setHeaderData(1, Qt::Horizontal, tr("Op"));
        model->setHeaderData(2, Qt::Horizontal, tr("Msg type"));
        model->setHeaderData(3, Qt::Horizontal, tr("Msg text"));
    }
    else
    {
        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);
    }

    return model;
}


//
//  creates a optimizeTable results view
//
void DataManager::optimizeTablesResult()
{

    QSqlQueryModel* model = optimizeTables();

    QTableView *view = new QTableView;
    view->setModel(model);
    //view->resizeRowsToContents();
    view->setSortingEnabled(true);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->show();

}



//
//  creates a repairTables model
//
QSqlQueryModel* DataManager::repairTables()
{

    static QSqlQueryModel* repairTablesModel = NULL;

    QSqlQueryModel *model = repairTablesModel;
    if (repairTablesModel == NULL)
    {
        // create model
        repairTablesModel = new QSqlQueryModel;
        qDebug() << "DataManager::repairTables() Created model";

        QString sql = "repair table  "
                      "cloudupdate, customer, employees, "
                "itemkabdrawerbins, itemkabdrawers, itemkabs, jobs, "
                "kabemployeeitemtransactions, kabemployeeitemtransactions_history, "
                "kabtransactionlog, location, soapuser, "
                "toolcategories, tools, transfertool, users ";

        //sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(Issued);

        model = repairTablesModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::repairTables() SQL error " << model->lastError();

        model->setHeaderData(0, Qt::Horizontal, tr("Table"));
        model->setHeaderData(1, Qt::Horizontal, tr("Op"));
        model->setHeaderData(2, Qt::Horizontal, tr("Msg type"));
        model->setHeaderData(3, Qt::Horizontal, tr("Msg text"));
    }
    else
    {
        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);
    }

    return model;
}


//
//  creates a checkTable results view
//
void DataManager::repairTablesResult()
{

    QSqlQueryModel* model = repairTables();

    QTableView *view = new QTableView;
    view->setModel(model);
    //view->resizeRowsToContents();
    view->setSortingEnabled(true);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->show();

}





//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  Tool display list functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//
//  returns the tool display list for the requested drawer
//

DrawerList* DataManager::getTooldisplayList(int drwnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
        return NULL;
    return toolDrawerArray[drwnum - 1];

}










//
//  gets the tool display date for the current open drawer
//      Used by the userScreen when the drawer is open
//      Uses toolsTableModel so must be called after table initialised.
//      Only called at init when all tool drawers are built.
//

DrawerList* DataManager::buildToolsDrawer(int drwnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
    {
        qWarning() << "DataManager::buildToolsDrawer() Invalid drawer number" << drwnum;
        return NULL;
    }
    QList<ToolData*>* list = toolDataDrawerArray[drwnum-1];
    if (list == NULL)
    {
        return NULL;
    }
    DrawerList* drawer = new DrawerList();

    qDebug() << "DataManager::buildToolsDrawer rows " << list->size();
    for (int i = 0; i < list->size(); ++i)
    {
        ToolData* tool = list->at(i);
        if (tool->drawerNum == drwnum)
        {
            // add details to the drawer list
            ToolImageTransform* tt = new ToolImageTransform();
            tt->x = tool->cleft;
            tt->x = tool->cleft;
            tt->y = tool->ctop;
            tt->rotation = tool->angle;
            tt->scale = 1.0;
            tt->width = tool->width;
            tt->height = tool->height;
            if (tool->flip == 1)
                tt->hFlip = true;
            else
                tt->hFlip = false;
            if (tool->reverse == 1)
                tt->vFlip = true;
            else
                tt->vFlip = false;

            int toolnum = tool->toolNumber;
            QString desc = tool->description;
            QString stockcode = tool->stockcode;

            ToolStates sts = getToolState (drwnum, toolnum, true);

//            int tsts = tool->status;
//            int missing = tool->missing;
//            ToolStates sts = ToolStates::TOOL_IN;

//            if (missing == 1)
//            {
//                sts = ToolStates::TOOL_MISSING;

//            }
//            else if (tsts == Issued)
//            {
//                sts = ToolStates::TOOL_OUT;
//            }
//            else //if (tsts == Returned)
//            {
//                sts = ToolStates::TOOL_IN;
//            }

            QString image = tool->image_b;

                qDebug() << "DataManager::buildToolsDrawer added image " << image << toolnum << drwnum;
//                qDebug() << "DataManager::buildToolsDrawer() updating status " <<
//                            (sts == ToolStates::TOOL_MISSING ? "TOOL_MISSING" :
//                            sts == ToolStates::TOOL_OUT ? "TOOL_OUT" :
//                            sts == ToolStates::TOOL_OTHER_USER ? "TOOL_OTHER_USER" :
//                            sts == ToolStates::TOOL_IN ? "TOOL_IN" : "UNKNOWN STATE");


            drawer->addTool(new Tool(desc, sts, image, *tt, toolnum, stockcode, drwnum));
        }
    }
    return drawer;
}




void DataManager::buildToolsDrawerArray()
{
    for (int i = 0; i < MaxDrawers; i++)
    {
        if (MainController::getCabinetManagerInstance()->drawerExist(i+1))
        {
            toolDrawerArray[i] = buildToolsDrawer(i+1);
            qDebug() << "DataManager::buildToolsDrawerArray() built tools drawer " << i+1;
        }
        else
        {
            toolDrawerArray[i] = NULL;
            qDebug() << "DataManager::buildToolsDrawerArray() No drawer " << i+1;
        }
    }
}

DrawerList* DataManager::getDrawerTools(int dnum)
{
    if (dnum < 1 || dnum > MaxDrawers)
        return NULL;
    return toolDrawerArray[dnum - 1];
}







//
//  updates the tool display data for the current open drawer
//      Used by the userScreen when the drawer is open
//      Uses toolsTableModel so must be called after table initialised.
//

void DataManager::updateToolsDrawer(int dnum)
{

    if (dnum < 1 || dnum > MaxDrawers)
        return;

DrawerList* drawer = getDrawerTools(dnum);
    // The code here should be in an update function.
    for(int i = 0; i < drawer->tools.size(); ++i)
    {
        Tool *tool = drawer->tools[i].get();
        QString usr = findToolUser(dnum, tool->toolNumber, Issued);
        tool->state = getToolState (dnum, tool->toolNumber, (usr == currentUser.userId));

//        bool missing = isToolMissing(dnum, tool->toolNumber);
//        int status = getToolStatus(dnum, tool->toolNumber);

//        if (missing)
//        {
//            tool->state = ToolStates::TOOL_MISSING;
//        }
//        else if (status == Issued)
//        {
//            // is this current user?
//            QString usr = findToolUser(dnum, tool->toolNumber, Issued);
//            if (usr == currentUser.userId)
//            {
//                tool->state = ToolStates::TOOL_OUT;
//            }
//            else
//            {
//                tool->state = ToolStates::TOOL_OTHER_USER;
//            }
//            //qDebug() << "DataManager::updateToolsDrawer() updating status drawer/user/curuser" << dnum << usr << currentUser.userId;
//        }
//        else
//        {
//            tool->state = ToolStates::TOOL_IN;
//        }


        qDebug() << "DataManager::updateToolsDrawer() updating status drawer//curuser" << dnum << currentUser.userId <<
                    (tool->state == ToolStates::TOOL_MISSING ? "TOOL_MISSING" :
                    tool->state == ToolStates::TOOL_OUT ? "TOOL_OUT" :
                    tool->state == ToolStates::TOOL_OTHER_USER ? "TOOL_OTHER_USER" :
                    tool->state == ToolStates::TOOL_IN ? "TOOL_IN" : "UNKNOWN STATE");
    }
}



//
//      returns the Tool::State for the requested tool
//

ToolStates DataManager::getToolState(int drwnum, int toolnum, bool currentuser)
{
    ToolData* tooldata = getToolData(drwnum, toolnum);
    ToolStates state;

    if (tooldata->serviceable == Trak::Unserviceable)
    {
        state = ToolStates::TOOL_UNSERVICEABLE;
    }
    else if (tooldata->transferred == Trak::Transferred)
    {
        state = ToolStates::TOOL_TRANSFERRED;
    }
    else if (tooldata->missing == Trak::ToolMissing)
    {
        state = ToolStates::TOOL_MISSING;
    }
    else if (tooldata->status == Trak::Returned)
    {
        state = ToolStates::TOOL_IN;
    }
    else  // tool must be issued
    {
        if (currentuser)
        {
            state = ToolStates::TOOL_OUT;
        }
        else
        {
            state = ToolStates::TOOL_OTHER_USER;
        }
    }
    qDebug() << "DataManager::getToolState() updating status drawer//tool" << drwnum << toolnum <<
                (state == ToolStates::TOOL_MISSING ? "TOOL_MISSING" :
                state == ToolStates::TOOL_OUT ? "TOOL_OUT" :
                state == ToolStates::TOOL_OTHER_USER ? "TOOL_OTHER_USER" :
                state == ToolStates::TOOL_UNSERVICEABLE ? "TOOL_UNSERVICEABLE" :
                state == ToolStates::TOOL_TRANSFERRED ? "TOOL_TRANSFERRED" :
                state == ToolStates::TOOL_IN ? "TOOL_IN" : "UNKNOWN STATE");
    return state;
}







//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  Support classes for tools and drawer data
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------



ToolImageTransform::ToolImageTransform() : x(0), y(0), rotation(0), scale(1)
{

}

ToolImageTransform::ToolImageTransform(float x, float y) : x(x), y(y), rotation(0), scale(1)
{

}


Tool::Tool()
{

}

Tool::Tool(QString name, ToolStates state, QString imageID, ToolImageTransform transform, int toolnum, QString stockcode, int drwnum)
    : name(name), state(state), imageID(imageID), transform(transform), toolNumber(toolnum), stockCode(stockcode), drawerNum(drwnum)
{

}



DrawerList::DrawerList()
{
}

void DrawerList::addTool(Tool *tool)
{
    tools.push_back(std::unique_ptr<Tool>(tool));
}





//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  Employee/User functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------

QSqlQueryModel* DataManager::getEmployeeList()
{
    static QSqlQueryModel* employeeListModel = NULL;

    QSqlQueryModel *model = employeeListModel;
    if (employeeListModel == NULL)
    {
        // create model
        employeeListModel = new QSqlQueryModel;
        qDebug() << "DataManager::getEmployeeList() Created model";

        QString sql = "SELECT userId, firstName,lastName,isUserKab,disabled, CASE isUserKab WHEN 1 THEN 'YES' ELSE 'NO' END, CASE disabled  WHEN 0 THEN 'NO' ELSE 'YES' END   FROM employees WHERE custId = %1";

        sql = sql.arg(currentCustomer.custId);

        model = employeeListModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::getEmployeeList() SQL error " << model->lastError();

//        model->setHeaderData(0, Qt::Horizontal, tr("UserID"));
//        model->setHeaderData(1, Qt::Horizontal, tr("Fist Name"));
//        model->setHeaderData(2, Qt::Horizontal, tr("Last Name"));
//        model->setHeaderData(3, Qt::Horizontal, tr("kabTRAK User"));
//        model->setHeaderData(4, Qt::Horizontal, tr("Disabled"));
    }
    else
    {
        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);
    }

    return model;
}


QSqlQueryModel* DataManager::getUsersList()
{
    static QSqlQueryModel* userListModel = NULL;

    QSqlQueryModel *model = userListModel;
    if (userListModel == NULL)
    {
        // create model
        userListModel = new QSqlQueryModel;
        qDebug() << "DataManager::getUsersList() Created model";

        QString sql = "SELECT userId, fullName, CASE accessTypeKab WHEN 1 THEN 'Administrator' ELSE 'User' END, CASE enabled WHEN 1 THEN 'YES' ELSE 'NO' END  FROM users WHERE (custId = %1) AND (kabtrak =1) AND (kabId = %2) AND (enabledKab = 1)";


        sql = sql.arg(currentCustomer.custId).arg(currentCabinet.kabId);

        model = userListModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::getUserList() SQL error " << model->lastError();

//        model->setHeaderData(0, Qt::Horizontal, tr("UserID"));
//        model->setHeaderData(1, Qt::Horizontal, tr("Name"));
//        model->setHeaderData(2, Qt::Horizontal,  tr("Active"));
//        model->setHeaderData(3, Qt::Horizontal, tr("Admin"));
    }
    else
    {
        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);
    }

    return model;
}


bool DataManager::isUserExists(QString token)
{

    // build query string
    QString str = QString("select fullName from users where userId = '%1' and kabId = '%2' and custId = %3 and  kabtrak = 1");
    str = str.arg(token).arg(currentCabinet.kabId).arg(currentCustomer.custId);
    qDebug() << "DataManager::isUserExists sql: " << str;

    QSqlQuery query;
    query.prepare(str);

    qDebug() << "DataManager::isUserExists " << token << currentCustomer.custId << currentCabinet.kabId;
    qDebug() << str;
    query.exec();
    qDebug() << "DataManager::isUserExist" << query.size();
    qDebug() << query.executedQuery();
    if (query.size() == 1)
    {
        // one and only one token/user exists
        query.next();


        return true;
    }
    return false;

}


bool DataManager::isEmployeeExists(QString token)
{

    // build query string
    QString str = QString("select * from employees where userId = '%1' and custId = %2");
    str = str.arg(token).arg(currentCustomer.custId);
    qDebug() << "DataManager::isEmployeeExists sql: " << str;

    QSqlQuery query;
    query.prepare(str);

    qDebug() << "DataManager::isEmployeeExists " << token << currentCustomer.custId ;
    qDebug() << str;
    query.exec();
    qDebug() << "DataManager::isEmployeeExist" << query.size();
    qDebug() << query.executedQuery();
    if (query.size() == 1)
    {
        // one and only one token/user exists
        query.next();


        return true;
    }
    return false;

}



bool DataManager::updateEmployeeGrant(QString userId, int status)
{
    QString sql = "UPDATE employees \
                     SET isUserKab = %3 \
                     WHERE userId = '%1' AND custId = %2 ";

    sql = sql.arg(userId)
              .arg(currentCustomer.custId)
              .arg(status);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateEmployeeGrant() sql update error " << query.lastError();
        return false;
    } else
    {
        postToPortal(sql);
        return true;
    }
}



bool DataManager::updateEmployeeRecord(QString puserId, QString pFirstName, QString pMiddlename, QString pLastName, QString pExtName, QString pPosition, QString pEmail,QString pMobileNumber, QString pUserName, QString pPassword, int pIsUser,  int pUserRole,int pisUser_kab, int pDisabled)
{
  QString sql = "UPDATE employees \
                 SET firstName = '%1', \
                 middleName = '%2',\
                 lastName = '%3',\
                 position = '%4',\
                 extName = '%5',\
                 email = '%6',\
                 mobileNumber = '%7',\
                 isUser = %8, \
                 isUserKab = %9, \
                 username = '%10' ,\
                 password = '%11', \
                 userRole = %12, \
                 disabled = %13 \
                 WHERE userId = '%14' AND custId = %15 ";

  sql = sql.arg(pFirstName) //1
          .arg(pMiddlename) // 2
          .arg(pLastName)   // 3
          .arg(pPosition)   // 4
          .arg(pExtName)    // 5
          .arg(pEmail)      // 6
          .arg(pMobileNumber) // 7
          .arg(pIsUser)     // 8
          .arg(pisUser_kab)  // 9
          .arg(pUserName)  //10
          .arg(pPassword)   //11
          .arg(pUserRole)   //12
          .arg(pDisabled)   //13
          .arg(puserId)     //14
          .arg(currentCustomer.custId); //15

  qDebug()<< "Update Employee SQL" << sql;

  QSqlQuery query;
  query.exec(sql);

  if (query.lastError().isValid())
  {
      qWarning() << "DataManager::updateEmployeeRecord() sql update error " << query.lastError();
      return false;
  } else
  {
      return true;
  }

}

bool DataManager::insertEmployeeRecord(QString puserId, QString pFirstName, QString pMiddlename, QString pLastName, QString pExtName, QString pPosition, QString pEmail,QString pMobileNumber, QString pUserName, QString pPassword, int pIsUser,  int pUserRole,int pisUser_kab, int pDisabled)
{

  QString sql = "INSERT INTO employees \
                 (userId,firstName,middleName,lastName,extName,position,email,mobileNumber,custId, createDate, createTime, isUser,isUserKab,username,password,userRole,disabled, cribId, lastvisit) \
                 VALUES \
                 ('%1','%2','%3','%4','%5','%6','%7','%8',%9,CURDATE(),CURTIME(),%10,%11,'%12','%13',%14,%15,'',CURDATE())";

  sql = sql.arg(puserId) //1
          .arg(pFirstName) //2
          .arg(pMiddlename) // 3
          .arg(pLastName)   // 4
          .arg(pExtName)   // 5
          .arg(pPosition)    // 6
          .arg(pEmail)      // 7
          .arg(pMobileNumber) // 8
          .arg(currentCustomer.custId)     // 9
//          .arg(QDate::currentDate().toString())  // 10
          .arg(0)  //11
          .arg(0)   //12
          .arg("")   //13
          .arg("")   //14
          .arg(0)     //15
          .arg(0); //16

  qDebug()<< "Insert Employee SQL" << sql;

  QSqlQuery query;
  query.exec(sql);

  if (query.lastError().isValid())
  {
      qWarning() << "DataManager::insertEmployeeRecord() sql update error " << query.lastError();
      return false;
  } else
  {
      postToPortal(sql);
      return true;
  }

}

bool DataManager::updateUserRecord(QString puserId, QString pFirstName, QString pMiddlename, QString pLastName, int pEnabledKab, int pAccessTypeKab)
{
    if(isUserExists(puserId)) {



        QString sql = "UPDATE users \
                       SET fullName = '%1', \
                       enabledKab = %2,\
                       accessTypeKab = %3\
                       WHERE userId = '%5' AND custId = %6 AND kabId = '%7' AND kabtrak = 1";
                sql = sql.arg(pFirstName + " " + pLastName)
                        .arg(pEnabledKab)
                        .arg(pAccessTypeKab)
                        .arg(puserId)
                        .arg(currentCustomer.custId)
                        .arg(currentCabinet.kabId);
        qDebug()<< "Update User SQL" << sql;

        QSqlQuery query;
        query.exec(sql);

        if (query.lastError().isValid())
        {
            qWarning() << "DataManager::updateUserRecord() sql update error " << query.lastError();
            return false;
        } else
        {
            postToPortal(sql);
            return true;
        }
    }
    return false;
}



bool DataManager::grantUserAccess(QString userId, QString Fullname)
 {

      QString sql = "insert into  users \
                    (userId,fullName, enabled, enabledKab, createdDate, accessCountKab, custId, accessTypeKab, kabId, kabtrak, pin) \
                    values ('%1', '%2', 1, 1, CURDATE(), 0, %3, 0, '%4',1, '') ";
    sql = sql.arg(userId)
            .arg(Fullname)
            .arg(currentCustomer.custId)
            .arg(currentCabinet.kabId);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::grantUserAccess() sql insert error " << query.lastError();
        return false;
    } else
    {
        DataManager::updateEmployeeGrant(userId,1);
        qDebug()<<"User "<<userId<<" kabTRAK access granted!" ;
        postToPortal(sql);
        return true;

    }
}

bool DataManager::revokeUserAccess(QString userId)
 {

    QString sql = "DELETE FROM users \
              WHERE (userId = '%1') AND (custId = %2) AND (kabId = '%3') ";
    sql = sql.arg(userId)
            .arg(currentCustomer.custId)
            .arg(currentCabinet.kabId);

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::revoketUserAccess() sql delete error " << query.lastError();
        return false;
    } else
    {
        DataManager::updateEmployeeGrant(userId,0);
        qDebug()<<"User "<<userId<<" kabTRAK access revoked!" ;
        postToPortal(sql);
        return true;

    }
}


QSqlQuery DataManager::getSelectedEmp(QString token)
{
     QString sql = "SELECT\n"
     "userId,\n"
     "firstName,\n"
     "middleName,\n"
     "lastName,\n"
     "extName,\n"
     "position,\n"
     "address1,\n"
     "address2,\n"
     "address3,\n"
     "mobileNumber,\n"
     "custId,\n"
     "isUser,\n"
     "isUserKab,\n"
     "username,\n"
     "`password`,\n"
     "userRole,\n"
     "disabled,\n"
     "email\n"
     "FROM\n"
     "employees\n"
     "WHERE\n"
     "custId = %1\n"
     "AND\n"
     "userId = '%2'\n";

     sql = sql.arg(currentCustomer.custId).arg(token);
     QSqlQuery query;
     if (query.exec(sql))
     {
          while (query.next())
          {
            return query;
          }

     }
  return query;

}




// Tail Functions

bool DataManager::isTailExists(QString token)
{

    // build query string
    QString str = QString("select * from jobs where custid = %1 and description = '%2'");
    str = str.arg(currentCustomer.custId).arg(token);
    qDebug() << "DataManager::isTailExist sql: " << str;

    QSqlQuery query;
    query.prepare(str);

    qDebug() << "DataManager::isTailExists " << token << currentCustomer.custId ;
    qDebug() << str;
    query.exec();
    qDebug() << "DataManager::isTailExist" << query.size();
    qDebug() << query.executedQuery();
    if (query.size() == 1)
    {
        // one and only one token/user exists
        query.next();


        return true;
    }
    return false;

}



QSqlQuery DataManager::getSelectedTail(QString token)

{
     QString sql = "SELECT trailid,description,custid,remark,speeddial,disabled FROM jobs WHERE custId = %1 AND description = '%2'";


     sql = sql.arg(currentCustomer.custId).arg(token);
     qDebug() << "Tail SQL" << sql;


     QSqlQuery query;
     if (query.exec(sql))
     {
          while (query.next())
          {
            return query;
          }

     }
  return query;

}


bool DataManager::insertTailRecord(QString pDescription, QString pRemark, int pSpeedDial, int pDisabled, int pCustID)
{
 // INSERT INTO table_name ( field1, field2,...fieldN ) VALUES ( value1, value2,...valueN );
  QString sql = "INSERT INTO jobs \
                 (description,remark,speeddial,disabled,custid) \
                 VALUES \
                 ('%1','%2',%3,%4,%5)";

  sql = sql.arg(pDescription) //1
          .arg(pRemark) // 2
          .arg(pSpeedDial)   // 3
          .arg(pDisabled)   // 4
          .arg(pCustID);    // 5

  qDebug()<< "Insert Tail SQL" << sql;

  QSqlQuery query;
  query.exec(sql);

  if (query.lastError().isValid())
  {
      qWarning() << "DataManager::insertTailRecord() sql update error " << query.lastError();
      return false;
  } else
  {
      postToPortal(sql);
      return true;
  }

}


bool DataManager::updateTailRecord(QString pDescription, QString pRemark, int pSpeedDial, int pDisabled, int pCustID)
{
 // INSERT INTO table_name ( field1, field2,...fieldN ) VALUES ( value1, value2,...valueN );
  QString sql = "UPDATE jobs \
                 SET remark = '%1', \
                 speeddial = %2, \
                 disabled = %3 \
                 WHERE \
                 description = '%4' AND custid = %5";

  sql = sql.arg(pRemark) //1
          .arg(pSpeedDial) //2
          .arg(pDisabled) // 3
          .arg(pDescription)   // 4
          .arg(currentCustomer.custId);    // 5

  qDebug()<< "Update Tail SQL" << sql;

  QSqlQuery query;
  query.exec(sql);

  if (query.lastError().isValid())
  {
      qWarning() << "DataManager::updateTailRecord() sql update error " << query.lastError();
      return false;
  } else
  {
      postToPortal(sql);
      return true;
  }

}


// End tail Functions



QSqlQuery DataManager::getSelectedUser(QString token)
{
     QString sql = "SELECT userId,fullName,enabled,enabledKab,custId,accessTypeKab, kabId, kabtrak,`status` FROM users WHERE custId = %1 AND userId = '%2' AND kabId = '%3' AND kabtrak = 1";

     sql = sql.arg(currentCustomer.custId).arg(token).arg(currentCabinet.kabId);
     qDebug() << "User SQL" << sql;
     QSqlQuery query;
     if (query.exec(sql))
     {
          while (query.next())
          {
            return query;
          }

     }
  return query;

}






//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  Admin cabinet functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------



QSqlQuery DataManager::getCabinetDetails()
{
    QString sql = "SELECT kabId,description,modelNumber, custId,firmwareVersion, serialNumber,locationId,notes FROM itemkabs WHERE kabId = '%1' AND custId = %2";


     sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId);
     qDebug() << "Cabinet SQL" << sql;
     QSqlQuery qryCabinet;
     if (qryCabinet.exec(sql))
     {
          while (qryCabinet.next())
          {
            return qryCabinet;
          }

     }
  return qryCabinet;

}

bool DataManager::updateCabinetRecord(QString kabId, int CustID, QString description, QString modelnumber, QString firmware, QString notes, QString serialnumber, int location )
{
    QString sql = "UPDATE itemkabs \
     SET \
     description = '%3', modelNumber = '%4', firmwareVersion = '%5', notes = '%6', serialNumber = '%7', locationId = %8 \
     WHERE \
     kabId = '%1'  AND custId = %2";

     sql = sql.arg(kabId).arg(CustID).arg(description).arg(modelnumber).arg(firmware).arg(notes).arg(serialnumber).arg(location);

      qDebug()<< "Update Cabinet SQL" << sql;

      QSqlQuery query;
      query.exec(sql);

      if (query.lastError().isValid())
      {
          qWarning() << "DataManager::updateCabinetRecord() sql update error " << query.lastError();
          return false;
      } else
      {
          postToPortal(sql);
          return true;
      }



}

QSqlQueryModel* DataManager::getLocationList()
{

    qDebug() << "DataManager::getLocationList";
    // build query string
    QString sql = QString("select * from location where custId = %1");
    sql = sql.arg(currentCustomer.custId);

    QSqlQueryModel *model = new QSqlQueryModel;

    QSqlQuery query(sql);
    model->setQuery(query);

    if (model->lastError().isValid())
    {
        qDebug() << model->lastError();
        delete model;
        model = NULL;
    }
    userModel = model;
    return model;
}


QSqlQuery DataManager::getLocationListQry()
{
    QString sql = QString("select * from location where custId = %1");
    sql = sql.arg(currentCustomer.custId);

     qDebug() << "Location SQL" << sql;
     QSqlQuery qryLocation;
     if (qryLocation.exec(sql))
     {

            return qryLocation;


     }
  return qryLocation;

}

QSqlQuery DataManager::getTailListQry()
{
    QString sql = QString("select id, trailId, description, custId from jobs where custId = %1");
    sql = sql.arg(currentCustomer.custId);

     qDebug() << "Tail SQL" << sql;
     QSqlQuery qryTail;
     if (qryTail.exec(sql))
     {

            return qryTail;


     }
  return qryTail;

}


QSqlQueryModel* DataManager::getDrawerList()
{
    static QSqlQueryModel* drawerListModel = NULL;

    QSqlQueryModel *model = drawerListModel;
    if (drawerListModel == NULL)
    {
        // create model
        drawerListModel = new QSqlQueryModel;
        qDebug() << "DataManager::getDrawerList() Created model";

         QString sql =
         "SELECT id,kabId,drawerCode, CASE  `status` WHEN 255 THEN 'NO' ELSE 'YES' END , remarks,indexCode,custId FROM itemkabdrawers WHERE kabId = '%1' AND custId = %2 ORDER BY drawerCode ASC";


        sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId);

        model = drawerListModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::getDrawerList() SQL error " << model->lastError();


    }
    else
    {
        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);
    }

    return model;
}

QSqlQueryModel* DataManager::getToolList(int DrawNum)
{
    static QSqlQueryModel* toolListModel = NULL;

    QSqlQueryModel *model = toolListModel;
        // create model
        toolListModel = new QSqlQueryModel;
        qDebug() << "DataManager::getToolList() Created model";

        QString sql =
                "SELECT \
                itemkabdrawerbins.toolNumber, \
                itemkabdrawerbins.drawerNum, \
                CASE  `status` WHEN 1 THEN 'IN' ELSE 'OUT' END , \
                itemkabdrawerbins.kabId, \
                itemkabdrawerbins.custId, \
                tools.description, \
                itemkabdrawerbins.itemId \
                FROM \
                itemkabdrawerbins \
                LEFT JOIN tools ON tools.partNo = itemkabdrawerbins.itemId \
                WHERE \
                itemkabdrawerbins.drawerNum = %1 AND \
                itemkabdrawerbins.kabId = '%2' AND \
                itemkabdrawerbins.custId = %3 \
                ORDER BY \
                itemkabdrawerbins.toolNumber ASC";

        sql = sql.arg(DrawNum).arg(currentCabinet.kabId).arg(currentCustomer.custId);
        qDebug()<< "getToolList SQL "<< sql;
        model = toolListModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::getToolList() SQL error " << model->lastError();


        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);


    return model;
}



QSqlQuery DataManager::getCabToolQry(const int DrawNum, int ToolNum)

{
    QString sql =  "SELECT \
                    itemkabdrawerbins.toolNumber, \
                    itemkabdrawerbins.drawerNum, \
                    CASE  `status` WHEN 1 THEN 'IN' ELSE 'OUT' END , \
                    itemkabdrawerbins.kabId, \
                    itemkabdrawerbins.custId, \
                    itemkabdrawerbins.missing, \
                    itemkabdrawerbins.calibration, \
                    itemkabdrawerbins.currentCalibrationDate, \
                    itemkabdrawerbins.lastCalibrationDate, \
                    itemkabdrawerbins.serviceable, \
                    itemkabdrawerbins.calibrated, \
                    itemkabdrawerbins.transferred, \
                    tools.description, \
                    itemkabdrawerbins.itemId, \
                    tools.stockcode \
                    FROM \
                    itemkabdrawerbins \
                    LEFT JOIN tools ON tools.partNo = itemkabdrawerbins.itemId \
                    WHERE \
                    itemkabdrawerbins.drawerNum = %1 AND \
                    itemkabdrawerbins.toolNumber = %2 AND \
                    itemkabdrawerbins.kabId = '%3' AND \
                    itemkabdrawerbins.custId = %4";

     sql = sql.arg(DrawNum).arg(ToolNum).arg(currentCabinet.kabId).arg(currentCustomer.custId);

     qDebug() << "getCabToolQuery SQL" << sql;
     QSqlQuery qryCabTool;
     if (qryCabTool.exec(sql))
     {
           while(qryCabTool.next())
          {
            return qryCabTool;
          }

     }
  return qryCabTool;

}

bool DataManager::setCabTool(const int drawNum, int toolNum, int serviceable, int calibration, int calibrated, int transferred,  QDate dateCalibration, QDate datelastCalibration )

{
    QString sql = "UPDATE itemkabdrawerbins "
        "SET "
        "serviceable = %1, calibration = %2, calibrated = %3, transferred  = %4, currentCalibrationDate = '%5', lastCalibrationDate = '%6'   "
        "WHERE "
        "custId = %7 AND kabId = '%8' AND drawerNum = %9 AND toolNumber = %10";

    sql = sql.arg(serviceable).arg(calibration).arg(calibrated).arg(transferred ).arg(dateCalibration.toString("yyyy-MM-dd")).arg(datelastCalibration.toString("yyyy-MM-dd")).arg(currentCustomer.custId).arg(currentCabinet.kabId).arg(drawNum).arg(toolNum);

    qDebug()<< "Update Cab Tool SQL" << sql;

    QSqlQuery query;
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::setCabTool() sql update error " << query.lastError();
        return false;
    } else
    {
        postToPortal(sql);
        // update tooldata array
        ToolData* tool =  getToolData(drawNum, toolNum);
        tool->serviceable = serviceable;
        tool->calibration = calibration;
        tool->calibrated = calibrated;
        tool->transferred = transferred;
        tool->currentCalibrationDate = dateCalibration;
        tool->lastCalibrationDate = datelastCalibration;
        qInfo() << "Cabinet tool update for tool number " << toolNum << " in drawer " << drawNum;
        qInfo() << "   serviceable to " << (serviceable == Trak::Serviceable ? "Serviceable" : "Unserviceable");
        qInfo() << "   transferred to " << (transferred == Trak::Transferred ? "Transferred" : "NotTransferred");
        qInfo() << "   calibration to " << (calibration == Trak::Yes ? "Yes" : "No");
        qInfo() << "   calibrated to " << (calibrated == Trak::Yes ? "Yes" : "No");
        qInfo() << "   currentCalibrationDate to " << dateCalibration.toString("yyyy-MM-dd");
        qInfo() << "   lastCalibrationDate to " << datelastCalibration.toString("yyyy-MM-dd");

        return true;
    }


}








//Transactions

bool DataManager::clearTransactions()
{
    // Clear transactions
    QString sql = "UPDATE kabemployeeitemtransactions \
     SET \
     transType = 1 \
     WHERE \
     kabId = '%1'  AND custId = %2 AND transType = 0";

     sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId);

      qDebug()<< "Clear Transaction SQL" << sql;

      QSqlQuery query;
      query.exec(sql);

      if (query.lastError().isValid())
      {
          qWarning() << "DataManager::clearTransactions() sql update error " << query.lastError();
          return false;
      } else
      {
          postToPortal(sql);
          return true;
      }


}

//Tool List Module

QSqlQueryModel* DataManager::getToolInventoryList()
{
    static QSqlQueryModel* inventoryToolListModel = NULL;

    QSqlQueryModel *model = inventoryToolListModel;
        // create model
        inventoryToolListModel = new QSqlQueryModel;
        qDebug() << "DataManager::getToolInventoryList() Created model";

         QString sql =
                 "SELECT \
                 id, \
                 PartNo, \
                 description, \
                 custId , \
                 stockcode, \
                 image_b, \
                 ispresent, \
                 serial, \
                 isKit, \
                 kitcount \
                 FROM tools \
                 WHERE \
                 custid = %1 \
                 ORDER BY \
                 description ASC";

        sql = sql.arg(currentCustomer.custId);
        qDebug()<< "getToolInventoryList SQL "<< sql;
        model = inventoryToolListModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::getToolInventoryList SQL error " << model->lastError();


        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);


    return model;
}

bool DataManager::toolExists(QString token)
{

    // build query string
    QString str = QString("select * from tools where custid = %1 and stockcode = '%2'");
    str = str.arg(currentCustomer.custId).arg(token);
    qDebug() << "DataManager::toollExists sql: " << str;

    QSqlQuery query;
    query.prepare(str);

    qDebug() << "DataManager::toolExists " << token << currentCustomer.custId ;
    qDebug() << str;
    query.exec();
    qDebug() << "DataManager::isToolExist" << query.size();
    qDebug() << query.executedQuery();
    if (query.size() == 1)
    {
        // one and only one token/user exists
        return true;
    }
    return false;
}


QSqlQueryModel* DataManager::getCabinetToolList()
{
    static QSqlQueryModel* toolCabinetListModel = NULL;

    QSqlQueryModel *model = toolCabinetListModel;
        // create model
        toolCabinetListModel = new QSqlQueryModel;
        qDebug() << "DataManager::getCabinetToolList() Created model";

         QString sql =
                 "SELECT \
                 itemkabdrawerbins.toolNumber, \
                 itemkabdrawerbins.drawerNum, \
                 CASE  `status` WHEN 1 THEN 'IN' ELSE 'OUT' END , \
                 itemkabdrawerbins.kabId, \
                 itemkabdrawerbins.custId, \
                 tools.description, \
                 itemkabdrawerbins.itemId \
                 FROM \
                 itemkabdrawerbins \
                 LEFT JOIN tools ON tools.partNo = itemkabdrawerbins.itemId \
                 WHERE \
                 itemkabdrawerbins.kabId = '%1' AND \
                 itemkabdrawerbins.custId = %2 \
                 ORDER BY \
                 itemkabdrawerbins.drawerNum ASC";

        sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId);
        qDebug()<< "getCabinetToolList SQL "<< sql;
        model = toolCabinetListModel;
        model->setQuery(sql);
        if (model->lastError().isValid())
            qWarning() << "DataManager::getCabinetToolList() SQL error " << model->lastError();


        QString queryStr = model->query().executedQuery();
        model->clear();
        model->query().clear();
        model->setQuery(queryStr);


    return model;
}





QSqlQuery DataManager::getSelectedTool(QString token)
{
     QString sql = "SELECT PartNo,description,stockcode,custId,serialNo,isKit,kitcount,catId FROM tools WHERE custId = %1 AND stockcode = '%2'";

     sql = sql.arg(currentCustomer.custId).arg(token);
     qDebug() << "DataManager::getSelectedTool() Tool Detail SQL" << sql;
     QSqlQuery query;
     if (query.exec(sql))
     {
          while (query.next())
          {
            return query;
          }

     }
  return query;

}

QSqlQuery DataManager::getToolCategoryList()
{

           QString sql =
                 "SELECT catId, description, custId  FROM toolcategories WHERE custId = %1";

        sql = sql.arg(currentCustomer.custId);
        qDebug()<< "DataManager::getToolCategoryList() SQL " << sql;

        QSqlQuery query;
        if(query.exec(sql))
        {
           return query;
        }

    return query;
}

bool DataManager::updateToolRecord(QString stockCode,  QString description, int isKit , int kitCount, int catid)
{
    QString sql = "UPDATE tools "
     "SET "
     "description = '%1', isKit = %2, kitcount = %3, catId  = %4 "
     "WHERE "
     "custId = %5 AND stockcode = '%6'";

     sql = sql.arg(description).arg(isKit).arg(kitCount).arg(catid).arg(currentCustomer.custId).arg(stockCode);

      qDebug()<< "DataManager::updateToolRecord() SQL" << sql;

      QSqlQuery query;
      query.exec(sql);

      if (query.lastError().isValid())
      {
          qWarning() << "DataManager::updateToolRecord() sql update error " << query.lastError();
          return false;
      } else
      {
          postToPortal(sql);
          return true;
      }



}

bool DataManager::insertToolRecord(QString stockCode,  QString description, int isKit , int kitCount, int catid)
{
    QString sql = "INSERT INTO  tools(description,catId,stockcode,isKit,kitcount,custId,image_b) "
     "VALUES('%1',%2,'%3',%4,%5, %6,'%7'); "
     "SET @tool_id = LAST_INSERT_ID(); "
     "UPDATE tools SET PartNo = CONCAT(@tool_id,'-',stockcode) ";
     sql = sql.arg(description).arg(catid).arg(stockCode).arg(isKit).arg(kitCount).arg(currentCustomer.custId).arg(stockCode + ".svg");

      qDebug()<< "Insert Tool SQL" << sql;

      QSqlQuery query;
      query.exec(sql);

      if (query.lastError().isValid())
      {
          qWarning() << "DataManager::insertToolRecord() sql update error " << query.lastError();
          return false;
      } else
      {
          postToPortal(sql);
          return true;
      }
}



bool DataManager::isCabToolExists(QString token)
{

    // build query string
    QString str = QString("select * from itemkabdrawerbins where custId = %1 and itemId = '%2' and kabId = %3");
    str = str.arg(currentCustomer.custId).arg(token).arg(currentCabinet.kabId);
    qDebug() << "DataManager::isCabToollExist sql: " << str;

    QSqlQuery query;
    query.prepare(str);

    qDebug() << "DataManager::isCabToolExists " << token << currentCustomer.custId ;
    qDebug() << str;
    query.exec();
    qDebug() << "DataManager::isCabToolExist" << query.size();
    qDebug() << query.executedQuery();
    if (query.size() == 1)
    {
        // one and only one token/user exists
        return true;
    }
    return false;
}


bool DataManager::deleteToolRecord(const QString stockCode)
{
    QString sql = "DELETE FROM tools WHERE custId = %1 AND stockcode = '%2'";

     sql = sql.arg(currentCustomer.custId).arg(stockCode);

      qDebug()<< "Delete Tool SQL" << sql;

      QSqlQuery query;
      query.exec(sql);

      if (query.lastError().isValid())
      {
          qWarning() << "DataManager::deleteToolRecord() sql update error " << query.lastError();
          return false;
      } else
      {
          postToPortal(sql);
          return true;
      }



}

// Portal


bool DataManager::postToPortal(const QString sSQL)
{
    QString sql = "INSERT INTO  cloudupdate(Custid,SQLString,DatePosted,posted) "
     "VALUES(%1,\"%2\",NOW(),0)";
     sql = sql.arg(currentCustomer.custId).arg(sSQL);

      qDebug()<< "Insert cloud update SQL" << sql;

      QSqlQuery query;
      query.exec(sql);

      if (query.lastError().isValid())
      {
          qWarning() << "DataManager::postToPortal() sql update error " << query.lastError();
          return false;
      } else
      {
          return true;
      }



}



//Dashboard

QSqlQuery DataManager::getCustomerData()
{
           QString sql =
                 "SELECT  companyName,  contactPerson, email, telNo, faxNo FROM customer WHERE id = %1";

        sql = sql.arg(currentCustomer.custId);
        qDebug()<< "getCustomerData SQL "<< sql;

        QSqlQuery query;
        if (query.exec(sql))
        {
             while (query.next())
             {
               return query;
             }

        }

    return query;
}


QSqlQuery DataManager::getCabinetData()
{
           QString sql =
                 "SELECT modelNumber, \
                 description, \
                 serialNumber, \
                 firmwareVersion \
                 FROM \
                 itemkabs \
                 WHERE \
                 custId = %1 AND kabId = '%2'";

        sql = sql.arg(currentCustomer.custId).arg(currentCabinet.kabId);
        qDebug()<< "getCustomerData SQL "<< sql;

        QSqlQuery query;
        if (query.exec(sql))
        {
             while (query.next())
             {
               return query;
             }

        }

    return query;
}

QSqlQuery DataManager::loadIsssueanceData(const QString sql_statement)
{
       QString sql;

       if(sql_statement.isEmpty())
       {
          sql =   "SELECT \
                  kabemployeeitemtransactions.userId, \
                  kabemployeeitemtransactions.transType, \
                  kabemployeeitemtransactions.itemId, \
                  kabemployeeitemtransactions.transDate, \
                  kabemployeeitemtransactions.transTime, \
                  kabemployeeitemtransactions.remarks, \
                  kabemployeeitemtransactions.trailId, \
                  kabemployeeitemtransactions.custId, \
                  kabemployeeitemtransactions.drawerNum, \
                  kabemployeeitemtransactions.toolNum, \
                  kabemployeeitemtransactions.kabId, \
                  kabemployeeitemtransactions.outDate, \
                  tools.description, \
                  users.fullName \
                  FROM \
                  kabemployeeitemtransactions \
                  LEFT JOIN tools ON tools.PartNo = kabemployeeitemtransactions.itemId \
                  LEFT JOIN users ON users.userId = kabemployeeitemtransactions.userId \
                  WHERE \
                  kabemployeeitemtransactions.custId = %1 AND \
                  kabemployeeitemtransactions.kabId = '%2' AND \
                  kabemployeeitemtransactions.transType = 0 \
                  ORDER BY \
                  kabemployeeitemtransactions.transDate ASC";
       }
       else
        {
            sql = sql_statement;
        }


        sql = sql.arg(currentCustomer.custId).arg(currentCabinet.kabId);
        qDebug()<< "loadIssueanceData SQL "<< sql;

        QSqlQuery query;
        if (query.exec(sql))
        {
             while (query.next())
             {
               return query;
             }

        }

    return query;
}


QSqlQueryModel* DataManager::loadIssueanceModel(const QString sql_statement)
{
    QString sql;
    static QSqlQueryModel* issueanceModel = NULL;

    if(sql_statement.isEmpty())
    {
       sql =   "SELECT \
               kabemployeeitemtransactions.outDate, \
               kabemployeeitemtransactions.transTime, \
               users.fullName, \
               tools.description, \
               kabemployeeitemtransactions.drawerNum, \
               kabemployeeitemtransactions.toolNum, \
               kabemployeeitemtransactions.trailId \
               FROM \
               kabemployeeitemtransactions \
               LEFT JOIN tools ON tools.PartNo = kabemployeeitemtransactions.itemId \
               LEFT JOIN users ON users.userId = kabemployeeitemtransactions.userId \
               WHERE \
               kabemployeeitemtransactions.custId = %1 AND \
               kabemployeeitemtransactions.kabId = '%2' AND \
               kabemployeeitemtransactions.transType = 0 \
               ORDER BY \
               kabemployeeitemtransactions.transTime DESC";
    }
    else
     {
         sql = sql_statement;
     }

    QSqlQueryModel *model = issueanceModel;
        // create model
    issueanceModel = new QSqlQueryModel;
    qDebug() << "DataManager::loadIssueanceModel() Created model";

    sql = sql.arg(currentCustomer.custId).arg(currentCabinet.kabId);
    qDebug()<< "loadIssueanceModel SQL "<< sql;
    model = issueanceModel;
    model->setQuery(sql);
    if (model->lastError().isValid())
       qWarning() << "DataManager::loadIssueanceModel() SQL error " << model->lastError();


     QString queryStr = model->query().executedQuery();
     model->clear();
     model->query().clear();
     model->setQuery(queryStr);


    return model;
}

QSqlQueryModel* DataManager::loadAllTransactionModel()
{
    QString sql;
    static QSqlQueryModel* transactionModel = NULL;

       sql =   "SELECT \
               kabemployeeitemtransactions.outDate, \
               kabemployeeitemtransactions.inDate, \
               users.fullName, \
               tools.description, \
               kabemployeeitemtransactions.drawerNum, \
               kabemployeeitemtransactions.toolNum, \
               kabemployeeitemtransactions.trailId \
               FROM \
               kabemployeeitemtransactions \
               LEFT JOIN tools ON tools.PartNo = kabemployeeitemtransactions.itemId \
               LEFT JOIN users ON users.userId = kabemployeeitemtransactions.userId \
               WHERE \
               kabemployeeitemtransactions.custId = %1 AND \
               kabemployeeitemtransactions.kabId = '%2' AND \
               kabemployeeitemtransactions.transType = 1 \
               ORDER BY \
               kabemployeeitemtransactions.outDate DESC";

    QSqlQueryModel *model = transactionModel;
        // create model
    transactionModel = new QSqlQueryModel;
    qDebug() << "DataManager::loadAllTransactions() Created model";

    sql = sql.arg(currentCustomer.custId).arg(currentCabinet.kabId);
    qDebug()<< "loadAllTransactionsModel SQL "<< sql;
    model = transactionModel;
    model->setQuery(sql);
    if (model->lastError().isValid())
       qWarning() << "DataManager::loadAllTransactionModel() SQL error " << model->lastError();


     QString queryStr = model->query().executedQuery();
     model->clear();
     model->query().clear();
     model->setQuery(queryStr);


    return model;
}









//----------------------------------------------------------------------
//----------------------------------------------------------------------
//
//  tool data structure functions
//
//----------------------------------------------------------------------
//----------------------------------------------------------------------


ToolData::ToolData()
{
    //qDebug() << "ToolData::ToolData() Empty tool data class";
    id = -1;
}

ToolData::ToolData(int drwnum, int toolnum)
{

    id =-1;
    if (drwnum < 1 || drwnum > MaxDrawers)
    {
        id = -1;
        return;
    }


    ToolsTableModel *model = new ToolsTableModel();

    QString sql = " select \
        bins.id, bins.binId, bins.drawerId,  bins.kabId, bins.custId, bins.itemId, bins.prevstatus, \
        bins.`status`, bins.toolNumber, bins.drawerNum, bins.missing, bins.cleft, bins.ctop, \
        bins.visible, bins.blockIndex, bins.angle, bins.width, bins.height, bins.currentCalibrationDate, \
        bins.lastCalibrationDate, bins.calibration, bins.serviceable, bins.serialNumber, \
        bins.calibrated, bins.transferred, bins.flip, bins.reverse, \
        tools.description, tools.stockcode, tools.kitcount, tools.isKit, \
        tools.image_b, tools.image_r, tools.image_g, tools.image_w  \
    FROM itemkabdrawerbins as bins \
    INNER JOIN tools ON tools.partNo = bins.itemId \
    WHERE bins.kabId = '%1' AND bins.custId = %2 AND bins.drawerNum = %3 AND bins.toolNumber = %4";

    sql = sql.arg(MainController::getDataManagerInstance()->currentCabinet.kabId).arg(MainController::getDataManagerInstance()->currentCustomer.custId).arg(drwnum).arg(toolnum);

    QSqlQuery query(sql);
    model->setQuery(query);

    if (model->lastError().isValid())
    {
        qWarning() << "ToolData::ToolData() sql error " << model->lastError();
        delete model;
        model = NULL;
        id = -1;
        return;
    }
    if (model->rowCount() < 1)
    {
        delete model;
        id =-1;
        qWarning() << "ToolData::ToolData() Could not find tool " << toolnum << " for drawer " << drwnum;
        return;
    }

    int index = 0;

    if (model == NULL)
    {
        qWarning() << "ToolData::ToolData() Could not find ToolsTableModel for drawer " << drwnum;
        id = -1;
        return;
    }

    changed = false;
    id = model->record(index).value("id").toInt();
    binId = model->record(index).value("binId").toString();
    drawerId = model->record(index).value("drawerId").toString();
    kabId = model->record(index).value("kabId").toString();
    custId = model->record(index).value("custId").toInt();
    itemId = model->record(index).value("itemId").toString();
    prevstatus = model->record(index).value("prevstatus").toInt();
    status = model->record(index).value("status").toInt();
    toolNumber = model->record(index).value("toolNumber").toInt();
    drawerNum = model->record(index).value("drawerNum").toInt();
    missing = model->record(index).value("missing").toInt();
    cleft = model->record(index).value("cleft").toFloat();
    ctop = model->record(index).value("ctop").toFloat();
    visible = model->record(index).value("visible").toInt();
    blockIndex = model->record(index).value("blockIndex").toInt();
    angle = model->record(index).value("angle").toFloat();
    width = model->record(index).value("width").toFloat();
    height = model->record(index).value("height").toFloat();
    currentCalibrationDate = model->record(index).value("currentCalibrationDate").toDate();
    lastCalibrationDate = model->record(index).value("lastCalibrationDate").toDate();
    calibration = model->record(index).value("calibration").toInt();
    serviceable = model->record(index).value("serviceable").toInt();
    serialNumber = model->record(index).value("serialNumber").toString();
    calibrated = model->record(index).value("calibrated").toInt();
    transferred = model->record(index).value("transferred").toInt();
    flip = model->record(index).value("flip").toInt();
    reverse = model->record(index).value("reverse").toInt();

    description = model->record(index).value("description").toString();
    stockcode = model->record(index).value("stockcode").toString();
    kitcount = model->record(index).value("kitcount").toInt();
    isKit = model->record(index).value("isKit").toInt();
    image_b = model->record(index).value("image_b").toString();
}



//QList<ToolData*>* toolDataDrawerArray[Trak::MaxDrawerNumber];


void DataManager::buildToolsDataArray()
{
    for (int i = 0; i < Trak::MaxDrawerNumber; i++)
    {
        if (MainController::getCabinetManagerInstance()->drawerExist(i+1))
        {
            toolDataDrawerArray[i] = buildToolsDataDrawer(i+1);
            qDebug() << "DataManager::buildToolsDataArray() built tools drawer " << i+1;
        }
        else
        {
            toolDataDrawerArray[i] = NULL;
            qDebug() << "DataManager::buildToolsDataArray() No drawer " << i+1;
        }
    }
}


QList<ToolData*>* DataManager::buildToolsDataDrawer(int drwnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
        return NULL;


    ToolsTableModel *model = new ToolsTableModel();

    QString sql = " select \
        bins.id, bins.binId, bins.drawerId,  bins.kabId, bins.custId, bins.itemId, bins.prevstatus, \
        bins.`status`, bins.toolNumber, bins.drawerNum, bins.missing, bins.cleft, bins.ctop, \
        bins.visible, bins.blockIndex, bins.angle, bins.width, bins.height, bins.currentCalibrationDate, \
        bins.lastCalibrationDate, bins.calibration, bins.serviceable, bins.serialNumber, \
        bins.calibrated, bins.transferred, bins.flip, bins.reverse, \
        tools.description, tools.stockcode, tools.kitcount, tools.isKit, \
        tools.image_b, tools.image_r, tools.image_g, tools.image_w  \
    FROM itemkabdrawerbins as bins \
    INNER JOIN tools ON tools.partNo = bins.itemId \
    WHERE bins.kabId = '%1' AND bins.custId = %2 AND bins.drawerNum = %3";
    sql = sql.arg(currentCabinet.kabId).arg(currentCustomer.custId).arg(drwnum);

    QSqlQuery query(sql);
    model->setQuery(query);

    if (model->lastError().isValid())
    {
        qDebug() << model->lastError();
        delete model;
        model = NULL;
    }
    else
    {


        QList<ToolData*>* list = new QList<ToolData*>;
        for (int index = 0; index < model->rowCount(); ++index) {

            ToolData* tool = new ToolData();

            tool->changed = false;
            tool->id = model->record(index).value("id").toInt();
            tool->binId = model->record(index).value("binId").toString();
            tool->drawerId = model->record(index).value("drawerId").toString();
            tool->kabId = model->record(index).value("kabId").toString();
            tool->custId = model->record(index).value("custId").toInt();
            tool->itemId = model->record(index).value("itemId").toString();
            tool->prevstatus = model->record(index).value("prevstatus").toInt();
            tool->status = model->record(index).value("status").toInt();
            tool->toolNumber = model->record(index).value("toolNumber").toInt();
            tool->drawerNum = model->record(index).value("drawerNum").toInt();
            tool->missing = model->record(index).value("missing").toInt();
            tool->cleft = model->record(index).value("cleft").toFloat();
            tool->ctop = model->record(index).value("ctop").toFloat();
            tool->visible = model->record(index).value("visible").toInt();
            tool->blockIndex = model->record(index).value("blockIndex").toInt();
            tool->angle = model->record(index).value("angle").toFloat();
            tool->width = model->record(index).value("width").toFloat();
            tool->height = model->record(index).value("height").toFloat();
            tool->changedCalibration = false;
            tool->currentCalibrationDate = model->record(index).value("currentCalibrationDate").toDate();
            tool->lastCalibrationDate = model->record(index).value("lastCalibrationDate").toDate();
            tool->calibration = model->record(index).value("calibration").toInt();
            tool->serviceable = model->record(index).value("serviceable").toInt();
            tool->serialNumber = model->record(index).value("serialNumber").toString();
            tool->calibrated = model->record(index).value("calibrated").toInt();
            tool->transferred = model->record(index).value("transferred").toInt();
            tool->flip = model->record(index).value("flip").toInt();
            tool->reverse = model->record(index).value("reverse").toInt();

            tool->description = model->record(index).value("description").toString();
            tool->stockcode = model->record(index).value("stockcode").toString();
            tool->kitcount = model->record(index).value("kitcount").toInt();
            tool->isKit = model->record(index).value("isKit").toInt();
            tool->image_b = model->record(index).value("image_b").toString();

            list->append(tool);


//            qDebug() << "DataManager::buildToolsDataDrawer() id/drawer/tool/status/prevstatus " << tool->id
//                     << tool->drawerNum << tool->toolNumber << tool->status << tool->prevstatus << tool->description;


        }
        delete model;
        return list;
    }
    return NULL;
}


ToolData* DataManager::getToolData(int drwnum, int toolnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
    {
        qWarning() << "DataManager::getToolData() Invalid drawer number" << drwnum;
        return NULL;
    }
    QList<ToolData*>* list = toolDataDrawerArray[drwnum-1];
    if (list == NULL)
    {
        return NULL;
    }
    for (int i = 0; i < list->size(); ++i)
    {
        ToolData* tool = list->at(i);
        if (tool->toolNumber == toolnum)
        {
            return tool;
        }
    }
    return NULL;
}








//
//  finds the tools in the tool table that has changed status
//  then updates the tool status in the database table and the
//  tool transaction table.
//      The status has changed when the prevstatus and the status
//      field are different.
//
void DataManager::updateToolsChanged(int drwnum)
{
    if (drwnum < 1 || drwnum > MaxDrawers)
    {
        qWarning() << "DataManager::updateToolsChanged() Invalid drawer number" << drwnum;
        return;
    }
    QList<ToolData*>* list = toolDataDrawerArray[drwnum-1];
    if (list == NULL)
    {
        return;
    }
    for (int i = 0; i < list->size(); ++i)
    {
        ToolData* tool = list->at(i);
        if (tool->changed)
        {
            updateTool (tool);
            tool->changed = false;
        }
    }
}

void DataManager::updateToolsChangedAllDrawers()
{
    for (int i = 0; i < Trak::MaxDrawerNumber; i++)
    {
        if (MainController::getCabinetManagerInstance()->drawerExist(i+1))
        {
            qDebug() << "DataManager::updateToolsChangedAllDrawers() Updated changed tools in drawer " << i+1;
            updateToolsChanged(i+1);
        }
    }
}





//
//  update tool details in database
//

void DataManager::updateTool(ToolData* tool)
{

    qDebug() << "DataManager::updateTool() status/prevstatus/changed " << tool->status << tool->prevstatus << tool->changed;
    if (tool->changed && tool->status == tool->prevstatus)
    {

        if (tool->missing == 1 && tool->status == Issued)
        {
            // do not create a transaction if tool still not returned
        }
        else
        {
            updateToolTransactionStatus(tool->drawerNum,tool->toolNumber, tool->status, tool->itemId);
        }

        if (tool->status == tool->prevstatus)
        {
            // status has changed
            if (tool->status == Trak::Issued) tool->prevstatus = Trak::Returned; else tool->prevstatus = Trak::Issued;
        }

        QString sql = "update itemkabdrawerbins "
                      "set "
                      "status = %2, "
                      "prevstatus = %3, "
                      "missing = %4 "
                      "where id = %1 ";
        sql = sql.arg(tool->id).arg(tool->status).arg(tool->prevstatus).arg(tool->missing);

        qDebug() << "DataManager::updateTool() sql: " << sql;
        qInfo() << "Updateded tool/ drawer/toolnumber/id " <<tool->stockcode << tool->drawerNum << tool->toolNumber << tool->id;
        qInfo() << "with /status/prevstatus/missing" << tool->status << tool->prevstatus << tool->missing;

        QSqlQuery query(sql);
        query.exec(sql);

        if (query.lastError().isValid())
        {
            qWarning() << "DataManager::updateTool() Update failed" << query.lastError();
        }
        else
        {
            qInfo() << "Updated database tool status";
            postToPortal(sql);
        }
    }
}




void DataManager::updateToolCalibrationDate(ToolData* tool)
{

    // update calibration dates if changed.
    QString sql = "update itemkabdrawerbins "
                  "set "
                  "currentCalibrationDate = '%2', "
                  "lastCalibrationDate = '%3' "
                  "where id = %1 ";
    sql = sql.arg(tool->id).arg(tool->currentCalibrationDate.toString("yyyy-MM-dd")).arg(tool->lastCalibrationDate.toString("yyyy-MM-dd"));

    qDebug() << "DataManager::updateToolCalibrationDate() sql: " << sql;

    QSqlQuery query(sql);
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolCalibrationDate() Update calibration date failed" << query.lastError();
    }
    else
    {
        qInfo() << "Updated database tool calibration date to " << tool->currentCalibrationDate;
        postToPortal(sql);
    }
    tool->changedCalibration = false;
}




void DataManager::updateToolCalibrationInfo(ToolData* tool)
{

    // update calibration dates if changed.
    QString sql = "update itemkabdrawerbins "
                  "set "
                  "currentCalibrationDate = '%2', "
                  "lastCalibrationDate = '%3' "
                  "where id = %1 ";
    sql = sql.arg(tool->id).arg(tool->currentCalibrationDate.toString("yyyy-MM-dd")).arg(tool->lastCalibrationDate.toString("yyyy-MM-dd"));

    qDebug() << "DataManager::updateToolCalibrationDate() sql: " << sql;

    QSqlQuery query(sql);
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolCalibrationDate() Update calibration date failed" << query.lastError();
    }
    else
    {
        qInfo() << "Updated database tool calibration date to " << tool->currentCalibrationDate;
        postToPortal(sql);
    }
    tool->changedCalibration = false;
}




void DataManager::updateToolServiceable(ToolData* tool)
{

    QString sql = "update itemkabdrawerbins "
                  "set "
                  "serviceable = %2 "
                  "where id = %1 ";
    sql = sql.arg(tool->id).arg(tool->serviceable);

    qDebug() << "DataManager::updateToolServiceable() sql: " << sql;

    QSqlQuery query(sql);
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolServiceable() Update calibration date failed" << query.lastError();
    }
    else
    {
        qInfo() << "Updated database tool serviceable status " << tool->serviceable;
        postToPortal(sql);
    }
}



//
//  Checks the toolsTableModel missing field
//  Returns true if missing field is set to 1
//

bool DataManager::isToolMissing(int drwnum, int toolnum)
{

    if (drwnum < Trak::MinDrawerNumber || drwnum > Trak::MaxDrawerNumber)
    {
        qWarning() << "DataManager::isToolMissing() Invalid drawer number: " << drwnum;
        return false;
    }

    ToolData* tool = getToolData(drwnum, toolnum);
    if (tool == NULL)
    {
        qWarning() << "DataManager::isToolMissing() tool does not exist!!";
        return false;
    }

    if (tool->missing == Trak::ToolMissing)
        return true;
    else
        return false;
}


void DataManager::setToolMissingStatus(int drwnum, int toolnum, int sts)
{

    if (drwnum < Trak::MinDrawerNumber || drwnum > Trak::MaxDrawerNumber)
    {
        qWarning() << "DataManager::setToolMissingStatus() Invalid drawer number: " << drwnum;
        return;
    }

    ToolData* tool = getToolData(drwnum, toolnum);
    if (tool == NULL)
    {
        qWarning() << "DataManager::setToolMissingStatus() tool does not exist!!";
        return;
    }
    tool->missing = sts;
}



//
//  update tool status in database and tool data list
//

void DataManager::updateToolStatus(int drwnum, int toolnum, int sts)
{

    ToolData* tool = getToolData(drwnum, toolnum);
    if (tool->status == sts)
    {
        // status has NOT changed
        return;
    }
    tool->status = sts;
    if (tool->status == Trak::Issued) tool->prevstatus = Trak::Returned; else tool->prevstatus = Trak::Issued;

    QString sql = "update itemkabdrawerbins "
                  "set "
                  "status = %2, "
                  "prevstatus = %3 "
                  "where id = %1 ";
    sql = sql.arg(tool->id).arg(tool->status).arg(tool->prevstatus);

    qDebug() << "DataManager::updateToolStatus() sql: " << sql;

    QSqlQuery query(sql);
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolStatus() Update failed" << query.lastError();
    }
    else
    {
        qInfo() << "Updated database tool status";
        postToPortal(sql);
    }
}



void DataManager::updateToolFromToolData(int drwnum, int toolnum)
{

    ToolData* tool = getToolData(drwnum, toolnum);

    QString sql = "update itemkabdrawerbins "
                  "set "
                  "status = %2, prevstatus = %3, "
                  "missing = %4, transferred = %5, serviceable = %6 "
                  "where id = %1 ";
    sql = sql.arg(tool->id).arg(tool->status).arg(tool->prevstatus)
             .arg(tool->missing).arg(tool->transferred).arg(tool->serviceable);

    qDebug() << "DataManager::updateToolFromToolData() sql: " << sql;

    QSqlQuery query(sql);
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolFromToolData() Update failed" << query.lastError();
    }
    else
    {
        qInfo() << "Updated database tool from ToolData";
        postToPortal(sql);
    }
}



//
//  update tool missing in database and tool data list
//

void DataManager::updateToolMissing(int drwnum, int toolnum, int sts)
{

    ToolData* tool = getToolData(drwnum, toolnum);
    if (tool->missing == sts)
    {
        // missing status has NOT changed
        return;
    }
    tool->missing = sts;

    QString sql = "update itemkabdrawerbins "
                  "set "
                  "missing = %2 "
                  "where id = %1 ";
    sql = sql.arg(tool->id).arg(tool->missing);

    qDebug() << "DataManager::updateToolMissing() sql: " << sql;

    QSqlQuery query(sql);
    query.exec(sql);

    if (query.lastError().isValid())
    {
        qWarning() << "DataManager::updateToolMissing() Update failed" << query.lastError();
    }
    else
    {
        qInfo() << "Updated database tool missing";
        postToPortal(sql);
    }
}



int DataManager::getToolStatus(int drwnum, int toolnum)
{

    if (drwnum < Trak::MinDrawerNumber || drwnum > Trak::MaxDrawerNumber)
    {
        qWarning() << "DataManager::getToolStatus() Invalid drawer number: " << drwnum;
        return -1;
    }

    ToolData* tool = getToolData(drwnum, toolnum);
    if (tool == NULL)
    {
        qWarning() << "DataManager::getToolStatus() tool does not exist!!" << drwnum << toolnum;
        return -1;
    }
    return tool->status;
}


