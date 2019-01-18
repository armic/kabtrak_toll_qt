#ifndef TOOLADMIN_H
#define TOOLADMIN_H

#include <QDialog>
#include <QtSql>

namespace Ui {
class tooladmin;
}

class tooladmin : public QDialog
{
    Q_OBJECT

public:
    explicit tooladmin(QWidget *parent = 0);
    ~tooladmin();
    void populateTooldataItem();
    void populateCabTooldataItem();

private slots:
    void on_btnNew_clicked();

    void on_btnEdit_clicked();

    void on_btnDelete_clicked();

    void on_btnImport_clicked();

    void on_tabList_currentChanged(int index);

private:
    Ui::tooladmin *ui;
    QSqlQueryModel* toollistmodel;
    QSqlQueryModel* cabtoollistmodel;
};

#endif // TOOLADMIN_H
