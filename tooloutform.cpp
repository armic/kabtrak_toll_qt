#include "tooloutform.h"
#include "ui_tooloutform.h"
#include "maincontroller.h"

toolOutForm::toolOutForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::toolOutForm)
{
    ui->setupUi(this);
    QSqlQueryModel* model = MainController::getDataManagerInstance()->toolsOutModel;

    model->setHeaderData(0, Qt::Horizontal, tr(" Out Date "));
    model->setHeaderData(1, Qt::Horizontal, tr(" Out Time "));
    model->setHeaderData(2, Qt::Horizontal, tr(" User Id "));
    model->setHeaderData(3, Qt::Horizontal, tr(" Issued to "));
    model->setHeaderData(4, Qt::Horizontal, tr(" Tool Name "));
    model->setHeaderData(5, Qt::Horizontal, tr(" Drawer# "));
    model->setHeaderData(6, Qt::Horizontal, tr(" Tool# "));
    model->setHeaderData(7, Qt::Horizontal, tr(" Tail No. "));

    // sorting columns
    QSortFilterProxyModel *sort_filter = new QSortFilterProxyModel(this);
    sort_filter->setSourceModel(model);
    sort_filter->sort (2);
     ui->tvToolOutList->setModel(sort_filter);

    ui->tvToolOutList->resizeRowsToContents();
    ui->tvToolOutList->setAlternatingRowColors(true);

    ui->tvToolOutList->hideColumn(2);
    ui->tvToolOutList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tvToolOutList->setSortingEnabled(true);

    // calculate total width of columns
//    int cnt = model->columnCount();
//    int totalwidth = 0;
//    for (int i = 0; i < cnt; i++)
//    {
//        totalwidth += ui->tvToolOutList->columnWidth(i);
//    }
//    totalwidth += ui->tvToolOutList->verticalHeader()->width();
//    totalwidth += 2;
//    ui->tvToolOutList->resize(totalwidth,400);

    // set timeout for this window
    // Get timer values from ini file
    QSettings settings("trak.ini", QSettings::IniFormat);
    settings.beginGroup("System");
    int tmr = settings.value("IssuedToolsTimeout", 30).toInt();
    settings.endGroup();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this , &toolOutForm::on_brnClose_clicked);
    timer->start(tmr * 1000);

}

toolOutForm::~toolOutForm()
{
    delete ui;
}

void toolOutForm::on_brnClose_clicked()
{
    close();
}


