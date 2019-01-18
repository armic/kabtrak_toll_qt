#include "taildetail.h"
#include "ui_taildetail.h"
#include "maincontroller.h"
#include "datamanager.h"
#include "taillist.h"

taildetail::taildetail(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::taildetail)
{
    ui->setupUi(this);
}

taildetail::~taildetail()
{
    delete ui;
}

void taildetail::setTailNumber(const QString &tailNumber)
{

  m_tailNumber = tailNumber;
qDebug()<<"Tail Number "<<m_tailNumber;

    keybrd = new KeyPad(this);
    ui->keybrd->setLayout(new QHBoxLayout);
    ui->keybrd->layout()->addWidget(keybrd);


if(!m_tailNumber.isEmpty())
 {
    // Edit Mode
    DataManager* dataman = MainController::getDataManagerInstance();
    QSqlQuery tailQuery = dataman->getSelectedTail(m_tailNumber);
    taildetail::setWindowTitle("Editing Tail  " + tailQuery.value(1).toString());
    //Transfer data to form
    ui->edtTailNumber->setReadOnly(true);
    ui->edtTailNumber->setText(tailQuery.value(1).toString());
    ui->edtDescription->setText(tailQuery.value(3).toString());

    int iSpeedDial = tailQuery.value(4).toInt();
    int iDisabled = tailQuery.value(5).toInt();

    if(iSpeedDial == 1){

     ui->cbSpeedDial->setChecked(true);
    }
    else
    {
        ui->cbSpeedDial->setChecked(false);
    }

    if(iDisabled == 1){

     ui->cbDisabled->setChecked(true);
    }
    else
    {
        ui->cbDisabled->setChecked(false);
    }




 }
else
  {
    // Insert Mode
     ui->edtTailNumber->setReadOnly(false);

  }
}

void taildetail::on_buttonBox_accepted()
{
    //Ok button
    DataManager* dataman = MainController::getDataManagerInstance();
    int iSpeedDial;
    int iDisabled;
    if(!m_tailNumber.isEmpty())
     {
      // edit mode
      if(ui->cbSpeedDial->isChecked()){
         iSpeedDial = 1;
      }
     else
      {
          iSpeedDial = 0;
      }

      if(ui->cbDisabled->isChecked()){
         iDisabled = 1;
      }
     else
      {
          iDisabled = 0;
      }


      if(dataman->updateTailRecord(ui->edtTailNumber->text(),ui->edtDescription->text(),iSpeedDial, iDisabled, dataman->currentCustomer.custId))
      {
          QMessageBox msgBox;
          msgBox.setText("Current tail number details successfullly updated.");
          msgBox.exec();\

          close();
      }
     else
      {
          QMessageBox msgBox;
          msgBox.setText("Unable to update current record.");
          msgBox.exec();
      }


    }
    else
     {
      // insert mode
      bool tailEXist = dataman->isTailExists(ui->edtTailNumber->text());

      if(tailEXist)
      {
          qDebug() << "Insert Tail Duplicate record";
          QMessageBox msgBox;
          msgBox.setText("Duplicate Tail Number. Please enter a unique value.");
          msgBox.exec();
          // Duplicate error message here

      }
      else
      {
          // yes allow to insert

          int isSpeedDial = ui->cbSpeedDial->isChecked();
          int isDisabled = ui->cbDisabled->isChecked();

          dataman->insertTailRecord(ui->edtTailNumber->text(), ui->edtDescription->text(), isSpeedDial,isDisabled, dataman->currentCustomer.custId);

          QMessageBox msgBox;
          msgBox.setText("New Tail Number " + ui->edtTailNumber->text() + " created. ");
          msgBox.exec();

          close();
      }

     }


}

void taildetail::on_buttonBox_rejected()
{
    // Cancel

    close();
}

void taildetail::on_edtTailNumber_textChanged(const QString &arg1)
{
     ui->edtTailNumber->setText(arg1.toUpper());
}

void taildetail::on_edtDescription_textChanged(const QString &arg1)
{
     ui->edtDescription->setText(arg1.toUpper());
}
