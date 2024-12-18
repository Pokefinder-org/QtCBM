#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>

settingsDialog::settingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsDialog)
{
    ui->setupUi(this);

#ifdef Q_OS_OSX
    ui->cableFrame->setEnabled(true);
#endif

//    QAction *actUseTracks;

    connect(ui->input_usetracks, &QAbstractButton::toggled, this, &settingsDialog::on_usetracks_clicked);

    // Create our context menu items

    settings = new QSettings("mvgrafx", "QtCBM");
    ui->input_cbmctrl->setText(settings->value("tools/cbmctrl", findCBMUtil("cbmctrl")).toString());
    ui->input_cbmforng->setText(settings->value("tools/cbmforng", findCBMUtil("cbmforng")).toString());
    ui->input_d64copy->setText(settings->value("tools/d64copy", findCBMUtil("d64copy")).toString());
    ui->input_cbmcopy->setText(settings->value("tools/cbmcopy", findCBMUtil("cbmcopy")).toString());
    ui->input_nibread->setText(settings->value("tools/nibread", findCBMUtil("nibread")).toString());
    ui->input_nibwrite->setText(settings->value("tools/nibwrite", findCBMUtil("nibwrite")).toString());
    ui->noKillertracks->setChecked(settings->value("noKillertracks", false).toBool());
    ui->parTransfer1571->setChecked(settings->value("parTransfer1571", false).toBool());
    ui->beVerbose->setChecked(settings->value("beVerbose", false).toBool());
    ui->readRetry->setChecked(settings->value("readRetry", false).toBool());
    ui->input_errors->setValue(settings->value("retryErrors", 5).toInt());
    ui->defaultDensities->setChecked(settings->value("defaultDensities", false).toBool());
    ui->readHalftracks->setChecked(settings->value("readHalftracks", false).toBool());
    ui->input_starttrack->setValue(settings->value("starttrack", 1).toInt());
    ui->input_cbmdevice->setValue(settings->value("deviceid", 8).toInt());
    ui->input_usetracks->setChecked(settings->value("usetracks", false).toBool());
    ui->input_starttrack->setValue(settings->value("starttrack", 1).toInt());
    ui->input_endtrack->setValue(settings->value("endtrack", 35).toInt());
    ui->autoRefresh->setChecked(settings->value("autorefresh", true).toBool());
    ui->showCommands->setChecked(settings->value("showcmd", false).toBool());
    //ui->useC64font->setChecked(settings->value("usec64font", false).toBool());
    ui->genRandomDiskname->setChecked(settings->value("genrandomdisk", false).toBool());
    ui->appendMap->setChecked(settings->value("appendmap", false).toBool());
    ui->formatVerify->setChecked(settings->value("formatverify", true).toBool());
    ui->formatExtended->setChecked(settings->value("formatextended", false).toBool());
    ui->formatNobump->setChecked(settings->value("formatnobump", true).toBool());
    ui->formatOriginal->setChecked(settings->value("formatoriginal", true).toBool());

    if (ui->input_usetracks->isChecked())
        ui->frame_3->setEnabled(true);
    else
        ui->frame_3->setEnabled(false);

    QString transfermode = settings->value("transfermode", "auto").toString();
    QString cableType = settings->value("cableType", "xum1541").toString();

    if (transfermode == "original")
        ui->trOriginal->setChecked(true);
    if (transfermode == "serial1")
        ui->trSerial1->setChecked(true);
    if (transfermode == "serial2")
        ui->trSerial2->setChecked(true);
    if (transfermode == "parallel")
        ui->trParallel->setChecked(true);
    if (transfermode == "auto")
        ui->trAuto->setChecked(true);

    if (cableType == "xu1541")
        ui->cableXU->setChecked(true);
    if (cableType == "xum1541")
        ui->cableXUM->setChecked(true);

}

settingsDialog::~settingsDialog()
{
    delete ui;
}

QString settingsDialog::findCBMUtil(QString name)
{
#ifdef Q_OS_WIN
    return QStandardPaths::findExecutable(name+".exe");
#endif
#ifdef Q_OS_OSX
    return QCoreApplication::applicationDirPath()+"/bin/"+name;
#endif
#ifdef Q_OS_LINUX
    return QStandardPaths::findExecutable(name);
#endif
}

void settingsDialog::on_browse_cbmctrl_clicked()
{
    ui->input_cbmctrl->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Find cbmctrl"), "/", tr("Executable Files (*)"))));
}

void settingsDialog::on_browse_cbmforng_clicked()
{
    ui->input_cbmforng->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Find cbmforng"), "/", tr("Executable Files (*)"))));
}

void settingsDialog::on_browse_d64copy_clicked()
{
    ui->input_d64copy->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Find d64copy"), "/", tr("Executable Files (*)"))));
}

void settingsDialog::on_browse_cbmcopy_clicked()
{
    ui->input_cbmcopy->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Find cbmcopy"), "/", tr("Executable Files (*)"))));
}

void settingsDialog::on_browse_nibread_clicked()
{
    ui->input_nibread->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Find nibread"), "/", tr("Executable Files (*)"))));
}

void settingsDialog::on_browse_nibwrite_clicked()
{
    ui->input_nibwrite->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Find nibwrite"), "/", tr("Executable Files (*)"))));
}

void settingsDialog::on_buttonBox_accepted()
{
    settings->setValue("tools/cbmctrl", ui->input_cbmctrl->text());
    settings->setValue("tools/cbmforng", ui->input_cbmforng->text());
    settings->setValue("tools/d64copy", ui->input_d64copy->text());
    settings->setValue("tools/cbmcopy", ui->input_cbmcopy->text());
    settings->setValue("tools/nibread", ui->input_nibread->text());
    settings->setValue("tools/nibwrite", ui->input_nibwrite->text());
    settings->setValue("parTransfer1571", ui->parTransfer1571->isChecked());
    settings->setValue("beVerbose", ui->beVerbose->isChecked());
    settings->setValue("readRetry", ui->readRetry->isChecked());
    settings->setValue("retryErrors", ui->input_errors->value());
    settings->setValue("noKillertracks", ui->noKillertracks->isChecked());
    settings->setValue("defaultDensities", ui->defaultDensities->isChecked());
    settings->setValue("readHalftracks", ui->readHalftracks->isChecked());
    settings->setValue("deviceid", ui->input_cbmdevice->value());
    settings->setValue("usetracks", ui->input_usetracks->isChecked());
    settings->setValue("starttrack", ui->input_starttrack->value());
    settings->setValue("endtrack", ui->input_endtrack->value());
    settings->setValue("transfermode", getTransferMode());
    settings->setValue("showcmd", ui->showCommands->isChecked());
    settings->setValue("autorefresh", ui->autoRefresh->isChecked());
    //settings->setValue("usec64font", ui->useC64font->isChecked());
    settings->setValue("genrandomdisk", ui->genRandomDiskname->isChecked());
    settings->setValue("appendmap", ui->appendMap->isChecked());
    settings->setValue("formatverify", ui->formatVerify->isChecked());
    settings->setValue("formatextended", ui->formatExtended->isChecked());
    settings->setValue("formatnobump", ui->formatNobump->isChecked());
    settings->setValue("formatoriginal", ui->formatOriginal->isChecked());

    settings->setValue("cableType", getCableType());
    settings->sync();

    emit settingsChanged();
}

QString settingsDialog::getCableType()
{
    if (ui->cableXU->isChecked())
        return "xu1541";
    if (ui->cableXUM->isChecked())
        return "xum1541";
    return "xum1541";
}

QString settingsDialog::getTransferMode()
{
    if (ui->trOriginal->isChecked())
        return "original";
    if (ui->trSerial1->isChecked())
        return "serial1";
    if (ui->trSerial2->isChecked())
        return "serial2";
    if (ui->trParallel->isChecked())
        return "parallel";
    if (ui->trAuto->isChecked())
        return "auto";
    return "auto";
}

void settingsDialog::on_cbmctrl_reset_clicked()
{
    ui->input_cbmctrl->setText(findCBMUtil("cbmctrl"));
}

void settingsDialog::on_usetracks_clicked()
{
    if (ui->input_usetracks->isChecked())
        ui->frame_3->setEnabled(true);
    else
        ui->frame_3->setEnabled(false);
}

void settingsDialog::on_track_reset_clicked()
{
    ui->input_cbmdevice->setValue(8);
    ui->input_starttrack->setValue(1);
    ui->input_endtrack->setValue(35);
}

void settingsDialog::on_reset_errors_clicked()
{
    ui->input_errors->setValue(5);
}

void settingsDialog::on_d64copy_reset_clicked()
{
    ui->input_d64copy->setText(findCBMUtil("d64copy"));
}

void settingsDialog::on_cbmcopy_reset_clicked()
{
    ui->input_cbmcopy->setText(findCBMUtil("cbmcopy"));
}

void settingsDialog::on_cbmforng_reset_clicked()
{
    ui->input_cbmforng->setText(findCBMUtil("cbmforng"));
}

void settingsDialog::on_nibread_reset_clicked()
{
    ui->input_nibread->setText(findCBMUtil("nibread"));
}

void settingsDialog::on_nibwrite_reset_clicked()
{
    ui->input_nibwrite->setText(findCBMUtil("nibwrite"));
}
