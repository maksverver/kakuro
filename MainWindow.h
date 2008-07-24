#ifndef MAINWINDOW_H_INCLUDED
#define MAINWINDOW_H_INCLUDED

#include "Sheet.h"
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow, protected Ui::MainWindow
{
    Q_OBJECT

    friend class SwapSheetsCommand;
    friend class AddRemoveSheetCommand;


public:
    MainWindow();
    ~MainWindow();

    void openFiles(QStringList files);

protected:
    void closeEvent(QCloseEvent *event);

protected slots:
    void requestAbout();
    void requestExport();
    void requestImport();
    void requestNewFile();
    void requestOpenFile();
    void requestSaveFile();
    void requestSaveFileAs();
    void requestNewSheet();
    void requestRenameSheet();
    void requestRemoveSheet();
    void requestMoveSheetLeft();
    void requestMoveSheetRight();
    void requestSaveSheetAsImage();
    void requestPrint(double marginInch = 0.5, double cellSizeInch = 0.4);

    void crop();
    void enlarge();
    void clearDigits();
    void clearCandidates();
    void clearSums();
    void calculateCandidates();
    void calculateSums();
    void checkPuzzle();
    void solvePuzzle();
    void generatePuzzle();
    void updateMoveSheetButtons();

protected:
    Sheet *currentSheet();
    Sheet *sheetAt(int index);

    bool clean();
    bool confirmUnsavedChanges(const char *title);

    void newFile();
    bool saveFile(QString path);
    bool openFile(QString path, bool first = true);
    void removeAllSheets();

    void activateSheet(QUndoCommand *command);

    int last_sheet_id;
    QUndoStack undo_stack;
    QString file_path;
};

#endif /* ndef MAINWINDOW_H_INCLUDED */

