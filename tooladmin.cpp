#include "tooladmin.h"
#include "ui_tooladmin.h"
#include "maincontroller.h"
#include "datamanager.h"
#include "tooldetail.h"

tooladmin::tooladmin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::tooladmin)
{
    ui->setupUi(this);
    populateTooldataItem();
    populateCabTooldataItem();
}

tooladmin::~tooladmin()
{
    delete ui;
}

void tooladmin::populateTooldataItem()
{
    DataManager* dataman = MainController::getDataManagerInstance();
    toollistmodel = dataman->getToolInventoryList();
    toollistmodel->setHeaderData(1,Qt::Horizontal, tr("Part #"));
    toollistmodel->setHeaderData(4,Qt::Horizontal, tr("Stock Code"));
    toollistmodel->setHeaderData(2,Qt::Horizontal, tr("Description"));
    toollistmodel->setHeaderData(5,Qt::Horizontal, tr("Image"));
    toollistmodel->setHeaderData(7,Qt::Horizontal, tr("Serial Number"));
    toollistmodel->setHeaderData(8,Qt::Horizontal, tr("is a Kit"));
    toollistmodel->setHeaderData(9,Qt::Horizontal, tr("Kit Count"));

  //  ui->tableView->setColumnHidden(3,true);
    ui->tableViewAllTools->setModel(toollistmodel);
    ui->tableViewAllTools->setAlternatingRowColors(true);
    ui->tableViewAllTools->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewAllTools->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewAllTools->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewAllTools->setColumnWidth(2,350);
    ui->tableViewAllTools->setColumnWidth(8,50);
    ui->tableViewAllTools->setColumnWidth(9,50);
    ui->tableViewAllTools->hideColumn(0);
    ui->tableViewAllTools->hideColumn(3);
    ui->tableViewAllTools->hideColumn(6);

}

void tooladmin::populateCabTooldataItem()
{
    DataManager* dataman = MainController::getDataManagerInstance();
    cabtoollistmodel = dataman->getCabinetToolList();
    cabtoollistmodel->setHeaderData(1,Qt::Horizontal, tr("Drawer"));
    cabtoollistmodel->setHeaderData(0,Qt::Horizontal, tr("Tool"));
    cabtoollistmodel->setHeaderData(6,Qt::Horizontal, tr("Item ID"));
    cabtoollistmodel->setHeaderData(5,Qt::Horizontal, tr("Description"));
    cabtoollistmodel->setHeaderData(2,Qt::Horizontal, tr("Status"));


  //  ui->tableView->setColumnHidden(3,true);
    ui->tableViewCabTools->setModel(cabtoollistmodel);
    ui->tableViewCabTools->setAlternatingRowColors(true);
    ui->tableViewCabTools->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewCabTools->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewCabTools->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewCabTools->setColumnWidth(5,500);
    ui->tableViewCabTools->setColumnWidth(0,50);
    ui->tableViewCabTools->setColumnWidth(1,50);
    ui->tableViewCabTools->hideColumn(3);
    ui->tableViewCabTools->hideColumn(4);

}



void tooladmin::on_btnNew_clicked()
{
    // New button
    toolDetail frmToolDetail;
    frmToolDetail.setStockCode("");
    int exitCode = frmToolDetail.exec();
    qDebug() << "Tool Detail Exec Code" << exitCode;
    populateTooldataItem();
}

void tooladmin::on_btnEdit_clicked()
{

    // Edit
    int i;
    QString sStockCode;
    QModelIndex index = ui->tableViewAllTools->currentIndex();
    i = index.row();
    if(i == -1)
    {
       //No selected row

    }
  else
     {
     qDebug()<< "Tool Index "<<i;
//     sStockCode = ui->tableViewAllTools->model()->data(ui->tableViewAllTools->model()->index(i,1)).toString();
     sStockCode = ui->tableViewAllTools->model()->data(ui->tableViewAllTools->model()->index(i,4)).toString();
     toolDetail frmToolDetail;
     frmToolDetail.setStockCode(sStockCode);
     int exitCode = frmToolDetail.exec();
     qDebug() << "Tool Detail Exec Code" << exitCode;
     populateTooldataItem();

  }
}

void tooladmin::on_btnDelete_clicked()
{
    // Delete
    DataManager* dataman = MainController::getDataManagerInstance();
    int i;
    QString sPartNumber;
    QModelIndex index = ui->tableViewAllTools->currentIndex();
    i = index.row();
    if(i == -1)
    {
     // No index selected
        QMessageBox msgBox;
        msgBox.setText("Please select a tool to delete.");
        msgBox.exec();

    }
   else
    {
        sPartNumber = ui->tableViewAllTools->model()->data(ui->tableViewAllTools->model()->index(i,0)).toString();
        if(dataman->isCabToolExists(sPartNumber))
        {
            // Cannot delete
            QMessageBox msgBox;
            msgBox.setText("Current tool is linked to one of the cabinet tool and cannot be deleted.");
            msgBox.exec();
        }
        else
        {
           // Delete too
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Please Confirm", "Do you really want to delete current tool?",
                                              QMessageBox::Yes|QMessageBox::No);
             // Grant access
            if (reply == QMessageBox::Yes) {
                qDebug() << "Yes was clicked";
                dataman->deleteToolRecord(sPartNumber);
                populateTooldataItem();
            }

        }

    }
}

void tooladmin::on_btnImport_clicked()
{
    //Import
}

void tooladmin::on_tabList_currentChanged(int index)
{
    // Tab Changed
    qDebug() << "Current Tab Index " << index;
    switch (index) {
    case 0:
        ui->btnNew->setEnabled(true);
        ui->btnEdit->setEnabled(true);
        ui->btnDelete->setEnabled(true);
        ui->btnImport->setEnabled(true);
        break;

    case 1:
        ui->btnNew->setEnabled(false);
        ui->btnEdit->setEnabled(false);
        ui->btnDelete->setEnabled(false);
        ui->btnImport->setEnabled(false);

        break;

    default:
        break;
    }
}
