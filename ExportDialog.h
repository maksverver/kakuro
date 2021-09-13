#ifndef EXPORTDIALOG_H_INCLUDED
#define EXPORTDIALOG_H_INCLUDED

#include "Puzzle.h"
#include "ui_ExportDialog.h"

#include <memory>

class ExportDialog : public QDialog, protected Ui::ExportDialog
{
    Q_OBJECT

public:
    ExportDialog(std::unique_ptr<Puzzle> puzzle, QWidget *parent = 0);

protected slots:
    void updateText();
    void copyTextToClipboard();

protected:
    std::unique_ptr<Puzzle> puzzle;
};

#endif /* ndef MAINWINDOW_H_INCLUDED */

