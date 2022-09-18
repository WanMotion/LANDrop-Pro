#include <QMessageBox>
#include "addhostsmanuldialog.h"
#include "ui_addhostsmanuldialog.h"

AddHostsManulDialog::AddHostsManulDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddHostsManulDialog)
{
    ui->setupUi(this);
}

AddHostsManulDialog::~AddHostsManulDialog()
{
    delete ui;
}

void AddHostsManulDialog::accept()
{
    if(ui->deviceNameLineEdit->text().length()==0){
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Invalid Device Name"));
        return;
    }
    QString deviceName=ui->deviceNameLineEdit->text();
    QString host=ui->hostLineEdit->text();
    if(host.length()==0){
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Invalid Host"));
        return;
    }
    bool ok;
    quint16 port=ui->portLineEdit->text().toUShort(&ok);
    if(!ok){
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Invalid port. Please enter a number between 1 and 65535."));
        return;
    }
    emit addHostManully(deviceName,host,port);
    this->setVisible(false);
}
