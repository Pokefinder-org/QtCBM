#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QString>

namespace Ui {
class settingsDialog;
}

class settingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit settingsDialog(QWidget *parent = 0);
    static QString findCBMUtil(QString name);
    ~settingsDialog();

signals:
    void settingsChanged();

private slots:
    void on_browse_cbmctrl_clicked();
    void on_browse_cbmforng_clicked();
    void on_browse_d64copy_clicked();
    void on_buttonBox_accepted();
    void on_browse_cbmcopy_clicked();
    void on_browse_morse_clicked();

    void on_cbmctrl_reset_clicked();

    void on_usetracks_clicked();

    void on_track_reset_clicked();

    void on_d64copy_reset_clicked();

    void on_cbmcopy_reset_clicked();

    void on_cbmforng_reset_clicked();

    void on_morse_reset_clicked();

private:
    Ui::settingsDialog *ui;
    QSettings *settings;
    QString getTransferMode();
    QString getCableType();

};

#endif // SETTINGSDIALOG_H
