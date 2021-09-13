#include "MainWindow.h"
#include "ExportDialog.h"
#include "LineEditDialog.h"
#include "ui_AboutDialog.h"
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QTextStream>
#include <QPicture>
#include <QPrinter>
#include <QPrintDialog>
#include "io.h"

const QSize default_sheet_size(12, 12);

class RenameSheetCommand : public QUndoCommand
{
    QTabWidget &sheets;
    int index;
    QString old_text, new_text;

public:
    RenameSheetCommand(QTabWidget &sheets, int index, QString old_text, QString new_text)
        : sheets(sheets), index(index), old_text(old_text), new_text(new_text)
    {
    }

    void undo()
    {
        sheets.setTabText(index, old_text);
    }

    void redo()
    {
        sheets.setTabText(index, new_text);
    }
};


class SwapSheetsCommand : public QUndoCommand
{
    MainWindow &window;
    int i, j;

public:
    SwapSheetsCommand(MainWindow &window, int i, int j)
        : window(window), i(std::min(i, j)), j(std::max(i, j))
    {
    }

    void undo()
    {
        swap();
    }

    void redo()
    {
        swap();
    }

protected:

    // NOTE: only swaps widget and label, currently.
    void swap()
    {
        QTabWidget &sheets = *window.sheets;

        sheets.setUpdatesEnabled(false);
        int current = sheets.currentIndex();
        QWidget *wi = sheets.widget(i), *wj = sheets.widget(j);
        QString ti = sheets.tabText(i), tj = sheets.tabText(j);
        sheets.removeTab(j);
        sheets.removeTab(i);
        sheets.insertTab(i, wj, tj);
        sheets.insertTab(j, wi, ti);
        if(current == i)
            sheets.setCurrentIndex(j);
        else
        if(current == j)
            sheets.setCurrentIndex(i);
        sheets.setUpdatesEnabled(true);

        window.sheetAt(i)->setIndex(i);
        window.sheetAt(j)->setIndex(j);

        window.updateMoveSheetButtons();
    }
};

class AddRemoveSheetCommand : public QUndoCommand
{
    MainWindow      &window;
    QList<QWidget*> widget;
    QList<QString>  name;
    QList<int>      index;

public:

    AddRemoveSheetCommand(MainWindow &window, Sheet *sheet)
        : window(window)
    {
        QScrollArea *scrollArea = new QScrollArea;
        scrollArea->setWidget(sheet);
        scrollArea->setMinimumSize(QSize(40,40));
        scrollArea->setBackgroundRole(QPalette::Dark);

        if(sheet->name().isEmpty())
            sheet->setName(QObject::tr("Sheet %1").arg(++window.last_sheet_id));
        QObject::connect(sheet, SIGNAL(modified(int)), window.sheets, SLOT(setCurrentIndex(int)));

        widget += scrollArea;
        name   += sheet->name();
        index  += window.sheets->count();
    }

    AddRemoveSheetCommand(MainWindow &window, int sheetIndex)
        : window(window)
    {
        widget += 0;
        name   += QString();
        index  += sheetIndex;
    }

    ~AddRemoveSheetCommand()
    {
        for(int n = 0; n < widget.size(); ++n)
            if(widget[n])
                delete widget[n];
    }

    void undo()
    {
        for(int n = widget.size() - 1; n >= 0; --n)
            doit(widget[n], name[n], index[n]);
    }

    void redo()
    {
        for(int n = 0; n < widget.size(); ++n)
            doit(widget[n], name[n], index[n]);
    }

    void doit(QWidget *&widget, QString &name, int index)
    {
        QTabWidget &sheets = *window.sheets;

        if(widget)
        {
            // Add sheet
            sheets.insertTab(index, widget, name);
            sheets.setCurrentIndex(index);
            widget = 0;
        }
        else
        {
            // Remove sheet
            widget = sheets.widget(index);
            name   = sheets.tabText(index);
            sheets.removeTab(index);
        }
        for(int i = index; i < sheets.count(); ++i)
            window.sheetAt(i)->setIndex(index);
    }

    int id() const
    {
        return 1;
    }

    bool mergeWith(const QUndoCommand *other)
    {
        AddRemoveSheetCommand *arsc = const_cast<AddRemoveSheetCommand *> (
            dynamic_cast<const AddRemoveSheetCommand *>(other) );
        if(!arsc)
            return false;
        widget += arsc->widget;
        name   += arsc->name;
        index  += arsc->index;
        arsc->widget.clear();
        return true;
    }
};


MainWindow::MainWindow()
    : QMainWindow(0)
{
    setupUi(this);

    // Connect actions
#   define CONNECT(var, slot) connect(action##var, SIGNAL(triggered(bool)), SLOT(slot()))
    CONNECT( FileNew,               requestNewFile );
    CONNECT( FileOpen,              requestOpenFile );
    CONNECT( FileSave,              requestSaveFile );
    CONNECT( FileSaveAs,            requestSaveFileAs );
    CONNECT( Print,                 requestPrint );
    CONNECT( Quit,                  close );
    CONNECT( SheetNew,              requestNewSheet );
    CONNECT( SheetRename,           requestRenameSheet );
    CONNECT( SheetRemove,           requestRemoveSheet );
    CONNECT( Crop,                  crop );
    CONNECT( Enlarge,               enlarge );
    CONNECT( Import,                requestImport );
    CONNECT( Export,                requestExport );
    CONNECT( SheetSaveAsImage,      requestSaveSheetAsImage );
    CONNECT( CalculateCandidates,   calculateCandidates );
    CONNECT( CalculateSums,         calculateSums );
    CONNECT( ClearCandidates,       clearCandidates );
    CONNECT( ClearDigits,           clearDigits );
    CONNECT( ClearSums,             clearSums );
    CONNECT( CheckPuzzle,           checkPuzzle );
    CONNECT( SolvePuzzle,           solvePuzzle );
    CONNECT( GeneratePuzzle,        generatePuzzle );
    CONNECT( About,                 requestAbout );
#   undef CONNECT

    // Moving sheets
    connect(sheets, SIGNAL(currentChanged(int)), SLOT(updateMoveSheetButtons()));
    connect(actionSheetMoveLeft, SIGNAL(triggered(bool)), SLOT(requestMoveSheetLeft()));
    connect(actionSheetMoveRight, SIGNAL(triggered(bool)), SLOT(requestMoveSheetRight()));

    // Undo/redo
    connect(actionUndo, SIGNAL(triggered(bool)), &undo_stack, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered(bool)), &undo_stack, SLOT(redo()));
    connect(&undo_stack, SIGNAL(canUndoChanged(bool)), actionUndo, SLOT(setEnabled(bool)));
    connect(&undo_stack, SIGNAL(canRedoChanged(bool)), actionRedo, SLOT(setEnabled(bool)));

    newFile();
}

MainWindow::~MainWindow()
{
    removeAllSheets();
}

Sheet *MainWindow::currentSheet()
{
    return reinterpret_cast<Sheet*> (
        reinterpret_cast<QScrollArea*>(sheets->currentWidget())->widget() );
}

Sheet *MainWindow::sheetAt(int index)
{
    return reinterpret_cast<Sheet*> (
        reinterpret_cast<QScrollArea*>(sheets->widget(index))->widget() );
}

void MainWindow::newFile()
{
    removeAllSheets();
    last_sheet_id = 0;
    file_path = "";
    sheets->setUpdatesEnabled(false);
    undo_stack.push(new AddRemoveSheetCommand(*this,
        new Sheet(undo_stack, default_sheet_size)) );
    sheets->setUpdatesEnabled(true);
    undo_stack.clear();
}

// Removes all widgets AND clears the undo stack!
void MainWindow::removeAllSheets()
{
    int n = sheets->count();
    while(n--)
    {
        QWidget *widget = sheets->widget(n);
        sheets->removeTab(n);
        delete widget;
    }
}

void MainWindow::requestAbout()
{
    QDialog dlg(this);
    Ui::AboutDialog about;
    about.setupUi(&dlg);
    dlg.exec();
}

void MainWindow::requestExport()
{
    ExportDialog(currentSheet()->toPuzzle(), this).exec();
}

void MainWindow::requestImport()
{
    LineEditDialog dlg(this, tr("Import Sheet"), tr("Textual representation:"));
    if(dlg.exec() == QDialog::Accepted)
    {
        std::auto_ptr<Puzzle> puzzle(Puzzle::fromString(
            dlg.text().trimmed().toUtf8().constData()) );
        if(!puzzle.get())
        {
            QMessageBox::warning( this, tr("Invalid Data"),
                tr("The text you entered does not describe a Kakuro grid!") );
        }
        else
        {
            if(currentSheet()->empty())
                currentSheet()->fromPuzzle(*puzzle);
            else
                undo_stack.push(new AddRemoveSheetCommand(*this,
                    new Sheet(undo_stack, *puzzle)) );
        }
    }
}

bool MainWindow::confirmUnsavedChanges(const char *title)
{
    return undo_stack.isClean() ||
        QMessageBox::warning( this,
            tr(title),
            tr("You have made changes to this file that have not yet been saved!\n"
               "These changes which will be lost if you continue."),
            "&OK", "&Cancel", QString(), 1, 1 ) == 0;
}

void MainWindow::requestNewFile()
{
    if(!confirmUnsavedChanges("New File"))
        return;
    newFile();
}

void MainWindow::requestOpenFile()
{
    QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Open File"), getenv("HOME"), tr("Kakuro File (*.kkr);; All Files (*)") );
    if(paths.isEmpty())
        return;
    if(!confirmUnsavedChanges("Open File"))
        return;
    openFiles(paths);
}

void MainWindow::requestSaveFile()
{
    if(file_path.isEmpty())
        requestSaveFileAs();
    else
    if(!saveFile(file_path))
    {
        QMessageBox::warning(this, tr("Save File"), tr("File could not be saved!"));
        file_path.clear();
    }
}

void MainWindow::requestSaveFileAs()
{
    QFileDialog dlg(this, "Save File", getenv("HOME"), tr("Kakuro File (*.kkr);; All Files (*)"));
    dlg.setDefaultSuffix("kkr");
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setConfirmOverwrite(true);
    if(dlg.exec() == QDialog::Accepted)
    {
        QStringList list = dlg.selectedFiles();
        if(!list.empty())
        {
            file_path = list[0];
            requestSaveFile();
        }
    }
}

void MainWindow::requestNewSheet()
{
    undo_stack.push(new AddRemoveSheetCommand(*this,
        new Sheet(undo_stack, default_sheet_size)) );
}

void MainWindow::requestRenameSheet()
{
    LineEditDialog dlg( this, tr("Change Sheet Title"), tr("Sheet title:"),
                        sheets->tabText(sheets->currentIndex()) );
    if(dlg.exec() == QDialog::Accepted)
    {
        int index = sheets->currentIndex();
        undo_stack.push( new RenameSheetCommand(
            *sheets, index, sheets->tabText(index), dlg.text() ) );
    }
}

void MainWindow::requestRemoveSheet()
{
    undo_stack.push(new AddRemoveSheetCommand(*this, sheets->currentIndex()));
    if(sheets->count() == 0)
        undo_stack.push(new AddRemoveSheetCommand(*this,
            new Sheet(undo_stack, default_sheet_size)) );
}

void MainWindow::requestMoveSheetLeft()
{
    int index = sheets->currentIndex();
    if(index > 0 && index < sheets->count())
        undo_stack.push(new SwapSheetsCommand(*this, index, index - 1));
}

void MainWindow::requestMoveSheetRight()
{
    int index = sheets->currentIndex();
    if(index >= 0 && index < sheets->count() - 1)
        undo_stack.push(new SwapSheetsCommand(*this, index, index + 1));
}

void MainWindow::requestSaveSheetAsImage()
{
    // Display the save file dialog
    QList<const char*> formats;
    formats << "PNG" << "BMP" << "JPEG";

    QStringList filters;
    filters << tr("Portable Network Graphics (*.png)")
            << tr("Windows Bitmap (*.bmp)")
            << tr("Joint Photographic Experts Group (*.jpg *.jpeg)");

    QFileDialog dlg(this, tr("Save Sheet As Image"), getenv("HOME"));
    dlg.setNameFilters(filters);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setConfirmOverwrite(true);
    if(dlg.exec() != QDialog::Accepted || dlg.selectedFiles().size() != 1)
        return;
    QString path = dlg.selectedFiles()[0];

    const char *format = 0;
    for(int n = 0; n < filters.size() && n < formats.size(); ++n)
        if(dlg.selectedNameFilter() == filters[n])
        {
            format = formats[n];
            break;
        }
    if(!format)
        return;

    if(path.lastIndexOf('.') <= path.lastIndexOf('/') + 1)
    {
        path += '.';
        path += QString(format).toLower();
    }

    // Create the image
    QPainter painter;

    QPicture picture;
    painter.begin(&picture);
    currentSheet()->paint(painter);
    painter.end();

    QImage image(picture.boundingRect().size(), QImage::Format_RGB32);
    image.fill(QColor(Qt::lightGray).rgb());
    painter.begin(&image);
    painter.drawPicture(-picture.boundingRect().topLeft(), picture);
    painter.end();

    // Save the image
    image.save(path, format);
}

void MainWindow::requestPrint(double marginInch, double cellSizeInch)
{
    // FIXME: high resolution printing conflicts with loaded fonts!

    QPrinter printer(QPrinter::ScreenResolution);
    QPrintDialog print_dlg(&printer, this);

    // Extract selected page range
    if(print_dlg.exec() != QDialog::Accepted)
        return;
    int page_from = printer.fromPage(), page_to = printer.toPage();
    if(!page_from || !page_to)
        page_from = page_to = 0;
    else
    if(page_from > page_to)
        std::swap(page_from, page_to);

    // Compute the margin and scale used, in the printer's coordinate space
    const double margin = marginInch*printer.resolution();
    const double scale  = cellSizeInch*(printer.resolution()/double(Sheet::cell_size));
    const QRectF page(0, 0, printer.pageRect().width(), printer.pageRect().height());

    QPointF page_pos = page.bottomRight(), line_pos = page_pos;

    QPainter painter;
    if(!painter.begin(&printer))
    {
        QMessageBox::critical( this, tr("Printing Failed"),
            tr("Could not begin printing!") );
        return;
    }

    // Print all sheets
    int page_number = 0;
    for(int n = 0; n < sheets->count(); ++n)
    {
        QRect rect = sheetAt(n)->paintRect();
        if(line_pos.x() + rect.width()*scale > page.x() + page.width())
        {
            // Move to next line
            line_pos = page_pos;
        }

        if(page_pos.y() + rect.height()*scale > page.y() + page.height())
        {
            // Move to next page
            ++page_number;

            if(page_to && page_number > page_to)
                break;

            if(page_number > std::max(1, page_from))
            {
                // Eject current page
                if(!printer.newPage())
                {
                    QMessageBox::critical(this, tr("Printing Failed"), tr("Could not eject the current page!"));
                    return;
                }
            }

            page_pos = line_pos = page.topLeft();
        }

        if(page_number >= page_from)
        {
            // Draw the sheet at the right location, scaled to fit if necessary
            QPointF translation = line_pos - rect.topLeft();
            double s = std::min(double(scale), std::min( double(page.width())/rect.width(),
                                                        double(page.height())/rect.height() ));
            QMatrix transformation(s, 0, 0, s, translation.x(), translation.y());
            painter.setWorldMatrix(transformation);
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QBrush(Qt::lightGray));
            painter.drawRect(rect);
            painter.restore();
            sheetAt(n)->paint(painter);
        }

        // Update position
        line_pos.setX(line_pos.x() + rect.width()*scale + margin);
        page_pos.setY(std::max(page_pos.y(), line_pos.y() + rect.height()*scale + margin));
    }

    if(!painter.end())
    {
        QMessageBox::critical(this, tr("Printing Failed"), tr("Could not finish printing!"));
        return;
    }
}

/* Attempts to save file to 'path' and returns wether or not this succeeded.

   Note that the document filename is NOT updated! */
bool MainWindow::saveFile(QString path)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ))
        return false;
    QTextStream os(&file);
    os.setCodec("UTF-8");
    os << "%version kakuro 1 0\n";
    for(int n = 0; n < sheets->count(); ++n)
    {
        std::auto_ptr<Puzzle> puzzle(sheetAt(n)->toPuzzle());
        os << "\n"
           << puzzle->toString().c_str() << "\n"
           << "@title " << quoteWord(sheets->tabText(n)) << "\n";
    }
    undo_stack.setClean();
    return true;
}

/* Attempts to open a file at 'path'. If the file is improperly formatted
   or contains no sheets, returns false.

   Otherwise, the new sheets are added to the tab widget. If 'first' is true,
   all existing sheets are removed first, and the undo stack is cleared.

   Note that the document filename is NOT updated! */
bool MainWindow::openFile(QString path, bool first)
{
    // Open file
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text ))
        return false;
    QTextStream is(&file);
    is.setCodec("UTF-8");

    // Check file format/version
    QStringList command = readCommand(is);
    if( command.size() != 4 || command[0] != "%version" ||
        command[1] != "kakuro" || command[2] != "1" )
        return false;

    // Parse contents
    std::vector<Sheet*> new_sheets;
    while((command = readCommand(is)).size() > 0)
    {
        if(command.size() == 1)
        {
            Puzzle *puzzle = Puzzle::fromString(command[0].toUtf8().constData());
            if(puzzle)
                new_sheets.push_back(new Sheet(undo_stack, *puzzle));
        }

        if(!new_sheets.empty() && command.size() == 2 && command[0] == "@title")
            new_sheets.back()->setName(command[1]);
    }

    // Empty collection not allowed!
    if(new_sheets.size() == 0)
        return false;

    if(first)
        removeAllSheets();

    // Add sheets
    last_sheet_id = new_sheets.size();
    for( std::vector<Sheet*>::const_iterator i = new_sheets.begin();
         i != new_sheets.end(); ++i )
    {
        undo_stack.push(new AddRemoveSheetCommand(*this, *i));
    }

    return true;
}

void MainWindow::openFiles(QStringList paths)
{
    sheets->setUpdatesEnabled(false);
    bool first = true;
    for(int n = 0; n < paths.size(); ++n)
    {
        if(openFile(paths[n], first))
            first = false;
        else
            QMessageBox::warning( this, tr("Open File"),
                                  tr("File \"%1\" could not be opened!").arg(paths[n]) );
    }
    if(!first)
    {
        sheets->setCurrentIndex(0);
        if(paths.size() == 1)
            file_path = paths[0];
        else
            file_path.clear();
    }
    sheets->setUpdatesEnabled(true);
    undo_stack.clear();
}

void MainWindow::crop()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    puzzle->grid.crop();
    currentSheet()->fromPuzzle(*puzzle);
}

void MainWindow::enlarge()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    puzzle->grid.enlarge();
    currentSheet()->fromPuzzle(*puzzle);
}

void MainWindow::clearDigits()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    puzzle->state.clearDigits();
    puzzle->state.clearCandidates();
    currentSheet()->fromPuzzle(*puzzle);
}

void MainWindow::clearCandidates()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    puzzle->state.clearCandidates();
    currentSheet()->fromPuzzle(*puzzle);
}

void MainWindow::clearSums()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    puzzle->state.clearSums();
    currentSheet()->fromPuzzle(*puzzle);
}

void MainWindow::calculateCandidates()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    puzzle->state.calculateCandidates();
    currentSheet()->fromPuzzle(*puzzle);
}

void MainWindow::calculateSums()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    puzzle->state.calculateSums();
    currentSheet()->fromPuzzle(*puzzle);
}

void MainWindow::checkPuzzle()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    int n = puzzle->state.solve(0);

    if(n == -1)
        QMessageBox::information(this, tr("Invalid Puzzle"), tr("Current configuration of digits is invalid!"));

    if(n == 0)
        QMessageBox::information(this, tr("Invalid Puzzle"), tr("Puzzle has no solutions!"));

    if(n == 1)
        QMessageBox::information(this, tr("Valid Puzzle"), tr("Puzzle has a unique solution."));

    if(n > 1)
        QMessageBox::information(this, tr("Invalid Puzzle"), tr("Puzzle has multiple solutions!"));
}

void MainWindow::solvePuzzle()
{
    std::auto_ptr<Puzzle> puzzle(currentSheet()->toPuzzle());
    std::vector<int> solution;
    int n = puzzle->state.solve(&solution);

    if(n == -1)
        QMessageBox::information(this, tr("Invalid Puzzle"), tr("Current configuration of digits is invalid!"));
    else
    if(n == 0)
        QMessageBox::information(this, tr("Invalid Puzzle"), tr("Puzzle has no solutions!"));
    else
    {
        if(n > 1)
            QMessageBox::information( this, tr("Invalid Puzzle"),
                tr("Puzzle has multiple solutions!\nOnly one is shown.") );
        puzzle->state.cand = solution;
        currentSheet()->fromPuzzle(*puzzle);
    }
}

void MainWindow::generatePuzzle()
{
    // TODO!
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(confirmUnsavedChanges("Quit"))
        event->accept();
    else
        event->ignore();
}

void MainWindow::updateMoveSheetButtons()
{
    int index = sheets->currentIndex();
    actionSheetMoveLeft ->setEnabled(index > 0);
    actionSheetMoveRight->setEnabled(index < sheets->count() - 1);
}
