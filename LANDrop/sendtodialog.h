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

#pragma once

#include <QDialog>
#include <QFile>
#include <QStringListModel>
#include <QTcpSocket>
#include <QTimer>

#include "discoveryservice.h"
#include "addhostsmanuldialog.h"

namespace Ui {
    class SendToDialog;
}

class SendToDialog : public QDialog {
    Q_OBJECT
public:
    explicit SendToDialog(QWidget *parent, const QList<QSharedPointer<QFile>> &files,
                          DiscoveryService &discoveryService);
    ~SendToDialog();
private:
    Ui::SendToDialog *ui;
    QList<QSharedPointer<QFile>> files;
    QStringListModel hostsStringListModel;
    QStringListModel hostsManulStringListModel;
    struct Endpoint {
        QHostAddress addr;
        quint16 port;
    };
    QVector<Endpoint> endpoints;
    QVector<Endpoint> endpointsManul;
    QTimer discoveryTimer;
    QTcpSocket *socket;
    QTimer socketTimeoutTimer;
    AddHostsManulDialog *addHostDialog;

private slots:
    void newHost(const QString &deviceName, const QHostAddress &addr, quint16 port);
    void hostsListViewClicked(const QModelIndex &index);
    void accept();
    void socketConnected();
    void socketErrorOccurred();
    void socketTimeout();
    void hostsListViewManulClicked(const QModelIndex &index);
    void showAddHostsManulDialog();
    void addHostManul(const QString &deviceName, const QHostAddress &addr, quint16 port);
    void onDeleteButtonClicked();
};
