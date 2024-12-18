#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfoList>
#include <QFileSystemModel>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QRegExp>
#include <QString>
#include <QThread>
#include <QTreeWidgetItem>
#include <QUrl>
#include <string>

#include <QtDebug>

#include "filewindow.h"
#include "ui_filewindow.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
#include "cbmroutines.h"
#include "detailsinfodialog.h"
#include "selectd64filesdialog.h"
#include "version.h"
//#include "diskimage/diskimage.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

FileWindow::FileWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FileWindow)
{
    QAction *actMakeDir, *actRenameFile, *actDeleteFile, *actViewFile, *actOpenD64;
    cbmctrlhasraw = false;
    selectedLocalFolder = "";

    ui->setupUi(this);

    this->setWindowTitle("QtCBM v"+QString(__QTCBM_VERSION__));

    QString c64TreeStyle = "QTreeWidget {background-color: #4E2EDE; color: #A7A1FD; }";
    QString c64LineStyle = "QLineEdit {background-color: #4E2EDE; color: #A7A1FD; }";

    // connect the signals for handling out of thread execution
    proc_cbmStatus = new QProcess(this);
    connect(proc_cbmStatus, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(cbmStatusFinished(int,QProcess::ExitStatus)));

    proc_just_CBMStatus = new QProcess(this);
    connect(proc_just_CBMStatus, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(cbmJustStatusFinished(int,QProcess::ExitStatus)));

    proc_d64copy = new QProcess(this);
    connect(proc_d64copy, SIGNAL(readyReadStandardOutput()), this, SLOT(cbmCopyProgress()));
    connect(proc_d64copy, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmCopyFinished(int,QProcess::ExitStatus)));

    proc_nibwrite = new QProcess(this);
    connect(proc_nibwrite, SIGNAL(readyReadStandardOutput()), this, SLOT(cbmNibwriteProgress()));
    connect(proc_nibwrite, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmNibwriteFinished(int,QProcess::ExitStatus)));

    proc_nibread = new QProcess(this);
    connect(proc_nibread, SIGNAL(readyReadStandardOutput()), this, SLOT(cbmNibreadProgress()));
    connect(proc_nibread, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmNibreadFinished(int,QProcess::ExitStatus)));

    proc_cbmDir = new QProcess(this);
    connect(proc_cbmDir, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmDirFinished(int,QProcess::ExitStatus)));

    proc_cbmReset = new QProcess(this);
    connect(proc_cbmReset, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmResetFinished(int,QProcess::ExitStatus)));

    proc_cbmFormat = new QProcess(this);
    connect(proc_cbmFormat, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmFormatFinished(int,QProcess::ExitStatus)));

    proc_cbmInit = new QProcess(this);
    connect(proc_cbmInit, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmInitFinished(int,QProcess::ExitStatus)));

    proc_cbmValidate = new QProcess(this);
    connect(proc_cbmValidate, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmValidateFinished(int,QProcess::ExitStatus)));

    proc_cbmScratch = new QProcess(this);
    connect(proc_cbmScratch, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmScratchFinished(int,QProcess::ExitStatus)));

    proc_cbmRename = new QProcess(this);
    connect(proc_cbmRename, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmRenameFinished(int,QProcess::ExitStatus)));

    proc_cbmDetect = new QProcess(this);
    connect(proc_cbmDetect, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmDetectFinished(int,QProcess::ExitStatus)));

//    proc_morse = new QProcess(this);
//    connect(proc_morse, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(morseFinished(int,QProcess::ExitStatus)));

    proc_cbmcopy = new QProcess(this);
    connect(proc_cbmcopy, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(cbmFileCopyFinished(int,QProcess::ExitStatus)));

#ifdef Q_OS_OSX
    // Set up the environment to use bundled opencbm on Mac
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("OPENCBM_HOME", QCoreApplication::applicationDirPath());

    proc_cbmcopy->setProcessEnvironment(env);
    proc_cbmDetect->setProcessEnvironment(env);
    proc_cbmDir->setProcessEnvironment(env);
    proc_cbmFormat->setProcessEnvironment(env);
    proc_cbmInit->setProcessEnvironment(env);
    proc_cbmRename->setProcessEnvironment(env);
    proc_cbmReset->setProcessEnvironment(env);
    proc_cbmScratch->setProcessEnvironment(env);
    proc_cbmStatus->setProcessEnvironment(env);
    proc_cbmValidate->setProcessEnvironment(env);
    proc_d64copy->setProcessEnvironment(env);
//    proc_morse->setProcessEnvironment(env);
#endif

    // initialize the settings object
    settings = new QSettings("mvgrafx", "QtCBM");
    loadSettings();

    // Create our context menu items
    actMakeDir = new QAction(tr("New Folder"), ui->localFolders);
    connect(actMakeDir, SIGNAL(triggered()), this, SLOT(act_newFolder()));
    ui->localFolders->addAction(actMakeDir);
    ui->localFolders->setContextMenuPolicy(Qt::ActionsContextMenu);

    actRenameFile = new QAction(tr("Rename"), ui->localFiles);
    actDeleteFile = new QAction(tr("Delete"), ui->localFiles);
    actViewFile = new QAction(tr("Run/View"), ui->localFiles);
    actOpenD64 = new QAction(tr("Examine D64"), ui->localFiles);

    connect(actRenameFile, SIGNAL(triggered()), this, SLOT(act_renameFile()));
    connect(actDeleteFile, SIGNAL(triggered()), this, SLOT(act_deleteFile()));
    connect(actViewFile, SIGNAL(triggered()), this, SLOT(act_viewFile()));
    connect(actOpenD64, SIGNAL(triggered()), this, SLOT(act_viewD64()));

    ui->localFiles->addAction(actRenameFile);
    ui->localFiles->addAction(actDeleteFile);
    ui->localFiles->addAction(actViewFile);
    ui->localFiles->addAction(actOpenD64);

    ui->localFiles->setContextMenuPolicy(Qt::ActionsContextMenu);

    // set up the folders view
    foldersModel = new QFileSystemModel(this);
    foldersModel->setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    foldersModel->setRootPath(QDir::rootPath());
    ui->localFolders->setModel(foldersModel);
    ui->localFolders->setRootIndex(foldersModel->index(QDir::rootPath()));
    ui->localFolders->hideColumn(1);
    ui->localFolders->hideColumn(2);
    ui->localFolders->hideColumn(3);
    ui->localFolders->setAnimated(false);

    filesModel = new QFileSystemModel();
    filesModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    filesModel->setRootPath(QDir::rootPath());
    ui->localFiles->setModel(filesModel);
    ui->localFiles->setRootIndex(filesModel->index(QDir::rootPath()));

    ui->actionView_Home_Folder->setChecked(false);
    ui->actionView_Drive->setChecked(true);



    // size the CBM file list headers
    ui->cbmFiles->header()->resizeSection(0, 45);
    ui->cbmFiles->header()->resizeSection(1, 140);
    ui->cbmFiles->header()->resizeSection(2, 40);
    ui->cbmFiles->header()->resizeSection(3, 60);
    ui->cbmFiles->hideColumn(4);

    // set the colors of the CBM elements
    ui->diskLabel->setStyleSheet(c64LineStyle);
    ui->diskId->setStyleSheet(c64LineStyle);
    ui->freeSpace->setStyleSheet(c64LineStyle);
    ui->cbmFiles->setStyleSheet(c64TreeStyle);
}

FileWindow::~FileWindow()
{
	// save current window geometry
    settings->setValue("ui/geometry", this->saveGeometry());
    settings->setValue("ui/state", this->saveState());
    settings->sync();

    delete ui;
}

void FileWindow::on_action_Quit_triggered()
{
    this->close();
}

void FileWindow::on_actionAbout_triggered()
{
    aboutDialog *dlg = new aboutDialog(this);
    dlg->show();
}

void FileWindow::disableUIElements()
{
    ui->copyToCBM->setEnabled(false);
    ui->copyFromCBM->setEnabled(false);
    ui->CBMDirectory->setEnabled(false);
    ui->CBMFormat->setEnabled(false);
    ui->CBMInitialize->setEnabled(false);
    ui->CBMValidate->setEnabled(false);
    ui->CBMRename->setEnabled(false);
    ui->CBMScratch->setEnabled(false);
    ui->CBMStatus->setEnabled(false);
    ui->menuTools->setEnabled(false);
}

void FileWindow::enableUIElements()
{
    QThread::sleep(2);
    ui->copyToCBM->setEnabled(true);
    ui->copyFromCBM->setEnabled(true);
    ui->CBMDirectory->setEnabled(true);
    ui->CBMFormat->setEnabled(true);
    ui->CBMInitialize->setEnabled(true);
    ui->CBMValidate->setEnabled(true);
    ui->CBMRename->setEnabled(true);
    ui->CBMScratch->setEnabled(true);
    ui->CBMStatus->setEnabled(true);
    ui->menuTools->setEnabled(true);
}

void FileWindow::resetUI()
{
    // remove the progress bar
    if (progbar)
        ui->statusBar->removeWidget(progbar);

    delete progbar;
    enableUIElements();
}

bool FileWindow::confirmExecute(QString command, QStringList params)
{
    QFileInfo file(command);

    if (!file.isExecutable())
    {
        QMessageBox::warning(this, "QtCBM", file.baseName()+"."+file.completeSuffix()+" is not an executable file.", QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }

    if (showcmd)
    {
        QFileInfo file = QFileInfo(command);
#ifdef Q_OS_WIN
        return (QMessageBox::information(this, "QtCBM", "About to execute:\n\n"+file.baseName()+"."+file.completeSuffix()+" "+params.join(' ')+"\n\nContinue?", QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes);
#else
        return (QMessageBox::information(this, "QtCBM", "About to execute:\n\n"+file.baseName()+" "+params.join(' ')+"\n\nContinue?", QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes);
#endif
    }
    return true;
}

// handles a d64 passed on the commandline
void FileWindow::writeD64FromArgs(QString filename)
{
    fileFromArgs = filename;
    on_copyToCBM_clicked();
}


void FileWindow::writeCBMconf()
{
    QString confPath = QCoreApplication::applicationDirPath()+"/etc/opencbm.conf";
    QDir appPath(QCoreApplication::applicationDirPath()); //+"/etc");

    if (!appPath.exists("etc/"))
    {
        if (!appPath.mkdir("etc/"))
        {
            QMessageBox::warning(this,"QtCBM","Unable to create opencbm.conf", QMessageBox::Ok, QMessageBox::Ok);
        }
    }

    QFile confFile(confPath);
    if (confFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QTextStream stream(&confFile);
        stream << "[plugins]" << Qt::endl;
        stream << "default="+cableType << Qt::endl;
        stream << "[xu1541]" << Qt::endl;
        stream << "location="+QCoreApplication::applicationDirPath()+"/plugins/libopencbm-xu1541.0.4.99.97.dylib" << Qt::endl;
        stream << "[xum1541]" << Qt::endl;
        stream << "location="+QCoreApplication::applicationDirPath()+"/plugins/libopencbm-xum1541.0.4.99.97.dylib" << Qt::endl;
    } else
    {
        QMessageBox::warning(this,"QtCBM","Unable to write opencbm.conf: "+confFile.errorString(), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    confFile.close();
}

void FileWindow::loadSettings()
{
    QFont font11;
    QFont font8;

    // read in settings

    // restore window size/position/state
    this->restoreGeometry(settings->value("ui/geometry").toByteArray());
    this->restoreState(settings->value("ui/state").toByteArray());

    // read in application settings
    cbmctrl = settings->value("tools/cbmctrl", settingsDialog::findCBMUtil("cbmctrl")).toString();
    cbmforng = settings->value("tools/cbmforng", settingsDialog::findCBMUtil("cbmforng")).toString();
    d64copy = settings->value("tools/d64copy", settingsDialog::findCBMUtil("d64copy")).toString();
    nibwrite = settings->value("tools/nibwrite", settingsDialog::findCBMUtil("nibwrite")).toString();
    nibread = settings->value("tools/nibread", settingsDialog::findCBMUtil("nibread")).toString();
    cbmcopy = settings->value("tools/cbmcopy", settingsDialog::findCBMUtil("cbmcopy")).toString();
//    morse = settings->value("tools/morse", settingsDialog::findCBMUtil("morse")).toString();
    parTransfer1571 = settings->value("parTransfer1571", false).toBool();
    beVerbose = settings->value("beVerbose", false).toBool();
    deviceid = settings->value("deviceid", 8).toInt();
    transfermode = settings->value("transfermode", "auto").toString();
    usetracks = settings->value("usetracks", false).toBool();
    starttrack = settings->value("starttrack", 1).toInt();
    endtrack = settings->value("endtrack", 35).toInt();
    readRetry = settings->value("readRetry", false).toBool();
    retryErrors = settings->value("retryErrors", 5).toInt();
    noKillertracks = settings->value("noKillertracks", false).toBool();
    defaultDensities = settings->value("defaultDensities", false).toBool();
    readHalftracks = settings->value("readHalftracks", false).toBool();
    trackMatching = settings->value("trackMatching", false).toBool();
    showcmd = settings->value("showcmd", false).toBool();
    autorefresh = settings->value("autorefresh", true).toBool();
    generateRandomDiskname = settings->value("genrandomdisk", false).toBool();
    formatVerify = settings->value("formatverify", true).toBool();
    formatExtended = settings->value("formatextended", false).toBool();
    formatOriginal = settings->value("formatoriginal", true).toBool();
    formatNobump = settings->value("formatnobump", true).toBool();
    cableType = settings->value("cableType", "xum1541").toString();

#ifdef Q_OS_OSX
    writeCBMconf();
#endif

    // Load the C64 system font from resources
    fontDB = new QFontDatabase();
    fontDB->addApplicationFont(":/res/fonts/C64_Elite_Mono_v1.0-STYLE.ttf");
    fontDB->addApplicationFont(":/res/fonts/c64.ttf");

    QFileInfo file(cbmctrl);
    if (file.isExecutable())
    {
        QProcess *proc = new QProcess(this);

        proc->start(cbmctrl, QStringList() << "--version", QIODevice::ReadWrite | QIODevice::Text);
        if (!proc->waitForStarted())
        {
            cbmctrlhasraw = false;
        } else
        {
            QByteArray data;

            while(proc->waitForReadyRead())
                data.append(proc->readAllStandardOutput());

            QString s_ver = QString(data).trimmed();

            QRegExp rx("cbmctrl\\s+version\\s+(\\d+)\\.(\\d+)\\.(\\d+)");
            if (rx.indexIn(s_ver) >= 0)
            {
                int major = rx.cap(1).toInt();
                int minor = rx.cap(2).toInt();
                int revis = rx.cap(3).toInt();

                cbmctrlhasraw = (major > 0 || minor > 4 || revis >= 99);
            }
        }
    }

        if (cbmctrlhasraw)
        {
            font8 = QFont("C64 Elite Mono", 7);
            font11 = QFont("C64 Elite Mono", 10);
        } else
        {
            font8 = QFont("C64 Pro Mono", 7);
            font11 = QFont("C64 Pro Mono", 10);
        }

    ui->diskLabel->setFont(font11);
    ui->diskId->setFont(font11);
    ui->freeSpace->setFont(font11);
    ui->cbmFiles->setFont(font8);

    ui->statusBar->showMessage("Settings read", 5000);
}

void FileWindow::selectedD64contents(QByteArray list)
{
    QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
    QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();
    QFileInfo fi(model->filePath(index.at(0)));
    QDir dir(selectedLocalFolder);

    QList<QByteArray> filelist = list.split('\n');

    if (!dir.exists(fi.completeBaseName()))
        if (!dir.mkdir(fi.completeBaseName()))
        {
            QMessageBox::critical(this,"QtCBM","Couldn't create destination folder \""+fi.completeBaseName()+"\" in \""+dir.absolutePath()+"\"",QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

    progbar = new QProgressBar(this);
    progbar->setMinimum(0);
    progbar->setMaximum(filelist.count());
    progbar->setTextVisible(true);
    ui->statusBar->addPermanentWidget(progbar);

    for (int i = 0; i < filelist.count(); i++)
    {
        if (!filelist.at(i).isEmpty())
        {
            CBMroutines::copyFromD64(model->filePath(index.at(0)), filelist.at(i), dir.absolutePath()+"/"+fi.completeBaseName());
            progbar->setValue(i);
        }
    }

    ui->statusBar->removeWidget(progbar);
    delete progbar;
}

void FileWindow::on_localFolders_clicked(const QModelIndex &index)
{
    QFileSystemModel *model = (QFileSystemModel*)ui->localFolders->model();
    QDir *dir = new QDir(model->filePath(index));

    filesModel = new QFileSystemModel();
    filesModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    filesModel->setRootPath(dir->path());
    ui->localFiles->setModel(filesModel);
    ui->localFiles->setRootIndex(filesModel->index(dir->path()));
    selectedLocalFolder = dir->path();
    ui->localFiles->setCurrentIndex(filesModel->index(0,0,ui->localFiles->rootIndex()));
    ui->localFiles->setUniformItemSizes(true);

    selectModel = ui->localFiles->selectionModel();
    connect(selectModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(localFiles_selectionChanged(QItemSelection,QItemSelection)));
}

void FileWindow::localFiles_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    (void)selected;
    (void)deselected;

    qint64 size, blocks=0;
    size = 0;
    //blocks = 0;

    QModelIndexList list = ui->localFiles->selectionModel()->selectedIndexes();

    foreach (const QModelIndex &index, list)
    {
        QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();
        QFile *file = new QFile(model->filePath(index));
        size+=file->size();
    }
    blocks = size / 256;
    QString sizeString = CBMroutines::formatFileSize(size);
    QString blockString = QString::number(blocks);
    ui->selectedKB->setText(sizeString);
    ui->selectedBlocks->setText(blockString.append(" blocks"));
}

void FileWindow::act_newFolder()
{
    bool ok;

    QModelIndexList index = ui->localFolders->selectionModel()->selectedIndexes();
    QFileSystemModel *model = (QFileSystemModel*)ui->localFolders->model();
    QDir *dir = new QDir(model->filePath(index.at(0)));

    QString text = QInputDialog::getText(this, tr("New Folder"), tr("Folder name:"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty())
    {
        if (dir->mkdir(text))
        {
            ui->statusBar->showMessage(tr("Folder created"), 5000);
        } else
        {
            QMessageBox::warning(this,tr("Error"),tr("Could not create a new folder"), QMessageBox::Ok,QMessageBox::Ok);
        }
    }
}

QFile * FileWindow::act_renameFile()
{
    bool ok;

    QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
    QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();

    if (index.count() < 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
    } else if (index.count() > 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("Can't rename multiple files"), QMessageBox::Ok, QMessageBox::Ok);
    } else {
        QFile *file = new QFile(model->filePath(index.at(0)));
        QFileInfo fileInfo(model->filePath(index.at(0)));

        QString text = QInputDialog::getText(this, tr("Rename File"), tr("New name:"), QLineEdit::Normal, fileInfo.baseName()+"."+fileInfo.completeSuffix(), &ok);
        if (ok && !text.isEmpty())
        {
            QString newPath = fileInfo.canonicalPath()+QDir::separator()+text;
            if (file->rename(newPath))
            {
                ui->statusBar->showMessage(tr("File renamed"), 5000);
            } else
            {
                QMessageBox::warning(this,tr("Error"),tr("Could not rename file"), QMessageBox::Ok, QMessageBox::Ok);
            }
        }
        return file;
    }
    return NULL;
}

void FileWindow::act_deleteFile()
{
    QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
    QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();

    if (index.count() < 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
    } else if (index.count() > 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("Won't delete multiple files"), QMessageBox::Ok, QMessageBox::Ok);
    } else {
        QFile *file = new QFile(model->filePath(index.at(0)));

        if (QMessageBox::question(this,tr("Delete File?"), tr("Are you sure?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        {
            if (file->remove())
            {
                ui->statusBar->showMessage(tr("File deleted"), 5000);
            } else
            {
                QMessageBox::warning(this,tr("Error"),tr("Could not delete file"), QMessageBox::Ok, QMessageBox::Ok);
            }
        }
    }
}

void FileWindow::act_viewFile()
{
    QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
    QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();

    if (index.count() < 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
    } else if (index.count() > 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("Can't run multiple files"), QMessageBox::Ok, QMessageBox::Ok);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(model->filePath(index.at(0))));
    }
}

void FileWindow::act_viewD64()
{
    QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
    QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();

    if (index.count() < 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
    } else if (index.count() > 1)
    {
        QMessageBox::warning(this,tr("Error"), tr("Can't view multiple files"), QMessageBox::Ok, QMessageBox::Ok);
    } else
    {
        QFileInfo file(model->filePath(index.at(0)));
        if (file.completeSuffix().toUpper() != "D64")
        {
            QMessageBox::warning(this, "QtCBM", "This is not a D64 file", QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        Selectd64FilesDialog *dlg = new Selectd64FilesDialog(this);

        connect(dlg, SIGNAL(sendSelectedContents(QByteArray)), this, SLOT(selectedD64contents(QByteArray)));
        dlg->setD64File(model->filePath(index.at(0)));
        dlg->exec();
    }
}

void FileWindow::on_localFiles_doubleClicked(const QModelIndex &index)
{
    (void)index;
    act_viewFile();
}

void FileWindow::on_actionPreferences_triggered()
{
    settingsDialog *dlg = new settingsDialog();

    connect(dlg, SIGNAL(settingsChanged()), this, SLOT(loadSettings()));
    dlg->show();
}

void FileWindow::on_CBMStatus_clicked()
{
    if (confirmExecute(cbmctrl, QStringList() << "status" << QString::number(deviceid)))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_cbmStatus->start(cbmctrl, QStringList() << "status" << QString::number(deviceid), QIODevice::ReadWrite | QIODevice::Text);
        if (!proc_cbmStatus->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmStatus->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }
    }
}

void FileWindow::just_CBMStatus()
{

    /*
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);
*/
        qDebug() << "BIN DA! ";

        proc_just_CBMStatus->start(cbmctrl, QStringList() << "status" << QString::number(deviceid), QIODevice::ReadWrite | QIODevice::Text);
        if (!proc_just_CBMStatus->waitForStarted())
        {
            qDebug() << "BIN TOT! ";
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_just_CBMStatus->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;

        } else
        {
            disableUIElements();
        }
}

void FileWindow::cbmJustStatusFinished(int, QProcess::ExitStatus)
{
    //resetUI();

    ui->statusBar->showMessage("Drive status: "+proc_just_CBMStatus->readAllStandardOutput());
}

void FileWindow::on_actionView_Drive_triggered()
{
    foldersModel->setRootPath(QDir::rootPath());
    ui->localFolders->setRootIndex(foldersModel->index(QDir::rootPath()));

    filesModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    filesModel->setRootPath(QDir::rootPath());
    ui->localFiles->setRootIndex(filesModel->index(QDir::rootPath()));
    ui->actionView_Home_Folder->setChecked(false);
    ui->actionView_Drive->setChecked(true);
}

void FileWindow::on_actionView_Home_Folder_triggered()
{
    foldersModel->setRootPath(QDir::homePath());
    ui->localFolders->setRootIndex(foldersModel->index(QDir::homePath()));

    filesModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    filesModel->setRootPath(QDir::homePath());
    ui->localFiles->setRootIndex(filesModel->index(QDir::homePath()));
    ui->actionView_Home_Folder->setChecked(true);
    ui->actionView_Drive->setChecked(false);
}

void FileWindow::cbmStatusFinished(int, QProcess::ExitStatus)
{
    resetUI();

    ui->statusBar->showMessage("Drive status: "+proc_cbmStatus->readAllStandardOutput());
}

void FileWindow::cbmFileCopyFinished(int, QProcess::ExitStatus)
{

    qDebug() << proc_cbmcopy->readAllStandardError();
    qDebug() << proc_cbmcopy->readAllStandardOutput();
    resetUI();

    ui->statusBar->showMessage("Copy completed");
    if (autorefresh)
        on_CBMDirectory_clicked();
}

void FileWindow::copyToCBM(QStringList list)
{
    if (confirmExecute(cbmcopy, QStringList() << "--transfer="+transfermode << "-q" << "-w" << QString::number(deviceid) << list))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        progbar->setTextVisible(true);
        ui->statusBar->addPermanentWidget(progbar);
        ui->copyToCBM->setEnabled(false);
        ui->statusBar->showMessage("Writing file(s)");

        proc_cbmcopy->start(cbmcopy, QStringList() << "--transfer="+transfermode << "-q" << "-w" << QString::number(deviceid) << list, QIODevice::ReadWrite | QIODevice::Text);
        if (!proc_cbmcopy->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmcopy+"\n\nExit status: "+QString::number(proc_cbmcopy->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            ui->statusBar->removeWidget(btn_abort);
            delete progbar;
            delete btn_abort;
        } else
        {
            disableUIElements();
        }
    }
}

void FileWindow::on_copyCBMfileToDisk_clicked()
{
    if (ui->localFiles->model() != NULL)
    {
        QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
        QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();

        if (index.count() < 1)
        {
            QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
            return;
        } else if (index.count() >= 1)
        {
            QStringList fileList;

            for (int i = 0; i < index.count(); i++)
            {
                //qDebug() << model->filePath(index.at(i));
                fileList << QDir::toNativeSeparators(model->filePath(index.at(i)));
            }
            copyToCBM(fileList);

            return;
        }
    }
}

void FileWindow::on_copyToCBM_clicked()
{
    QString fileToCopy;

    if (!fileFromArgs.isEmpty())
    {
        if (QMessageBox::question(this, "QtCBM", "Do you want to copy\n"+fileFromArgs+"\nTo the current disk now?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            fileToCopy = fileFromArgs;
            fileFromArgs.clear();
        } else
        {
            fileFromArgs.clear();
            return;
        }
    } else
    {
        if (ui->localFiles->model() != NULL)
        {
            QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
            QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();

            if (index.count() < 1)
            {
                QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
                return;
            } else if (index.count() > 1)
            {
                //QStringList fileList;
                QMessageBox::warning(this,tr("QtCBM"), tr("Unable to write multiple D64 files at once."), QMessageBox::Ok, QMessageBox::Ok);
                /*
                {
                    for (int i = 0; i < index.count(); i++)
                    {
                        //qDebug() << model->filePath(index.at(i));
                        fileList << QDir::toNativeSeparators(model->filePath(index.at(i)));
                    }
                    copyToCBM(fileList);
                }
                */
                return;
            } else {
                fileToCopy = model->filePath(index.at(0));
            }
        } else
        {
            QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
    }

    QFileInfo fileinfo(fileToCopy);
    QString ext = fileinfo.completeSuffix().toUpper();
    if (ext == "D64")
    {
        if (confirmExecute(d64copy, QStringList() << "--transfer="+transfermode << QDir::toNativeSeparators(fileToCopy) << QString::number(deviceid)))
        {
            progbar = new QProgressBar(this);
            progbar->setMinimum(0);
            progbar->setMaximum(100);
            progbar->setTextVisible(true);
            btn_abort = new QPushButton(this);
            connect(btn_abort, SIGNAL(clicked()), this, SLOT(stopCopy()));
            btn_abort->setText("X");
            btn_abort->setToolTip(tr("Abort the current transfer and reset the CBM bus"));
            btn_abort->setFixedHeight(18);
            btn_abort->setFixedWidth(18);
            ui->statusBar->addPermanentWidget(progbar);
            ui->statusBar->addPermanentWidget(btn_abort);
            timer = new QTimer(this);
            timer->setInterval(10000);
            connect(timer, SIGNAL(timeout()), this, SLOT(timerClick("writing")));
            ui->copyToCBM->setEnabled(false);
            QFileInfo file(fileToCopy);
            ui->statusBar->showMessage("Writing: "+file.baseName()+"."+file.completeSuffix());
            d64imageFile = file.baseName()+"."+file.completeSuffix();

            proc_d64copy->start(d64copy, QStringList() << "--transfer="+transfermode << QDir::toNativeSeparators(fileToCopy) << QString::number(deviceid), QIODevice::ReadWrite | QIODevice::Text);
            if (!proc_d64copy->waitForStarted())
            {
                QMessageBox::warning(this,"Error", "Failed to execute "+d64copy+"\n\nExit status: "+QString::number(proc_d64copy->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
                ui->statusBar->removeWidget(progbar);
                ui->statusBar->removeWidget(btn_abort);
                delete progbar;
                delete btn_abort;
            } else
            {
                timer->start();
                disableUIElements();
                currBlock = 0;
                lastBlock = 0;
            }
        }
    } /*else if (ext == "PRG")
    {

        if (confirmExecute(cbmcopy, QStringList() << "--transfer="+transfermode << "-q" << "-w" << QString::number(deviceid) << QDir::toNativeSeparators(fileToCopy) << "--output" << fileinfo.baseName()))
        {
            progbar = new QProgressBar(this);
            progbar->setMinimum(0);
            progbar->setMaximum(0);
            progbar->setTextVisible(true);
            ui->statusBar->addPermanentWidget(progbar);
            ui->copyToCBM->setEnabled(false);
            QFileInfo file(fileToCopy);
            ui->statusBar->showMessage("Writing: "+file.baseName()+"."+file.completeSuffix());
            d64imageFile = file.baseName()+"."+file.completeSuffix();

            proc_cbmcopy->start(cbmcopy, QStringList() << "--transfer="+transfermode << "-q" << "-w" << QString::number(deviceid) << QDir::toNativeSeparators(fileToCopy) << "--output" << fileinfo.baseName(), QIODevice::ReadWrite | QIODevice::Text);
            if (!proc_cbmcopy->waitForStarted())
            {
                QMessageBox::warning(this,"Error", "Failed to execute "+cbmcopy+"\n\nExit status: "+QString::number(proc_cbmcopy->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
                ui->statusBar->removeWidget(progbar);
                ui->statusBar->removeWidget(btn_abort);
                delete progbar;
                delete btn_abort;
            } else
            {
                disableUIElements();
            }
        }
    } */ else
    {
        QMessageBox::information(this, "QtCBM", fileinfo.completeSuffix().toUpper()+" is not a valid filetype to transfer", QMessageBox::Ok, QMessageBox::Ok);
    }
}

void FileWindow::timerClick(QString rwstate)
{
    if (currBlock > lastBlock)
    {
        lastBlock = currBlock;
    } else
    {
        timer->stop();
        if (QMessageBox::critical(this, "QtCBM", "Trouble "+rwstate+" "+d64imageFile+"\n\nAbort?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            stopCopy();
        } else {
            lastBlock = currBlock;
            timer->start();
        }
    }
}

void FileWindow::on_copyNibwrite_clicked()
{
    QString fileToCopy;

    if (!fileFromArgs.isEmpty())
    {
        if (QMessageBox::question(this, "QtCBM", "Do you want to copy\n"+fileFromArgs+"\nTo the current disk now?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            fileToCopy = fileFromArgs;
            fileFromArgs.clear();
        } else
        {
            fileFromArgs.clear();
            return;
        }
    } else
    {
        if (ui->localFiles->model() != NULL)
        {
            QModelIndexList index = ui->localFiles->selectionModel()->selectedIndexes();
            QFileSystemModel *model = (QFileSystemModel*)ui->localFiles->model();

            if (index.count() < 1)
            {
                QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
                return;
            } else if (index.count() > 1)
            {
                //QStringList fileList;
                QMessageBox::warning(this,tr("QtCBM"), tr("Unable to write multiple images at once."), QMessageBox::Ok, QMessageBox::Ok);
                /*
                {
                    for (int i = 0; i < index.count(); i++)
                    {
                        //qDebug() << model->filePath(index.at(i));
                        fileList << QDir::toNativeSeparators(model->filePath(index.at(i)));
                    }
                    copyToCBM(fileList);
                }
                */
                return;
            } else {
                fileToCopy = model->filePath(index.at(0));
            }
        } else
        {
            QMessageBox::warning(this,tr("Error"), tr("No files selected"), QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
    }

    QFileInfo fileinfo(fileToCopy);
    QString ext = fileinfo.completeSuffix().toUpper();
    QStringList params;

    params << "-D" + QString::number(deviceid);

    if (usetracks)
    {
        tracks = "-S"+QString::number(starttrack);
        moretracks =  "-E"+QString::number(endtrack);
        params << tracks << moretracks;
    }

    if (parTransfer1571)
    {
        params << "-P";
    }

    if (readRetry)
    {
        params << "-P";
    }

    if (beVerbose)
    {
        params << "-v";
    }

    qDebug() << "nibwrite params:" << params;

    if (ext == "D64" || ext == "G64" || ext == "NIB" || ext == "NBZ")
    {
        if (confirmExecute(nibwrite, QStringList() << params << QDir::toNativeSeparators(fileToCopy)))
        {
            progbar = new QProgressBar(this);
            progbar->setMinimum(0);
            progbar->setMaximum(100);
            progbar->setTextVisible(true);
            btn_abort = new QPushButton(this);
            connect(btn_abort, SIGNAL(clicked()), this, SLOT(stopNibwrite()));
            btn_abort->setText("X");
            btn_abort->setToolTip(tr("Abort the current transfer and reset the CBM bus"));
            btn_abort->setFixedHeight(18);
            btn_abort->setFixedWidth(18);
            ui->statusBar->addPermanentWidget(progbar);
            ui->statusBar->addPermanentWidget(btn_abort);
            timer = new QTimer(this);
            timer->setInterval(30000);
            connect(timer, SIGNAL(timeout()), this, SLOT(timerClick("writing")));

            ui->copyNibwrite->setEnabled(false);
            QFileInfo file(fileToCopy);
            ui->statusBar->showMessage("Writing: "+file.baseName()+"."+file.completeSuffix());
            d64imageFile = file.baseName()+"."+file.completeSuffix();

            proc_nibwrite->start(nibwrite, QStringList() << params << QDir::toNativeSeparators(fileToCopy), QIODevice::ReadWrite | QIODevice::Text);
            if (!proc_nibwrite->waitForStarted())
            {
                QMessageBox::warning(this,"Error", "Failed to execute "+nibwrite+"\n\nExit status: "+QString::number(proc_nibwrite->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
                ui->statusBar->removeWidget(progbar);
                ui->statusBar->removeWidget(btn_abort);
                delete progbar;
                delete btn_abort;
            } else
            {
                timer->start();
                disableUIElements();
                currBlock = 0;
                lastBlock = 0;
            }
        }
    } else
    {
        QMessageBox::information(this, "QtCBM", fileinfo.completeSuffix().toUpper()+" is not a valid filetype to transfer", QMessageBox::Ok, QMessageBox::Ok);
    }
}


void FileWindow::on_copyNibread_clicked()
{
//    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Disk Image"), QDir::rootPath(), tr("Disk Images (*.d64)"));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Disk Image"), selectedLocalFolder , tr("NIB/NB2/NBZ Images (*.nib *.nb2 *.nbz)"));

    tracks="";
    moretracks="";
    QStringList params;

    params << "-D" + QString::number(deviceid);

    if (usetracks)
    {
        tracks = "-S"+QString::number(starttrack);
        moretracks =  "-E"+QString::number(endtrack);
        params << tracks << moretracks;
    }

    if (readRetry)
    {
        params << "-e"+QString::number(retryErrors);
    }

    if (parTransfer1571)
    {
        params << "-P";
    }

    if (noKillertracks)
    {
        params << "-k";
    }

    if (defaultDensities)
    {
        params << "-d";
    }

    if (readHalftracks)
    {
        params << "-h";
    }

    if (trackMatching)
    {
        params << "-v";
    }

    if (beVerbose)
    {
        params << "-V";
    }

    qDebug() << "nibread params:" << params;

    if (!fileName.isEmpty() && confirmExecute(nibread, QStringList() << params << QDir::toNativeSeparators(fileName)))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(100);
        progbar->setTextVisible(true);
        btn_abort = new QPushButton(this);
        connect(btn_abort, SIGNAL(clicked()), this, SLOT(stopCopy()));
        btn_abort->setText("X");
        btn_abort->setToolTip("Abort the current transfer and reset the CBM bus");
        btn_abort->setFixedHeight(18);
        btn_abort->setFixedWidth(18);
        ui->statusBar->addPermanentWidget(progbar);
        ui->statusBar->addPermanentWidget(btn_abort);
        timer = new QTimer(this);
        timer->setInterval(30000);
        connect(timer, SIGNAL(timeout()), this, SLOT(timerClick("reading")));
        ui->copyNibread->setEnabled(false);
        QFileInfo file(fileName);
        ui->statusBar->showMessage("Reading from CBM: "+file.baseName()+"."+file.completeSuffix());
        d64imageFile = file.baseName()+"."+file.completeSuffix();

        proc_nibread->start(nibread, QStringList() << params << QDir::toNativeSeparators(fileName), QIODevice::ReadWrite | QIODevice::Text);

        if (!proc_nibread->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+nibread+"\n\nExit status: "+QString::number(proc_nibread->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            ui->statusBar->removeWidget(btn_abort);
            delete progbar;
            delete btn_abort;
        } else
        {
            timer->start();
            disableUIElements();
            currBlock = 0;
            lastBlock = 0;
        }
    }
}

void FileWindow::stopCopy()
{
    timer->stop();
    proc_d64copy->kill(); // this will call cbmCopyFinished(), so we don't need to manually
    ui->copyToCBM->setEnabled(true);
    QMessageBox::information(this, "Copy Aborted", "The copy operation was terminated", QMessageBox::Ok, QMessageBox::Ok);
}

void FileWindow::stopNibwrite()
{
    timer->stop();
    proc_nibwrite->kill(); // this will call cbmNibwriteFinished(), so we don't need to manually
    ui->copyNibwrite->setEnabled(true);
    QMessageBox::information(this, "Nibwrite Copy Aborted", "The copy operation was terminated", QMessageBox::Ok, QMessageBox::Ok);
}

void FileWindow::stopNibread()
{
    timer->stop();
    proc_nibread->kill(); // this will call cbmNibreadFinished(), so we don't need to manually
    ui->copyNibread->setEnabled(true);
    QMessageBox::information(this, "Nibread Copy Aborted", "The copy operation was terminated", QMessageBox::Ok, QMessageBox::Ok);
}

void FileWindow::cbmCopyFinished(int x, QProcess::ExitStatus status)
{
    timer->stop();
    enableUIElements();
    qDebug() << "Process terminated with exit code: "+QString::number(x)+" and exit status: "+QString::number(status);
    (void)x;
    (void)status;

    ui->statusBar->removeWidget(btn_abort);
    ui->statusBar->removeWidget(progbar);
    delete btn_abort;
    delete progbar;
    ui->copyToCBM->setEnabled(true);
    if (status == QProcess::NormalExit && autorefresh)
    {
        on_CBMDirectory_clicked();
    } else if (status == QProcess::CrashExit)
    {
        ui->statusBar->clearMessage();
#ifdef Q_OS_WIN
        Sleep(2);
#endif
        on_actionReset_Bus_triggered();
    }
}

void FileWindow::cbmNibwriteFinished(int x, QProcess::ExitStatus status)
{

    timer->stop();
    enableUIElements();
    qDebug() << "Process terminated with exit code: "+QString::number(x)+" and exit status: "+QString::number(status);

    //QString output = proc_nibwrite->readAllStandardOutput();
    detailsInfoDialog *dlg = new detailsInfoDialog(this);
    dlg->setText("The nibwrite command produced the following output:");
    dlg->setDetailText(output2);
    dlg->exec();

    (void)x;
    (void)status;

    ui->statusBar->removeWidget(btn_abort);
    ui->statusBar->removeWidget(progbar);
    delete btn_abort;
    delete progbar;
    ui->copyNibwrite->setEnabled(true);
    if (status == QProcess::NormalExit && autorefresh)
    {
        on_CBMDirectory_clicked();
    } else if (status == QProcess::CrashExit)
    {
        ui->statusBar->clearMessage();
#ifdef Q_OS_WIN
        Sleep(2);
#endif
        on_actionReset_Bus_triggered();
    }
}

void FileWindow::cbmNibreadFinished(int x, QProcess::ExitStatus status)
{
    timer->stop();
    enableUIElements();
    qDebug() << "Process terminated with exit code: "+QString::number(x)+" and exit status: "+QString::number(status);

    //QString output = proc_nibread->readAllStandardOutput();
    detailsInfoDialog *dlg = new detailsInfoDialog(this);
    dlg->setText("The nibread command produced the following output:");
    dlg->setDetailText(output2);
    dlg->exec();


    (void)x;
    (void)status;

    ui->statusBar->removeWidget(btn_abort);
    ui->statusBar->removeWidget(progbar);
    delete btn_abort;
    delete progbar;
    ui->copyNibread->setEnabled(true);
    if (status == QProcess::NormalExit && autorefresh)
    {
        on_CBMDirectory_clicked();
    } else if (status == QProcess::CrashExit)
    {
        ui->statusBar->clearMessage();
#ifdef Q_OS_WIN
        Sleep(2);
#endif
        on_actionReset_Bus_triggered();
    }
}

void FileWindow::cbmCopyProgress()
{
    QString output = proc_d64copy->readAllStandardOutput();
    //
    QRegExp rx("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*(\\d+)%\\s*(\\d+)\\/(\\d+).*");
    QRegExp rxTrackChange("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*");
    QRegExp rxDone(".*(\\d+ blocks copied)\\.");
    if (rx.indexIn(output) >= 0)
    {
        progbar->setValue(rx.cap(3).toInt());
        currBlock = rx.cap(4).toInt();

        // qDebug() << "currBlock" << currBlock;

#ifdef Q_OS_LINUX
        progbar->setFormat("Track: "+rx.cap(1)+" Block: "+rx.cap(4)+"/"+rx.cap(5));
#endif
#ifdef Q_OS_WIN
        progbar->setFormat("Track: "+rx.cap(1)+" Block: "+rx.cap(4)+"/"+rx.cap(5));
#endif
#ifdef Q_OS_MAC
        ui->statusBar->showMessage("Track: "+rx.cap(1)+", Block: "+rx.cap(4)+"/"+rx.cap(5));
#endif
    } else if (rxTrackChange.indexIn(output) >= 0)
    {
        //qDebug() << "Next track detected";
    } else if (rxDone.indexIn(output) >= 0)
    {
        ui->statusBar->showMessage(rxDone.cap(1));
    }
}

void FileWindow::cbmNibwriteProgress()
{
    qDebug() << "BIN DA! ";
    QString output = proc_nibwrite->readAllStandardOutput();
    output2 = output2+output;
    //QRegExp rx("\\s?(\\d+).(\\d):\\s*([\\*\\.\\-\\?]+).*");
    QRegExp rx("\\s?(\\d+).(\\d+):\\s*\(\\d+:\\d+\\)\\s*WRITE.*");
    //QRegExp rx("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*(\\d+)%\\s*(\\d+)\\/(\\d+).*");
    //QRegExp rxTrackChange("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*");
    QRegExp rxTrackChange("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*");
    //QRegExp rxDone(".*(\\d+ blocks copied)\\.");
    if (rx.indexIn(output) >= 0)
    {
        progbar->setValue(rx.cap(1).toInt());
        qDebug() << "test: " << rx.cap(1);
        //currBlock = rx.cap(4).toInt();

#ifdef Q_OS_LINUX
        progbar->setFormat("Track: "+rx.cap(1));
#endif
#ifdef Q_OS_WIN
        progbar->setFormat("Track: "+rx.cap(1)+" Block: "+rx.cap(4)+"/"+rx.cap(5));
#endif
#ifdef Q_OS_MAC
        ui->statusBar->showMessage("Track: "+rx.cap(1)+", Block: "+rx.cap(4)+"/"+rx.cap(5));
#endif
    } else if (rxTrackChange.indexIn(output) >= 0)
    {
        qDebug() << "Next track detected";
    }

    qDebug() << "hm: " << output;

    /* else if (rxDone.indexIn(output) >= 0)
    {
        ui->statusBar->showMessage(rxDone.cap(1));
    }
    */
}

void FileWindow::cbmNibreadProgress()
{
    qDebug() << "BIN DA! ";
    QString output = proc_nibread->readAllStandardOutput();
    output2 = output2+output;
    //QString output = "";
    //QRegExp rx("\\s?(\\d+).(\\d):\\s*([\\*\\.\\-\\?]+).*");
    QRegExp rx("\\s?(\\d+).(\\d+):\\s*\(\\d+:\\d+\\)\\s*WRITE.*");
    //QRegExp rx("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*(\\d+)%\\s*(\\d+)\\/(\\d+).*");
    //QRegExp rxTrackChange("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*");
    QRegExp rxTrackChange("\\s?(\\d+):\\s*([\\*\\.\\-\\?]+)\\s*");
    //QRegExp rxDone(".*(\\d+ blocks copied)\\.");
    if (rx.indexIn(output) >= 0)
    {
        progbar->setValue(rx.cap(1).toInt());
        qDebug() << "test: " << rx.cap(1);
        //currBlock = rx.cap(4).toInt();

#ifdef Q_OS_LINUX
        progbar->setFormat("Track: "+rx.cap(1));
#endif
#ifdef Q_OS_WIN
        progbar->setFormat("Track: "+rx.cap(1)+" Block: "+rx.cap(4)+"/"+rx.cap(5));
#endif
#ifdef Q_OS_MAC
        ui->statusBar->showMessage("Track: "+rx.cap(1)+", Block: "+rx.cap(4)+"/"+rx.cap(5));
#endif
    } else if (rxTrackChange.indexIn(output) >= 0)
    {
        qDebug() << "Next track detected";
    }

    qDebug() << "hm: " << output;

    /* else if (rxDone.indexIn(output) >= 0)
    {
        ui->statusBar->showMessage(rxDone.cap(1));
    }
    */
}

void FileWindow::cbmResetFinished(int,QProcess::ExitStatus)
{
    resetUI();

    ui->statusBar->showMessage("The CBM bus was reset");
}

void FileWindow::cbmDetectFinished(int, QProcess::ExitStatus)
{
    resetUI();

    QString output = proc_cbmDetect->readAllStandardOutput().trimmed();

    QRegExp rx("(\\d+):\\s*(.*)");
    if (rx.indexIn(output) >= 0)
    {
        QMessageBox::information(this, "QtCBM", "Found drive id "+rx.cap(1)+": "+rx.cap(2), QMessageBox::Ok, QMessageBox::Ok);
    } else
    {
        QMessageBox::warning(this, "QtCBM", "No drives found.", QMessageBox::Ok, QMessageBox::Ok);
    }
}

void FileWindow::cbmInitFinished(int, QProcess::ExitStatus)
{
    ui->statusBar->showMessage("Initialising...");

    just_CBMStatus();

    resetUI();

}

void FileWindow::cbmValidateFinished(int, QProcess::ExitStatus)
{
    ui->statusBar->showMessage("Validating...");

    just_CBMStatus();

    resetUI();

}

void FileWindow::cbmFormatFinished(int, QProcess::ExitStatus)
{
    QString output = proc_cbmFormat->readAllStandardOutput();

    resetUI();

    detailsInfoDialog *dlg = new detailsInfoDialog(this);
    dlg->setText("The format command produced the following output:");
    dlg->setDetailText(output);
    dlg->exec();
}

void FileWindow::cbmDirFinished(int, QProcess::ExitStatus)
{
    resetUI();

    // clear the file list
    ui->cbmFiles->clear();

    ui->freeSpace->setText("");
    ui->diskLabel->setText("");
    ui->diskId->setText("");

    QList<QByteArray> dirlist = proc_cbmDir->readAllStandardOutput().split('\n');
    qDebug() << "dirlist: " << dirlist;

        //QRegExp rxLabel("(0|1)\\s.\"(.*)\"\\s*(\\S+)");
    QRegExp rxLabel("(0|1)\\s.\"(.*)\"\\s*(.*)");
    QRegExp rxDirEntry("(\\d+)\\s*.\"(.*)\"\\s+(\\S\\S\\S)");
    QRegExp rxFreeSpace("(\\d+)\\s\\S\\S\\S\\S\\S\\S\\s\\S\\S\\S\\S");
    QRegExp rxStatus("(\\d\\d),(.*),(\\d\\d),(\\d\\d)");

    // if directory starts with an error output:
    if (rxStatus.indexIn(QString(dirlist.at(0))) >= 0)
    {
        ui->statusBar->showMessage("Drive status: "+dirlist.at(0).toUpper());
    } else if(rxLabel.indexIn(QString(dirlist.at(0))) >= 0)
        {
    // else we grab the first entry for the header info
        QByteArray rawname = dirlist.at(0).mid(4,16);
        rawname = rawname.mid(0, rawname.indexOf('"'));
        ui->diskLabel->setText(CBMroutines::stringToPETSCII(rawname, cbmctrlhasraw));

        rawname = dirlist.at(0).mid(21,25);
        rawname = rawname.mid(0, rawname.indexOf('"'));
        ui->diskId->setText(CBMroutines::stringToPETSCII(rawname.trimmed(), cbmctrlhasraw));

    // more than 3 entries means we have dir entries

    qDebug() << "dircnt:" << dirlist.count();

        if (dirlist.count() > 3)
        {
            for (int i = 1; i < dirlist.count(); i++)
            {


                QByteArray rawname = dirlist.at(i).mid(6,16);
                rawname = rawname.mid(0, rawname.indexOf('"'));

                QString regstring = CBMroutines::stringToPETSCII(dirlist.at(i), true, cbmctrlhasraw);

                if (rxStatus.indexIn(QString(dirlist.at(i))) >= 0)
                {
                    ui->statusBar->showMessage("Drive status: "+dirlist.at(i).toUpper());
                } else if (rxDirEntry.indexIn(regstring) >= 0)
                {
                    //qDebug() << "filename: " << regstring;
                    QTreeWidgetItem *item = new QTreeWidgetItem();
                    item->setText(0, CBMroutines::stringToPETSCII(rxDirEntry.cap(1).toLocal8Bit(), false, cbmctrlhasraw));
                    item->setText(1, CBMroutines::stringToPETSCII(QString(rxDirEntry.cap(2)), cbmctrlhasraw));
                    item->setText(2, rxDirEntry.cap(3).toUpper());
                    item->setText(3, CBMroutines::stringToPETSCII(CBMroutines::formatFileSize(rxDirEntry.cap(1).toInt()*254), cbmctrlhasraw));
                    item->setData(4, Qt::DisplayRole, rawname);
                    ui->cbmFiles->addTopLevelItem(item);
                } else if (rxFreeSpace.indexIn(QString(dirlist.at(i))) >= 0)
                {
                    //qDebug() << "freespace: " << dirlist.at(i);
                    QString tmp_fs = QString(rxFreeSpace.cap(1)+" blocks free ("+CBMroutines::formatFileSize(rxFreeSpace.cap(1).toInt()*254)+") ");
                    ui->freeSpace->setText(CBMroutines::stringToPETSCII(tmp_fs.toLatin1(), false, cbmctrlhasraw));
                }

            //qDebug() << "dline : " << dirlist.at(i);
            //qDebug() << "status: " << rxStatus.indexIn(QString(dirlist.at(i)));
            //qDebug() << "freesp: " << rxFreeSpace.indexIn(QString(dirlist.at(i)));

            } // less than 3 entries means we have an "empty" disk - just blocks free
        } else for (int i = 0; i < dirlist.count(); i++)
            {
                if (rxStatus.indexIn(QString(dirlist.at(i))) >= 0)
                {
                    ui->statusBar->showMessage("Drive status: "+dirlist.at(i).toUpper());
                } else if (rxFreeSpace.indexIn(QString(dirlist.at(i))) >= 0)
                {
                    //qDebug() << "freespace: " << dirlist.at(i);
                    QString tmp_fs = QString(rxFreeSpace.cap(1)+" blocks free ("+CBMroutines::formatFileSize(rxFreeSpace.cap(1).toInt()*254)+") ");
                    ui->freeSpace->setText(CBMroutines::stringToPETSCII(tmp_fs.toLatin1(), false, cbmctrlhasraw));
                }
            }
}

    // almost done
    //qDebug() << "ui-freespace: " << ui->freeSpace->text();

    if (ui->freeSpace->text() == "")
       {
            QString pars = QString("Parsing blocks free failed - unusual directory?");
            ui->freeSpace->setText(CBMroutines::stringToPETSCII(pars.toLatin1(), false, cbmctrlhasraw));
       }

    ui->cbmFiles->resizeColumnToContents(0);
    ui->cbmFiles->resizeColumnToContents(1);
    ui->cbmFiles->resizeColumnToContents(2);
}

void FileWindow::on_CBMDirectory_clicked()
{
    QStringList params;

    if (cbmctrlhasraw)
        params << "--raw";

    params << "dir" << QString::number(deviceid);

    if(confirmExecute(cbmctrl, QStringList() << params))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_cbmDir->start(cbmctrl, QStringList() << params, QIODevice::ReadWrite | QIODevice::Text);
        if (!proc_cbmDir->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmDir->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }
    }
}

void FileWindow::on_CBMFormat_clicked()
{
    QString diskLabel = "";
    QStringList params;
    bool ok;

    if (generateRandomDiskname)
        diskLabel = CBMroutines::randomFullString(12)+","+CBMroutines::randomFullString(2);

    if (formatNobump)
        params << "-n";

    if (formatExtended)
        params << "-x";

    if (formatVerify)
        params << "-v";

    if (formatOriginal)
        params << "-o";

    params << "--";

    if (QMessageBox::question(this, "QtCBM", "This will erase ALL data on the floppy disk. Continue?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        diskLabel = QInputDialog::getText(this, "QtCBM", "Diskname,ID:", QLineEdit::Normal, diskLabel, &ok).toUpper();

        QRegExp rx("[\\s\\S]+,[\\s\\S]+");
        if (ok)
        {
            if (rx.indexIn(diskLabel) >= 0)
            {
                if (confirmExecute(cbmforng, QStringList() << params << QString::number(deviceid) << diskLabel))
                {
                    qDebug() << "format cmd:" << cbmforng << params << QString::number(deviceid) << diskLabel;

                    progbar = new QProgressBar(this);
                    progbar->setMinimum(0);
                    progbar->setMaximum(0);
                    ui->statusBar->addPermanentWidget(progbar);

                    proc_cbmFormat->start(cbmforng, QStringList() << params << QString::number(deviceid) << diskLabel);
                    if (!proc_cbmFormat->waitForStarted())
                    {
                        QMessageBox::warning(this,"Error", "Failed to execute "+cbmforng+"\n\nExit status: "+QString::number(proc_cbmFormat->exitCode()), QMessageBox::Ok, QMessageBox::Ok);
                        ui->statusBar->removeWidget(progbar);
                        delete progbar;
                    } else
                    {
                        disableUIElements();
                    }
                }
            } else
            {
                QMessageBox::warning(this, "QtCBM", "Your input wasn't of the form \"Label,ID\". Unable to format with this input.");
            }
        }
    }
}

void FileWindow::on_CBMInitialize_clicked()
{
    if (confirmExecute(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "IO"))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_cbmInit->start(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "IO");
        if (!proc_cbmInit->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmInit->exitCode()), QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }
    }
}

void FileWindow::on_CBMValidate_clicked()
{
    if (confirmExecute(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "V0:"))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_cbmValidate->start(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "V0:");
        if (!proc_cbmValidate->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmValidate->exitCode()), QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }



    }
}

void FileWindow::on_copyFileFromCBMdisk_clicked()
{
    QList<QTreeWidgetItem *> list = ui->cbmFiles->selectedItems();

    if (list.count() < 1)
    {
        QMessageBox::warning(this, "QtCBM", "No files selected.", QMessageBox::Ok, QMessageBox::Ok);
        return;
    } else if (list.count() == 1)
    {
        if (!selectedLocalFolder.isEmpty())
        {
            //QString cbmFileName = list.at(0)->text(4); //+"."+list.at(0)->text(3);
            cbmFileName = CBMroutines::ptoa((unsigned char *)list.at(0)->data(4,0).toByteArray().data());

            //qDebug() << "file: " << cbmFileName:
            //qDebug() << "raw: " << regstring;

            if(confirmExecute(cbmcopy, QStringList() << "--transfer="+transfermode << "-q" << "-r" << "--" << QString::number(deviceid) << "\"" << cbmFileName << "\""))
            {
                progbar = new QProgressBar(this);
                progbar->setMinimum(0);
                progbar->setMaximum(0);
                progbar->setTextVisible(true);
                ui->statusBar->addPermanentWidget(progbar);
                ui->copyFromCBM->setEnabled(false);
                ui->statusBar->showMessage("Reading: "+cbmFileName);

                proc_cbmcopy->setWorkingDirectory(selectedLocalFolder);
                proc_cbmcopy->start(cbmcopy, QStringList() << "--transfer="+transfermode << "-q" << "-r" << "--" << QString::number(deviceid) << "\"" << cbmFileName << "\"", QIODevice::ReadWrite | QIODevice::Text);
                if (!proc_cbmcopy->waitForStarted())
                {
                    QMessageBox::warning(this,"Error", "Failed to execute "+cbmcopy+"\n\nExit status: "+QString::number(proc_cbmcopy->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
                    ui->statusBar->removeWidget(progbar);
                    ui->statusBar->removeWidget(btn_abort);
                    delete progbar;
                    delete btn_abort;
                } else
                {
                    disableUIElements();
                }
            }
        } else
        {
            QMessageBox::warning(this, "QtCBM", "No local folder selected. Select where I should save the file.", QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
    } else
    {
        QMessageBox::warning(this, "QtCBM", "Can only copy one file off disk at a time.", QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
}

void FileWindow::on_copyFromCBM_clicked()
{
//    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Disk Image"), QDir::rootPath(), tr("Disk Images (*.d64)"));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Disk Image"), selectedLocalFolder , tr("D64/D71 Images (*.d64 *.d71)"));

    QStringList params;
    tracks="";
    moretracks="";
    appenderrormap="";

    if (appendmap)
    {
        appenderrormap="--error-map=always";
        params << appenderrormap;
    }

    if (usetracks)
    {
        tracks = "--start-track="+QString::number(starttrack);
        moretracks =  "--end-track="+QString::number(endtrack);
        params << tracks << moretracks;
    }

    if (readRetry)
    {
        params << "-r"+QString::number(retryErrors);
    }

    //qDebug() << "params: "+params;

    if (!fileName.isEmpty() && confirmExecute(d64copy, QStringList() << params << QString::number(deviceid) << QDir::toNativeSeparators(fileName)))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(100);
        progbar->setTextVisible(true);
        btn_abort = new QPushButton(this);
        connect(btn_abort, SIGNAL(clicked()), this, SLOT(stopCopy()));
        btn_abort->setText("X");
        btn_abort->setToolTip("Abort the current transfer and reset the CBM bus");
        btn_abort->setFixedHeight(18);
        btn_abort->setFixedWidth(18);
        ui->statusBar->addPermanentWidget(progbar);
        ui->statusBar->addPermanentWidget(btn_abort);
        timer = new QTimer(this);
        timer->setInterval(10000);
        connect(timer, SIGNAL(timeout()), this, SLOT(timerClick("reading")));
        ui->copyToCBM->setEnabled(false);
        QFileInfo file(fileName);
        ui->statusBar->showMessage("Reading from CBM: "+file.baseName()+"."+file.completeSuffix());
        d64imageFile = file.baseName()+"."+file.completeSuffix();

        proc_d64copy->start(d64copy, QStringList() << "--transfer="+transfermode << params << QString::number(deviceid) << QDir::toNativeSeparators(fileName), QIODevice::ReadWrite | QIODevice::Text);

        if (!proc_d64copy->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+d64copy+"\n\nExit status: "+QString::number(proc_d64copy->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            ui->statusBar->removeWidget(btn_abort);
            delete progbar;
            delete btn_abort;
        } else
        {
            timer->start();
            disableUIElements();
            currBlock = 0;
            lastBlock = 0;
        }
    }
}

void FileWindow::cbmRenameFinished(int, QProcess::ExitStatus)
{
    resetUI();

    ui->statusBar->showMessage("File renamed");
    if (autorefresh)
        on_CBMDirectory_clicked();
}

void FileWindow::cbmScratchFinished(int, QProcess::ExitStatus)
{
    resetUI();

    ui->statusBar->showMessage("File erased");
    if (autorefresh)
        on_CBMDirectory_clicked();
}

QByteArray FileWindow::getSelectedCBMFile(QString message)
{
    QList<QTreeWidgetItem *> items = ui->cbmFiles->selectedItems();
    if (items.count() != 1)
    {
        QMessageBox::warning(this, "QtCBM", message, QMessageBox::Ok, QMessageBox::Ok);
        return "";
    }
    return items.at(0)->data(4,0).toByteArray();
}

QString subWildCard(QString in)
{
    QString output;

    for (int i = 0; i < in.length(); i++)
    {
        if (in.at(i) > 127)
        {
            output.append('?');
        } else
        {
            output.append(in.at(i));
        }
    }
    return output;
}

void FileWindow::on_CBMScratch_clicked()
{
    QByteArray cbmFilename = getSelectedCBMFile("Please select one item to delete.");
    //QByteArray cbmFilename = "2E??????????????";
    if (cbmFilename.isEmpty())
        return;

    if (QMessageBox::question(this, "QtCBM", "Are you sure you want to delete the file \""+cbmFilename+"\" from the current disk?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {

        if (confirmExecute(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "S0:"+cbmFilename))
        {
            progbar = new QProgressBar(this);
            progbar->setMinimum(0);
            progbar->setMaximum(0);
            ui->statusBar->addPermanentWidget(progbar);

            proc_cbmScratch->start(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "S0:"+cbmFilename);
            if (!proc_cbmScratch->waitForStarted())
            {
                QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmScratch->exitCode()), QMessageBox::Ok, QMessageBox::Ok);
                ui->statusBar->removeWidget(progbar);
                delete progbar;
            } else
            {
                disableUIElements();
            }
        }
    }
}

void FileWindow::on_CBMRename_clicked()
{
    bool ok = false;

    QString cbmFilename = getSelectedCBMFile("Please select a file to rename.");
    if (cbmFilename.isEmpty())
        return;

    QString newFilename = QInputDialog::getText(this, "QtCBM", "New filename:", QLineEdit::Normal, "", &ok).toUpper();
    if (ok && confirmExecute(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "R0:"+newFilename+"="+cbmFilename))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_cbmRename->start(cbmctrl, QStringList() << "command" << QString::number(deviceid) << "R0:"+newFilename+"="+cbmFilename);
        if (!proc_cbmRename->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmRename->exitCode()), QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }
    }
}

void FileWindow::on_actionReset_Bus_triggered()
{
    if (confirmExecute(cbmctrl, QStringList() << "reset"))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_cbmReset->start(cbmctrl, QStringList() << "reset", QIODevice::ReadWrite | QIODevice::Text);
        if (!proc_cbmReset->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmReset->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }
    }
}

void FileWindow::on_actionDetect_Drive_triggered()
{
    if (confirmExecute(cbmctrl, QStringList() << "detect"))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_cbmDetect->start(cbmctrl, QStringList() << "detect", QIODevice::ReadWrite | QIODevice::Text);
        if (!proc_cbmDetect->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+cbmctrl+"\n\nExit status: "+QString::number(proc_cbmDetect->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }
    }
}

/*
void FileWindow::on_actionMorse_Code_triggered()
{
    if (confirmExecute(morse, QStringList() << "8"))
    {
        progbar = new QProgressBar(this);
        progbar->setMinimum(0);
        progbar->setMaximum(0);
        ui->statusBar->addPermanentWidget(progbar);

        proc_morse->start(morse, QStringList() << "8", QIODevice::ReadWrite | QIODevice::Text);
        if (!proc_morse->waitForStarted())
        {
            QMessageBox::warning(this,"Error", "Failed to execute "+morse+"\n\nExit status: "+QString::number(proc_morse->exitCode()),QMessageBox::Ok, QMessageBox::Ok);
            ui->statusBar->removeWidget(progbar);
            delete progbar;
        } else
        {
            disableUIElements();
        }
    }
}
*/

