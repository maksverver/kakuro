#include "ExportDialog.h"
#include <QClipboard>

ExportDialog::ExportDialog(Puzzle *puzzle, QWidget *parent)
    : QDialog(parent), puzzle(puzzle)
{
    setupUi(this);
    connect(rbGrid,                 SIGNAL(toggled(bool)), this, SLOT(updateText()));
    connect(rbGridSums,             SIGNAL(toggled(bool)), this, SLOT(updateText()));
    connect(rbGridSumsDigits,       SIGNAL(toggled(bool)), this, SLOT(updateText()));
    connect(rbGridSumsCandidates,   SIGNAL(toggled(bool)), this, SLOT(updateText()));
    connect(buttonCopyToClipboard,  SIGNAL(clicked(bool)), this, SLOT(copyTextToClipboard()) );
    updateText();
}

ExportDialog::~ExportDialog()
{
    delete puzzle;
}

void ExportDialog::updateText()
{
    Puzzle::ExportOptions options = Puzzle::exportGrid;
    if(rbGridSums->isChecked())
        options = Puzzle::exportGridSums;
    if(rbGridSumsDigits->isChecked())
        options = Puzzle::exportGridSumsDigits;
    if(rbGridSumsCandidates->isChecked())
        options = Puzzle::exportGridSumsCandidates;

    std::string str = puzzle->toString(options);
    editText->setText(str.c_str());
}

void ExportDialog::copyTextToClipboard()
{
    QApplication::clipboard()->setText(editText->text());
}
