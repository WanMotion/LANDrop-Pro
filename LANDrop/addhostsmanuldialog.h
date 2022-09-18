#ifndef ADDHOSTSMANULDIALOG_H
#define ADDHOSTSMANULDIALOG_H

#include <QDialog>
#include <QHostAddress>

namespace Ui {
class AddHostsManulDialog;
}

class AddHostsManulDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddHostsManulDialog(QWidget *parent = nullptr);
    ~AddHostsManulDialog();

private:
    Ui::AddHostsManulDialog *ui;

signals:
    void addHostManully(const QString &deviceName, const QString &addr, quint16 port);
private slots:
    void accept();
};

#endif // ADDHOSTSMANULDIALOG_H
