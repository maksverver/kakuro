#ifndef EXPORTDIALOG_H_INCLUDED
#define EXPORTDIALOG_H_INCLUDED

#include "Puzzle.h"
#include "ui_ExportDialog.h"

class ExportDialog : public QDialog, protected Ui::ExportDialog
{
    Q_OBJECT

public:
    ExportDialog(Puzzle *puzzle, QWidget *parent = 0);
    ~ExportDialog();

protected slots:
    void updateText();
    void copyTextToClipboard();

protected:
    Puzzle *puzzle;
};

#endif /* ndef MAINWINDOW_H_INCLUDED */

