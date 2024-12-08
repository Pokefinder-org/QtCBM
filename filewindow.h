#ifndef FILEWINDOW_H
#define FILEWINDOW_H

#include <QFileSystemModel>
#include <QFontDatabase>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMainWindow>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QTimer>

namespace Ui {
class FileWindow;
}

class FileWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FileWindow(QWidget *parent = 0);

    /*!
     * \brief confirmExecute Displays the command and params to be executed and prompts for confirmation
     * Automatically returns true if showcmd is false
     * \param command The external command to be executed
     * \param params The list of parameters that we will pass to it
     * \return whether the user agreed to execute or not
     */
    bool confirmExecute(QString command, QStringList params);

    /*!
     * \brief formatFileSize return a string which converts a number in bytes to an
     * equivalent value in KB, MB, or GB
     * \return a QString with the converted value
     */
    //QString formatFileSize(qint64);
    //QString stringToPETSCII(QByteArray, bool);
    //QString stringToPETSCII(QString);
    void writeD64FromArgs(QString);
    void copyToCBM(QStringList list);
    void copyNibwrite(QStringList list);
    void copyNibread(QStringList list);
    void writeCBMconf();
    void disableUIElements();
    void enableUIElements();

    /*!
     * \brief getSelectedCBMFile
     * \return
     */
    QByteArray getSelectedCBMFile(QString);
    void resetUI();
    ~FileWindow();  

private slots:
    void loadSettings();
    void stopCopy();
    void stopNibwrite();
    void stopNibread();
    void timerClick(QString);

    void act_newFolder();
    QFile * act_renameFile();
    void act_deleteFile();
    void act_viewFile();
    void act_viewD64();

    void selectedD64contents(QByteArray);

    void cbmStatusFinished(int,QProcess::ExitStatus);
    void cbmJustStatusFinished(int,QProcess::ExitStatus);
    void cbmCopyProgress();
    void cbmNibwriteProgress();
    void cbmNibreadProgress();
    void cbmCopyFinished(int,QProcess::ExitStatus);
    void cbmNibwriteFinished(int,QProcess::ExitStatus);
    void cbmNibreadFinished(int,QProcess::ExitStatus);
    void cbmDirFinished(int, QProcess::ExitStatus);
    void cbmResetFinished(int, QProcess::ExitStatus);
    void cbmFormatFinished(int, QProcess::ExitStatus);
    void cbmInitFinished(int, QProcess::ExitStatus);
    void cbmValidateFinished(int, QProcess::ExitStatus);
    void cbmScratchFinished(int ,QProcess::ExitStatus);
    void cbmRenameFinished(int ,QProcess::ExitStatus);
    void cbmDetectFinished(int, QProcess::ExitStatus);
//    void morseFinished(int, QProcess::ExitStatus);
    void cbmFileCopyFinished(int, QProcess::ExitStatus);

    void localFiles_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void on_localFolders_clicked(const QModelIndex &index);
    void on_localFiles_doubleClicked(const QModelIndex &index);
    void on_actionPreferences_triggered();
    void on_CBMStatus_clicked();
    void just_CBMStatus();
    void on_actionView_Drive_triggered();
    void on_actionView_Home_Folder_triggered();
    void on_copyToCBM_clicked();
    void on_CBMDirectory_clicked();
    void on_actionAbout_triggered();
    void on_CBMFormat_clicked();
    void on_CBMInitialize_clicked();
    void on_CBMValidate_clicked();
    void on_copyFromCBM_clicked();
    void on_CBMScratch_clicked();
    void on_CBMRename_clicked();
    void on_actionReset_Bus_triggered();
    void on_actionDetect_Drive_triggered();
//    void on_actionMorse_Code_triggered();
    void on_action_Quit_triggered();

    void on_copyCBMfileToDisk_clicked();

    void on_copyFileFromCBMdisk_clicked();

    void on_copyNibwrite_clicked();
    void on_copyNibread_clicked();

private:

    // Qt objects
    Ui::FileWindow *ui;
    QFileSystemModel *foldersModel;
    QFileSystemModel *filesModel;
    QFontDatabase *fontDB;
    QItemSelectionModel *selectModel;
    QSettings *settings;
    QProgressBar *progbar;
    QPushButton *btn_abort;
    QTimer *timer;

    // Process pointers
    QProcess *proc_cbmStatus;
    QProcess *proc_just_CBMStatus;
    QProcess *proc_d64copy;
    QProcess *proc_nibwrite;
    QProcess *proc_nibread;
    QProcess *proc_cbmDir;
    QProcess *proc_cbmReset;
    QProcess *proc_cbmFormat;
    QProcess *proc_cbmInit;
    QProcess *proc_cbmValidate;
    QProcess *proc_cbmScratch;
    QProcess *proc_cbmRename;
    QProcess *proc_cbmDetect;
//    QProcess *proc_morse;
    QProcess *proc_cbmcopy;

    // strings
    QString selectedLocalFolder;
    QString cbmctrl;
    QString cbmforng;
    QString d64copy;
    QString nibwrite;
    QString nibread;
    QString cbmcopy;
//    QString morse;
    QString transfermode;
    QString cableType;
    QString d64imageFile;
    QString fileFromArgs;
    QString cbmFileName;
    QString tracks;
    QString moretracks;
    QString appenderrormap;
    QString output;
    QString output2;
    QString rwstate;

    // other variables
    bool showcmd;
    bool autorefresh;
    bool cbmctrlhasraw;
    bool generateRandomDiskname;
    bool formatVerify;
    bool formatExtended;
    bool formatNobump;
    bool formatOriginal;
    bool appendmap;
    bool parTransfer1571;
    bool beVerbose;
    bool usetracks;
    bool readRetry;
    bool noKillertracks;
    bool defaultDensities;
    bool readHalftracks;
    bool trackMatching;
    int deviceid;
    int starttrack;
    int endtrack;
    int retryErrors;
    int lastBlock;
    int currBlock;
};

#endif // FILEWINDOW_H
