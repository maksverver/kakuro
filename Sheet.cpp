#include "Sheet.h"

#include <QMouseEvent>
#include <QMessageBox>

#include <algorithm>
#include <memory>
#include <iostream> // DEBUG

class SetPuzzleCommand : public QUndoCommand
{
public:
    SetPuzzleCommand(
            Sheet &sheet,
            std::unique_ptr<Puzzle> before,
            std::unique_ptr<Puzzle> after)
        : sheet(sheet), before(std::move(before)), after(std::move(after))
    {
    }

    void undo()
    {
        sheet.setPuzzle(*before);
        sheet.modified();
    }

    void redo()
    {
        sheet.setPuzzle(*after);
        sheet.modified();
    }

private:
    Sheet &sheet;
    std::unique_ptr<Puzzle> before, after;
};

// This controls the cell size when printing or saving as images. To change the cell size in the UI,
// change the arguments to setMinimumSize() in CellControl.cpp instead.
const int Sheet::cell_size = 48;

Sheet::Sheet(QUndoStack &undo_stack, QSize size, QWidget *parent)
    : QWidget(parent), undo_stack(undo_stack)
{
    initialize();
    setGridSize(size);
}

Sheet::Sheet(QUndoStack &undo_stack, const Puzzle &puzzle, QWidget *parent)
    : QWidget(parent), undo_stack(undo_stack)
{
    initialize();
    setPuzzle(puzzle);
}

void Sheet::initialize()
{
    // Set style
    QPalette palette;
    palette.setColor(QPalette::Dark,   QColor(64, 64, 64));
    palette.setColor(QPalette::Shadow, Qt::black);
    setPalette(palette);
    setAutoFillBackground(true);

    // Create lay-out
    new QGridLayout(this);
    layout()->setSpacing(2);
    layout()->setMargin(2);

    // Initialize other members
    m_index = -1;
}

void Sheet::clear()
{
    QLayoutItem *child;
    while((child = layout()->takeAt(0)) != 0)
    {
        delete child->widget();
        delete child;
    }
}

void Sheet::setGridSize(const QSize &size)
{
    if(size.width() < 0 || size.height() < 0)
        return;

    QGridLayout *l = qobject_cast<QGridLayout*>(layout());

    l->setEnabled(false);

    clear();

    grid_size = size;

    // Add cell controls
    for(int r = 0; r < size.height(); ++r)
        for(int c = 0; c < size.width(); ++c)
        {
            CellControl *cc = new CellControl(QPoint(c, r), *this);
            l->addWidget(cc, r, c);
            cc->show();
        }

    // Set cell neighbours
    for(int r = 0; r < size.height(); ++r)
        for(int c = 0; c < size.width(); ++c)
        {
            cellAt(r, c).setNeighbours(
                r > 0                 ? &cellAt(r - 1, c    ) : 0,
                c > 0                 ? &cellAt(r    , c - 1) : 0,
                r < size.height() - 1 ? &cellAt(r + 1, c    ) : 0,
                c < size.width()  - 1 ? &cellAt(r    , c + 1) : 0 );
        }

    setFixedSize(l->minimumSize());
    l->setEnabled(true);
}

std::unique_ptr<Puzzle> Sheet::toPuzzle() const
{
    std::unique_ptr<Puzzle> puzzle = std::make_unique<Puzzle>();

    Grid &grid = puzzle->grid;
    State &state = puzzle->state;

    grid.height = grid_size.height();
    grid.width  = grid_size.width();
    grid.vars.resize(layout()->count(), -1);

    int hgrp = -1;
    std::vector<int> vgrps(grid_size.width(), -1);
    for(int n = grid_size.width() + 1, c = 1; n < layout()->count(); ++n)
    {
        if(cellAt(n).open())
        {
            if(hgrp == -1)
            {
                hgrp = state.grps++;
                state.sum.push_back(cellAt(n - 1).hsum());
                state.mem.resize(state.grps);
            }
            if(vgrps[c] == -1)
            {
                vgrps[c] = state.grps++;
                state.sum.push_back(cellAt(n - grid_size.width()).vsum());
                state.mem.resize(state.grps);
            }

            grid.vars[n] = state.vars;
            state.cand.push_back(cellAt(n).cands());
            state.hgrp.push_back(hgrp);
            state.vgrp.push_back(vgrps[c]);
            state.mem[hgrp].push_back(state.vars);
            state.mem[vgrps[c]].push_back(state.vars);
            ++state.vars;
        }
        else
        {
            hgrp = vgrps[c] = -1;
        }

        if(++c == grid_size.width())
            c = 0;
    }
    return puzzle;
}

void Sheet::fromPuzzle(const Puzzle &puzzle)
{
    std::unique_ptr<Puzzle> old_puzzle(toPuzzle());
    if(puzzle != *old_puzzle)
    {
        undo_stack.push(
            new SetPuzzleCommand(*this, std::move(old_puzzle), std::make_unique<Puzzle>(puzzle)) );
    }
}

void Sheet::setPuzzle(const Puzzle &puzzle)
{
    setUpdatesEnabled(false);

    int W = puzzle.grid.width, H = puzzle.grid.height;
    QSize new_size(W, H);
    if(grid_size != new_size)
        setGridSize(QSize(W, H));

    for(int n = 0; n < W*H; ++n)
    {
        int v = puzzle.grid.vars[n];
        if(v >= 0)
        {
            cellAt(n).setOpen(true);
            cellAt(n).setCands(puzzle.state.cand[v]);
            cellAt(n - W).setVsum(puzzle.state.sum[puzzle.state.vgrp[v]]);
            cellAt(n - 1).setHsum(puzzle.state.sum[puzzle.state.hgrp[v]]);
        }
        else
        {
            cellAt(n).setOpen(false);
            cellAt(n).setHsum(0);
            cellAt(n).setVsum(0);
        }
    }
    setUpdatesEnabled(true);
}

bool Sheet::empty()
{
    for(int n = 0; n < grid_size.width()*grid_size.height(); ++n)
        if(!cellAt(n).empty())
            return false;
    return true;
}

void Sheet::paint(QPainter &painter)
{
    for(int r = 0; r < grid_size.height(); ++r)
        for(int c = 0; c < grid_size.width(); ++c)
        {
            CellControl &cell = cellAt(r, c);
            if(!cell.empty())
            {
                cell.paint(painter, QRect( 2 + cell_size*c, 2 + cell_size*r,
                                           cell_size - 2, cell_size - 2 ));
                painter.setPen(QPen(QBrush(Qt::black), 2));
                painter.drawRect(1 + cell_size*c, 1 + cell_size*r, cell_size, cell_size);
            }
        }
}

QRect Sheet::paintRect()
{
    int min_r = 999999999, min_c = 999999999, max_r = -1, max_c = -1;
    for(int r = 0; r < grid_size.height(); ++r)
        for(int c = 0; c < grid_size.width(); ++c)
            if(!cellAt(r, c).empty())
            {
                min_r = std::min(min_r, r);
                min_c = std::min(min_c, c);
                max_r = std::max(max_r, r);
                max_c = std::max(max_c, c);
            }

    if(max_r == -1)
        return QRect();

    return QRect( cell_size*min_c,                     cell_size*min_r,
                  cell_size*(max_c - min_c + 1) + 1,   cell_size*(max_r - min_r + 1) + 1 );

}
