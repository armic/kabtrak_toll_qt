//#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QObject>


#include "guestscreen.h"
#include "datamanager.h"
#include "maincontroller.h"
#include "selecttail.h"
#include "ui_selecttail.h"

SelectTail::SelectTail(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectTail)
{
    ui->setupUi((QDialog*)parent);

    setupWidget();
}

SelectTail::~SelectTail()
{
    delete ui;
}



//
//  sets the tail widget
//      speed dial buttons
//      combo box of tail numbers
//      adds the button connects
//      inits the work order and batch number entry
//      adds the virtual keyboard
//

void SelectTail::setupWidget()
{
    // setup combo box
    DataManager* backend = MainController::getDataManagerInstance();

    QSqlQueryModel *model = backend->getTailNumbersModel(backend->currentCustomer.custId);
    currentTailModel = model;
    ui->tailComboBox->setModel(model);
    ui->tailComboBox->setModelColumn(1);
    ui->tailComboBox->setEditable(true);
    //ui->tailComboBox->setInsertPolicy(QComboBox::InsertAtTop);
    ui->tailComboBox->completer()->setCompletionMode(QCompleter::QCompleter::PopupCompletion);
    //ui->tailComboBox->completer()->setCompletionMode(QCompleter::QCompleter::InlineCompletion);
    ui->tailComboBox->setStyleSheet("background-color: rgb(240, 240, 240);"
                                    "font-size: 15pt;"
                                    "font-weight: bold;");
    ui->tailComboBox->clearEditText();
    ui->tailComboBox->setFocusPolicy(Qt::StrongFocus);

    connect(ui->tailComboBox, &QComboBox::currentTextChanged, this, &SelectTail::handleCurrentTextChanged);
    connect(ui->tailComboBox, &QComboBox::editTextChanged, this, &SelectTail::handleEditTextChanged);
    connect(ui->tailComboBox, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::highlighted),this, &SelectTail::handleHighlighted);
    connect(ui->tailComboBox, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::activated),this, &SelectTail::handleActivated);


    // setup the speed dial buttons
    // create a push button group which is checkable
    tailButtonGroup = new QButtonGroup(this);

    int rows = model->rowCount();
    int cnt = 0;

    cnt = setSpeedDialButton (model, ui->sd1, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd2, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd3, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd4, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd5, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd6, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd7, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd8, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd9, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd10, tailButtonGroup, rows, cnt++);

    // set button clicked signal
    connect(tailButtonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SelectTail::handleButtonGroupClicked);

    // setup confirm button signal etc
    connect(ui->btnConfirm, &QPushButton::clicked, this, &SelectTail::handleConfirmButton);

    // setup admin button signal etc
    connect(ui->btnAdmin, &QPushButton::clicked, this, &SelectTail::handleAdminButton);

    // setup cancel button signal etc
    connect(ui->btnCancel, &QPushButton::clicked, this, &SelectTail::handleCancelButton);

    ui->btnAdmin->setVisible(false);



    // add virtual keyboard
    keyBlock = new KeyPad(this);
    //kbdLayout = new QGridLayout();
    //kbdLayout->addWidget(keyBlock);
    ui->keyboard->setLayout(new QHBoxLayout);
    ui->keyboard->layout()->addWidget(keyBlock);
    //setLayout(kbdLayout);
    //show();
    //ui->keyboard->layout()->addWidget(keyBlock);

    // setup progress bar
    progressBar = new ProgressBar("SelectTailLogoff", ui->progressBar, this);
    connect(progressBar, &ProgressBar::progressBarTimeout,this, &SelectTail::handleProgressBarEnd);

}




void SelectTail::refreshWidget()
{
    // setup combo box
    DataManager* backend = MainController::getDataManagerInstance();

    QSqlQueryModel *model = backend->getTailNumbersModel(backend->currentCustomer.custId);
    //currentTailModel = model;
    ui->tailComboBox->setModel(model);
    ui->tailComboBox->setModelColumn(1);
    ui->tailComboBox->setEditable(true);
    //ui->tailComboBox->setInsertPolicy(QComboBox::InsertAtTop);
    ui->tailComboBox->completer()->setCompletionMode(QCompleter::QCompleter::PopupCompletion);
    //ui->tailComboBox->completer()->setCompletionMode(QCompleter::QCompleter::InlineCompletion);
    ui->tailComboBox->setStyleSheet("background-color: rgb(240, 240, 240);"
                                    "font-size: 15pt;"
                                    "font-weight: bold;");
    ui->tailComboBox->clearEditText();
    ui->tailComboBox->setFocusPolicy(Qt::StrongFocus);

//    connect(ui->tailComboBox, &QComboBox::currentTextChanged, this, &SelectTail::handleCurrentTextChanged);
//    connect(ui->tailComboBox, &QComboBox::editTextChanged, this, &SelectTail::handleEditTextChanged);
//    connect(ui->tailComboBox, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::highlighted),this, &SelectTail::handleHighlighted);
//    connect(ui->tailComboBox, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::activated),this, &SelectTail::handleActivated);


    // setup the speed dial buttons
    // create a push button group which is checkable
    //tailButtonGroup = new QButtonGroup(this);

    int rows = model->rowCount();
    int cnt = 0;

    cnt = setSpeedDialButton (model, ui->sd1, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd2, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd3, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd4, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd5, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd6, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd7, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd8, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd9, tailButtonGroup, rows, cnt++);
    cnt = setSpeedDialButton (model, ui->sd10, tailButtonGroup, rows, cnt++);

    // set button clicked signal
    //connect(tailButtonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SelectTail::handleButtonGroupClicked);

    // setup confirm button signal etc
    //connect(ui->btnConfirm, &QPushButton::clicked, this, &SelectTail::handleConfirmButton);

    // setup admin button signal etc
    //connect(ui->btnAdmin, &QPushButton::clicked, this, &SelectTail::handleAdminButton);

    // setup cancel button signal etc
    //connect(ui->btnCancel, &QPushButton::clicked, this, &SelectTail::handleCancelButton);

    ui->btnAdmin->setVisible(false);



//    // add virtual keyboard
//    keyBlock = new KeyPad(this);
//    //kbdLayout = new QGridLayout();
//    //kbdLayout->addWidget(keyBlock);
//    ui->keyboard->setLayout(new QHBoxLayout);
//    ui->keyboard->layout()->addWidget(keyBlock);
//    //setLayout(kbdLayout);
//    //show();
//    //ui->keyboard->layout()->addWidget(keyBlock);

//    // setup progress bar
//    progressBar = new ProgressBar("SelectTailLogoff", ui->progressBar, this);
//    connect(progressBar, &ProgressBar::progressBarTimeout,this, &SelectTail::handleProgressBarEnd);

}







//
//  sets the visibility of fields
//

void SelectTail::setWidget()
{

    refreshWidget();

    DataManager* backend = MainController::getDataManagerInstance();

    // set the batch number and work order text boxes
    if (backend->currentCustomer.useWorkOrder == false)
    {
        ui->woLabel->setVisible(false);
        ui->woTextEdit->setVisible(false);
        useWorkOrder = false;
        ui->lblTitle->setText("TAIL NUMBER ENTRY");
        workOrderNumeric = false;
        workOrderMax = 0;
        workOrderMin = 0;
    }
    else
    {
        ui->woLabel->setVisible(true);
        ui->woTextEdit->setVisible(true);
        ui->woLabel->setText(MainController::getDataManagerInstance()->currentCustomer.workOrderLabel);
        workOrderNumeric = MainController::getDataManagerInstance()->currentCustomer.workOrderNumeric;
        workOrderMax = MainController::getDataManagerInstance()->currentCustomer.workOrderMax;
        workOrderMin = MainController::getDataManagerInstance()->currentCustomer.workOrderMin;
        ui->woTextEdit->clear();
//        ui->woTextEdit->setTabChangesFocus(true);
        useWorkOrder = true;
        ui->lblTitle->setText("TAIL NUMBER ENTRY");

        // do we need to create a validator?
        QRegularExpression rex (workOrderInputValidator());
        QValidator* validator = new QRegularExpressionValidator(rex, this);
        workOrderValidator = validator;
        ui->woTextEdit->setValidator(validator);
    }
    if (backend->currentCustomer.useBatch == false)
    {
        ui->bnLabel->setVisible(false);
        ui->bnTextEdit->setVisible(false);
        useBatch = false;
        batchNumeric = false;
        batchMax = 0;
        batchMin = 0;
    }
    else
    {
        ui->bnLabel->setVisible(true);
        ui->bnTextEdit->setVisible(true);
        ui->bnLabel->setText(MainController::getDataManagerInstance()->currentCustomer.batchLabel);
        batchNumeric = MainController::getDataManagerInstance()->currentCustomer.batchNumeric;
        batchMax = MainController::getDataManagerInstance()->currentCustomer.batchMax;
        batchMin = MainController::getDataManagerInstance()->currentCustomer.batchMin;
        ui->woTextEdit->clear();
        ui->bnTextEdit->clear();
//        ui->bnTextEdit->setTabChangesFocus(true);
        useBatch = true;

        // do we need to create a validator?
        QRegularExpression rex (batchInputValidator());
        QValidator* validator = new QRegularExpressionValidator(rex, this);
        batchValidator = validator;
        ui->bnTextEdit->setValidator(validator);
    }
    clearErrorMessage();

    // set progress bar
    progressBar->start();

    // if not admin user do not display admin button
    if (backend->currentUser.adminPriv == 1)
    {
        ui->btnAdmin->setVisible(true);
    }
    else
    {
        ui->btnAdmin->setVisible(false);
    }

    QAbstractButton* checked = tailButtonGroup->checkedButton();
    if (checked)
    {
        tailButtonGroup->setExclusive(false);
        checked->setChecked(false);
        tailButtonGroup->setExclusive(true);
    }

    ui->tailComboBox->clearEditText();
    tailEntrySelected = noneSelected;
}


//
//  sets a speeddial button with the tail description
//  and initialises the button
//

int SelectTail::setSpeedDialButton (QSqlQueryModel* model, QPushButton* button, QButtonGroup* group, int rows, int cnt)
{

    // find the next speeddial row
    int row = cnt;
    for (; row < rows; row++)
    {
        int id = model->record(row).value("speedDial").toInt();
        if (id != 0)
        {
            // found next one
            break;
        }
    }
    // setup the speed dial button
    if (row < rows)
    {
        QString name = model->record(row).value("description").toString() + "\n" +  model->record(row).value("remark").toString();
        int id = model->record(row).value("trailId").toInt();

        button->setStyleSheet("background-color: rgb(240, 240, 240);");
//        button->setStyleSheet("background-color: rgb(255, 170, 0);");
        button->setVisible(true);
        button->setText((name));
        // add to group and set checkable
        group->addButton(button, id);
        button->setCheckable(true);

        qDebug() << "SelectTail::setSpeedDialButton setting " << name;
    }
    else
    {
        button->setVisible(false);
        qDebug() << "SelectTail::setSpeedDialButton setting to invisible ";
    }
    return row;
}



//
//  Handles the confirm button
//      Checks the tail has been entered through speed dial or combo box
//      Checks for work order and batch number if required
//      sets current user detailes
//      sets up the user screen
//

void SelectTail::handleConfirmButton()
{
    QString tailtext = "";
    int tailid = 0;
    qDebug() << "SelectTail::handleConfirmButton";

    // check tail selected
    int id = tailButtonGroup->checkedId();
    int cboxid = ui->tailComboBox->currentIndex();
    qDebug() << "SelectTail::handleConfirmButton id " << id << cboxid;

    if (tailEntrySelected == noneSelected)
    {
        displayErrorMessage("Please enter tail number");
        qDebug() << "SelectTail::handleConfirmButton() none selected. Please enter tail number";
        return;
    }
    else if (tailEntrySelected == speedDialSelected)
    {
        if (id >=0)
        {
            tailtext = tailButtonGroup->button(id)->text();
            tailid = id;
            qDebug() << "SelectTail::handleConfirmButton group tail text " << tailtext;
        }
        else
        {
            qDebug() << "SelectTail::handleConfirmButton group tail text problem " << id;
            displayErrorMessage("Tail number not correct");
            return;

        }
    }
    else if (tailEntrySelected == comboSelected)
    {
        tailtext = ui->tailComboBox->currentText();
        tailid = ui->tailComboBox->findText(tailtext);
        if (tailid == -1)
        {
            qDebug() << "SelectTail::handleConfirmButton combo tail text not correct " << tailtext << tailid;
            displayErrorMessage("Tail number not correct");
            return;
        }
        qDebug() << "SelectTail::handleConfirmButton combo tail text " << tailtext;
    }
    else // none selected
    {
        displayErrorMessage("Please enter tail number");
        qDebug() << "SelectTail::handleConfirmButton() none selected. Please enter tail number";
        return;
    }
    if (tailtext.isEmpty())
    {
        displayErrorMessage("Please enter tail number");
        qDebug() << "SelectTail::handleConfirmButton() empty tailtext, Please enter tail number";
        return;
    }



    if (useWorkOrder)
    {
        // get entered number
        currentWorkOrder = ui->woTextEdit->text();
        int pos = 0;
        if (currentWorkOrder.isNull() || (currentWorkOrder.trimmed().isEmpty() && workOrderMin != 0))
        {
            displayErrorMessage("Please enter work order");
            qWarning() << "SelectTail::handleConfirmButton Please enter work order";
            return;

        }
        else if (workOrderValidator->validate(currentWorkOrder, pos) != QValidator::Acceptable){
            displayErrorMessage("Incorrect format for work order");
            qWarning() << "SelectTail::handleConfirmButton Incorrect format for work order";
            return;
        }
        else
        {
            qInfo() << "SelectTail::handleConfirmButton work order number OK" << currentWorkOrder;
        }
    }

    if (useBatch)
    {
        // get entered number
        currentBatch = ui->bnTextEdit->text();
        int pos = 0;
        if (currentBatch.isNull() || (currentBatch.trimmed().isEmpty() && batchMin != 0))
        {
            displayErrorMessage("Please enter batch number");
            qWarning() << "SelectTail::handleConfirmButton Please enter batch number";
            return;

        }
        else if (batchValidator->validate(currentBatch, pos) != QValidator::Acceptable){
            displayErrorMessage("Incorrect format for batch number");
            qWarning() << "SelectTail::handleConfirmButton Incorrect format for batch number";
            return;
        }
        else
        {
            qInfo() << "SelectTail::handleConfirmButton batch number OK" << currentBatch;
        }
    }

    // all fields entered so save data and move on
    DataManager* backend = MainController::getDataManagerInstance();
    backend->currentUser.tailId = tailid;
    backend->currentUser.tailDesc = tailtext;
    qInfo() << "Tail number " << tailtext;
    if (useWorkOrder)
    {
        backend->currentUser.currentWorkOrder = currentWorkOrder;
        qInfo() << "Work order " << currentWorkOrder;
    }
    if (useBatch)
    {
        backend->currentUser.currentBatch = currentBatch;
        qInfo() << "Batch number " << currentBatch;
    }

    progressBar->stop();
    MainController::getMainViewInstance()->setUserScreen();

}




//
//  handles the admin button
//      Checks the user has admin rights
//      Updates the current user.
//

void SelectTail::handleAdminButton()
{
    DataManager* backend = MainController::getDataManagerInstance();
    if (backend->currentUser.adminPriv == 1)
    {
        qInfo() << "SelectTail::handleAdminButton() User entering admin" << backend->currentUser.userId << backend->currentUser.name;
        MainController::getMainViewInstance()->setAdminScreen();
        progressBar->stop();
    }
    else
    {
        qInfo() << "SelectTail::handleAdminButton() User disallowed entry to admin" << backend->currentUser.userId << backend->currentUser.name;
    }
}


//
//  handle cancel button
//

void SelectTail::handleCancelButton()
{
    qDebug() << "SelectTail::handleCancelButton";
    progressBar->stop();
    MainController::getMainViewInstance()->backToGuestMode();

}




//
//  display error message in error field
//

void SelectTail::displayErrorMessage(QString str)
{
    ui->errorLabel->setStyleSheet("background-color:rgb(230,0,0);"
                                         "font-size: 15pt;"
                                         "font-weight: bold;");
    ui->errorLabel->setText(str);
}


void SelectTail::clearErrorMessage()
{
    ui->errorLabel->setStyleSheet("background-color:rgb(0,0,0);"
                                         "font-size: 15pt;"
                                         "font-weight: bold;");
    ui->errorLabel->setText("");
}





void SelectTail::handleProgressBarEnd()
{
    qDebug() << "SelectTail::handleProgressBarEnd";
    MainController::getMainViewInstance()->backToGuestMode();

}



//
//  combo box selected
//
void SelectTail::handleCurrentTextChanged(const QString & text)
{
    qDebug() << "SelectTail::handleCurrentTextChanged() " << text;

    QAbstractButton* checked = tailButtonGroup->checkedButton();
    if (checked)
    {
        tailButtonGroup->setExclusive(false);
        checked->setChecked(false);
        tailButtonGroup->setExclusive(true);
    }
    tailEntrySelected = comboSelected;
}

void SelectTail::handleEditTextChanged(const QString & text)
{
    qDebug() << "SelectTail::handleEditTextChanged() " << text;
}

void SelectTail::handleHighlighted(const QString & text)
{
    qDebug() << "SelectTail::handleHighlighted() " << text;
}

void SelectTail::handleActivated(const QString & text)
{
    qDebug() << "SelectTail::handleActivated() " << text;
}

//
//  speed dial button clicked
//
void SelectTail::handleButtonGroupClicked(int id)
{
    qDebug() << "SelectTail::handleButtonGroupClicked() " << id;
    ui->tailComboBox->clearEditText();
    QAbstractButton* checked = tailButtonGroup->button(id);
    if (checked)
    {
        //tailButtonGroup->setExclusive(false);
        checked->setChecked(true);
        //tailButtonGroup->setExclusive(true);
        qDebug() << "SelectTail::handleButtonGroupClicked() set checked";
    }
    tailEntrySelected = speedDialSelected;

    if (!useWorkOrder && !useBatch)
    {
        handleConfirmButton();
    }

}



//
//  Input validator regex for work order
//
QString SelectTail::workOrderInputValidator()
{
    QString rex = "";
//    bool num = MainController::getDataManagerInstance()->currentCustomer.workOrderNumeric;
//    int max = MainController::getDataManagerInstance()->currentCustomer.workOrderMax;
//    int min = MainController::getDataManagerInstance()->currentCustomer.workOrderMin;
    if (workOrderNumeric)
    {
        if (workOrderMax > 0)
        {
            rex = "\\d{%1,%2}";
            rex = rex.arg(workOrderMin).arg(workOrderMax);

        }
        else
        {
            rex = "\\d+";
        }
    }
    else
    {
        // ascii numeric
        if (workOrderMax > 0)
        {
            rex = "\\S{%1,%2}";
            rex = rex.arg(workOrderMin).arg(workOrderMax);

        }
        else
        {
            rex = "\\S+";
        }
    }
    qDebug() << "SelectTail::workOrderInputValidator() " << rex;
    return rex;
}




//
//  Input validator regex for batch
//
QString SelectTail::batchInputValidator()
{
    QString rex = "";
//    bool num = MainController::getDataManagerInstance()->currentCustomer.workOrderNumeric;
//    int max = MainController::getDataManagerInstance()->currentCustomer.workOrderMax;
//    int min = MainController::getDataManagerInstance()->currentCustomer.workOrderMin;
    if (batchNumeric)
    {
        if (batchMax > 0)
        {
            rex = "\\d{%1,%2}";
            rex = rex.arg(batchMin).arg(batchMax);

        }
        else
        {
            rex = "\\d+";
        }
    }
    else
    {
        // ascii numeric
        if (batchMax > 0)
        {
            rex = "\\S{%1,%2}";
            rex = rex.arg(batchMin).arg(batchMax);

        }
        else
        {
            rex = "\\S+";
        }
    }
    qDebug() << "SelectTail::batchMaxInputValidator() " << rex;
    return rex;
}



