#include "systemsettings.h"
#include "ui_systemsettings.h"
#include "maincontroller.h"
#include "cabinet.h"
#include "datamanager.h"
#include "logviewer.h"



systemsettings::systemsettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::systemsettings)
{
    ui->setupUi(this);

    // load values

    DataManager* dataman = MainController::getDataManagerInstance();
    ui->edtCloseDrawer->setValue(dataman->currentCabinet.closeDrawerTimeOut);
    ui->edtAdmin->setValue(dataman->currentCabinet.adminTimeOut);
    ui->edtOpenAlarm->setValue(dataman->currentCabinet.openDrawerTimeout);
    ui->edtIssuedTools->setValue(dataman->currentCabinet.issuedToolsTimeout);
    ui->edtwebUpdateURL->setText(dataman->currentCabinet.url);
    ui->edtUpdateDir->setText(dataman->currentCabinet.updateDir);

   // Toggled options
    ui->cbAlertSound->setChecked(dataman->currentCabinet.allowSounds);

    keybrd = new KeyPad(this);
    ui->keybrd->setLayout(new QHBoxLayout);
    ui->keybrd->layout()->addWidget(keybrd);

}

systemsettings::~systemsettings()
{
    delete ui;
}


void systemsettings::on_btnOK_clicked()
{
    // Accept Changes

    DataManager* dataman = MainController::getDataManagerInstance();

     QSettings settings("trak.ini", QSettings::IniFormat);
     settings.beginGroup("Cabinet");
//     settings.setValue("DefaultCalibrationAccount",dataman->currentCabinet.defaultCalibrationAccount);
//     settings.setValue("DefaultServiceAccount",dataman->currentCabinet.defaultServiceAccount);
//     settings.setValue("DefaultTransferAccount",dataman->currentCabinet.defaultTransferAccount);
//     settings.setValue("DefaultTestAccount",dataman->currentCabinet.defaultTestAccount);
     settings.endGroup();

     dataman->currentCabinet.closeDrawerTimeOut = ui->edtCloseDrawer->text().toInt();
     dataman->currentCabinet.openDrawerTimeout = ui->edtOpenAlarm->text().toInt();
     dataman->currentCabinet.adminTimeOut = ui->edtAdmin->text().toInt();
     dataman->currentCabinet.issuedToolsTimeout = ui->edtIssuedTools->text().toInt();
     dataman->currentCabinet.url = ui->edtwebUpdateURL->text();
     dataman->currentCabinet.updateDir = ui->edtUpdateDir->text();

     dataman->currentCabinet.allowSounds = ui->cbAlertSound->isChecked();

     //Timers
     settings.beginGroup("System");
     settings.setValue("CloseDrawerToLogoff",dataman->currentCabinet.closeDrawerTimeOut);
     settings.setValue("OpenDrawerToAlarm",dataman->currentCabinet.openDrawerTimeout);
     settings.setValue("AdminToLogOff",dataman->currentCabinet.adminTimeOut);
     settings.setValue("IssuedToolsTime",dataman->currentCabinet.issuedToolsTimeout);
     settings.setValue("URL",dataman->currentCabinet.url);
     settings.setValue("UpdateDir",dataman->currentCabinet.updateDir);
     settings.endGroup();


     // Toggled options
     settings.beginGroup("Cabinet");
     settings.setValue("allowSounds",dataman->currentCabinet.allowSounds == true ? 1 : 0);
    // settings.setValue("AllowReturnByOthers",dataman->currentCabinet.allowReturnByOthers);

     settings.endGroup();

    close();

}

//void systemsettings::on_btnbackup_clicked()
//{

//}

void systemsettings::on_btnterminate_clicked()
{
    // Terminate kabtrak
    qInfo() << "Kabtrak terminated from system";
     MainController::getMainControllerInstance()->requestExit(Trak::AdminRequestExit);

}

void systemsettings::on_btnRestart_clicked()
{
    // Restart kabtrak
    qInfo() << "Kabtrak terminated and restarted from system";
    QProcess::startDetached(QApplication::arguments()[0], QApplication::arguments());
    MainController::getMainControllerInstance()->requestExit(Trak::AdminRequestExit);
}

void systemsettings::on_btnLogOff_clicked()
{
    //Logoff to windows
     QApplication::quit();
     QProcess proc;
     proc.start("shutdown", QStringList() << "/l");
//     if (!proc.waitForStarted()) {
//         return 1;
//     }
}

void systemsettings::on_btnCheck_clicked()
{
    // Web Update
}

void systemsettings::on_btnLocalUpdate_clicked()
{
    //Local Update
}

void systemsettings::on_btnCancel_clicked()
{
    // Cancel button
    // No changes were made
    close();
}

void systemsettings::on_btnClear_clicked()
{
    //Clear transactions
     DataManager* dataman = MainController::getDataManagerInstance();
     QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "kabTRAK Transactions", "Would you really want to clear transaction record?",
                                    QMessageBox::Yes|QMessageBox::No);
      if (reply == QMessageBox::Yes) {
        qDebug() << "Yes was clicked";
       dataman->clearTransactions();
      } else {
        qDebug() << "Yes was *not* clicked";
      }


}

void systemsettings::on_btnViewLogs_clicked()
{
    // View kabtrak logs file
    // Load Employee List window
    logViewer frmLogViewer;
    frmLogViewer.setModal(true);
    frmLogViewer.exec();

}

void systemsettings::on_btnViewLogin_clicked()
{
    // view Reader log
}

void systemsettings::on_btnDeleteLogs_clicked()
{
    //Delete all logs
}

void systemsettings::on_cbAlertSound_clicked(bool checked)
{
    // Use alert sound
    DataManager* dataman = MainController::getDataManagerInstance();
    if(checked) {
      dataman->currentCabinet.allowSounds = true;
    }
   else
     {
        dataman->currentCabinet.allowSounds = false;
    }




}

void systemsettings::on_edtCloseDrawer_valueChanged(const QString &arg1)
{
    // Close drawer value
     DataManager* dataman = MainController::getDataManagerInstance();
     dataman->currentCabinet.closeDrawerTimeOut = arg1.toInt();
}

void systemsettings::on_edtAdmin_valueChanged(const QString &arg1)
{
    //Admin time out value
    DataManager* dataman = MainController::getDataManagerInstance();
    dataman->currentCabinet.adminTimeOut = arg1.toInt();
}

void systemsettings::on_edtOpenAlarm_valueChanged(const QString &arg1)
{
    //Open Drawer timer value
    DataManager* dataman = MainController::getDataManagerInstance();
    dataman->currentCabinet.openDrawerTimeout = arg1.toInt();
}

void systemsettings::on_edtIssuedTools_valueChanged(const QString &arg1)
{
    // Issued tools value
    DataManager* dataman = MainController::getDataManagerInstance();
    dataman->currentCabinet.issuedToolsTimeout = arg1.toInt();
}
