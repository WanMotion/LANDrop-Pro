/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, LANDrop
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QMessageBox>
#include <QPushButton>
#include <QJsonDocument>

#include "filetransferdialog.h"
#include "filetransfersender.h"
#include "sendtodialog.h"
#include "ui_sendtodialog.h"

#define HOST_MANUL_FILE "hosts.json"

SendToDialog::SendToDialog(QWidget *parent, const QList<QSharedPointer<QFile>> &files,
                           DiscoveryService &discoveryService) :
    QDialog(parent), ui(new Ui::SendToDialog), files(files)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    ui->hostsListView->setModel(&hostsStringListModel);
    ui->hostsListViewManul->setModel(&hostsManulStringListModel);
    connect(ui->hostsListView, &QListView::clicked, this, &SendToDialog::hostsListViewClicked);
    connect(ui->hostsListViewManul,&QListView::clicked,this,&SendToDialog::hostsListViewManulClicked);
    connect(ui->hostsListView, &QListView::doubleClicked, ui->buttonBox, &QDialogButtonBox::accepted);
    connect(ui->hostsListViewManul,&QListView::doubleClicked,ui->buttonBox,&QDialogButtonBox::accepted);

    // Add Button
    ui->pushButton->setText(tr("Add Manully"));
    connect(ui->pushButton,&QPushButton::clicked,this,&SendToDialog::showAddHostsManulDialog);

    // AddHostDialog
    addHostDialog=new AddHostsManulDialog(this);
    addHostDialog->setVisible(false);
    connect(addHostDialog,&AddHostsManulDialog::addHostManully,this,&SendToDialog::addHostManul);

    // delete Button
    ui->deleteButton->setText(tr("Delete"));
    ui->deleteButton->setEnabled(false);
    connect(ui->deleteButton,&QPushButton::clicked,this,&SendToDialog::onDeleteButtonClicked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Send"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    connect(&discoveryService, &DiscoveryService::newHost, this, &SendToDialog::newHost);
    connect(&discoveryTimer, &QTimer::timeout, &discoveryService, &DiscoveryService::refresh);
    discoveryTimer.start(1000);
    discoveryService.refresh();

    connect(&socketTimeoutTimer, &QTimer::timeout, this, &SendToDialog::socketTimeout);
    socketTimeoutTimer.setSingleShot(true);

    // read hosts file
    QFile file(HOST_MANUL_FILE);
    if(file.exists()&&!file.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Error! When open hosts.json!"));
        return;
    }
    if(!file.exists()){
        return;
    }
    QByteArray data(file.readAll());
    file.close();
    QJsonParseError jsonErr;
    QJsonDocument jsonDoc=QJsonDocument::fromJson(data,&jsonErr);
    if(jsonErr.error!=QJsonParseError::NoError){
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Parse json Failed!"));
        return;
    }
    jsonObj=jsonDoc.object();
    QStringList l=hostsManulStringListModel.stringList();
    for(QJsonObject::iterator iter=jsonObj.begin();iter!=jsonObj.end();iter++){
        l.append(iter.key());
        QString addr=iter.value().toObject().value("host").toString();
        quint16 port=iter.value().toObject().value("port").toInt();
        endpointsManul.push_back({addr,port});
    }
    hostsManulStringListModel.setStringList(l);
}

SendToDialog::~SendToDialog()
{
    delete addHostDialog;
    delete ui;
}

void SendToDialog::newHost(const QString &deviceName, const QHostAddress &addr, quint16 port)
{
    QStringList l = hostsStringListModel.stringList();
    if (port == 0) {
        for (int i = 0; i < endpoints.size(); ++i) {
            if (endpoints[i].addr==addr.toString()) {
                endpoints.removeAt(i);
                l.removeAt(i);
                hostsStringListModel.setStringList(l);
                return;
            }
        }
        return;
    }
    for (int i = 0; i < endpoints.size(); ++i) {
        if (endpoints[i].addr==addr.toString()) {
            if (l.at(i) != deviceName) {
                l.replace(i, deviceName);
                hostsStringListModel.setStringList(l);
            }
            endpoints[i].port = port;
            return;
        }
    }
    endpoints.append({addr.toString(), port});
    l.append(deviceName);
    hostsStringListModel.setStringList(l);
}

void SendToDialog::hostsListViewClicked(const QModelIndex &index)
{
    Endpoint endpoint = endpoints[index.row()];
    ui->addrLineEdit->setText(endpoints[index.row()].addr);
    ui->portLineEdit->setText(QString::number(endpoint.port));
}

void SendToDialog::accept()
{
    QString addr = ui->addrLineEdit->text();
    bool ok;
    quint16 port = ui->portLineEdit->text().toUShort(&ok);
    if (!ok) {
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Invalid port. Please enter a number between 1 and 65535."));
        return;
    }

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &SendToDialog::socketConnected);
    connect(socket,
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
            &QTcpSocket::errorOccurred,
#else
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
#endif
            this, &SendToDialog::socketErrorOccurred);
    socket->connectToHost(addr, port);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    setCursor(QCursor(Qt::WaitCursor));
    socketTimeoutTimer.start(5000);
}

void SendToDialog::socketConnected()
{
    socketTimeoutTimer.stop();
    FileTransferSender *sender = new FileTransferSender(nullptr, socket, files);
    FileTransferDialog *d = new FileTransferDialog(nullptr, sender);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
    done(Accepted);
}

void SendToDialog::socketErrorOccurred()
{
    socketTimeoutTimer.stop();
    socket->disconnectFromHost();
    socket->close();
    socket->deleteLater();
    QMessageBox::critical(this, QApplication::applicationName(), socket->errorString());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    setCursor(QCursor(Qt::ArrowCursor));
}

void SendToDialog::socketTimeout()
{
    socket->disconnectFromHost();
    socket->close();
    socket->deleteLater();
    QMessageBox::critical(this, QApplication::applicationName(), tr("Connection timed out"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    setCursor(QCursor(Qt::ArrowCursor));
}

void SendToDialog::hostsListViewManulClicked(const QModelIndex &index)
{
    Endpoint endpoint = endpointsManul[index.row()];
    ui->addrLineEdit->setText(endpointsManul[index.row()].addr);
    ui->portLineEdit->setText(QString::number(endpoint.port));
    ui->deleteButton->setEnabled(true);
}

void SendToDialog::showAddHostsManulDialog()
{
    addHostDialog->setVisible(true);
}

void SendToDialog::addHostManul(const QString &deviceName, const QString &addr, quint16 port)
{
    QStringList l = hostsManulStringListModel.stringList();
    QJsonValue addrValue(addr);
    QJsonValue portValue(port);
    QJsonObject obj;
    obj["host"]=addrValue;
    obj["port"]=portValue;
    jsonObj[deviceName]=obj;
    endpointsManul.append({addr, port});
    l.append(deviceName);
    hostsManulStringListModel.setStringList(l);
    writeHostsJson();
}

void SendToDialog::onDeleteButtonClicked()
{
    QModelIndex index=ui->hostsListViewManul->currentIndex();
    jsonObj.remove(hostsManulStringListModel.stringList().at(index.row()));
    hostsManulStringListModel.removeRow(index.row());
    endpointsManul.removeAt(index.row());
    ui->addrLineEdit->setText("");
    ui->portLineEdit->setText("");
    ui->deleteButton->setEnabled(false);
    writeHostsJson();
}

void SendToDialog::writeHostsJson()
{
    QFile file(HOST_MANUL_FILE);
    if(!file.open(QIODevice::WriteOnly)){
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Error! When open hosts.json!"));
        return;
    }
    QJsonDocument jsonDoc(jsonObj);
    QByteArray data=jsonDoc.toJson();
    file.write(data);
    file.close();
}
