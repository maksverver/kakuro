#include "CellControl.h"
#include "Sheet.h"
#include <QApplication>
#include <QPainter>
#include <QKeyEvent>
#include <QKeyEvent>
#include <algorithm>
#include <iostream> // DEBUG

class CellCommand : public QUndoCommand
{
protected:

    Sheet &sheet;
    QPoint pos;
    struct State {
        bool open;
        int cands, hsum, vsum;

        bool operator== (const State &s) const {
            return open == s.open && cands == s.cands &&
                   hsum == s.hsum && vsum == s.vsum;
        }
    } before, after;

public:
    CellCommand(const CellControl &cc)
        : sheet(cc.sheet()), pos(cc.pos())
    {
        setState(before, cc);
    }

    void setAfter(const CellControl &cc)
    {
        setState(after, cc);
    }

    void undo()
    {
        applyState(before);
        sheet.modified();
    }

    void redo()
    {
        applyState(after);
        sheet.modified();
    }

    bool empty()
    {
        return before == after;
    }

protected:
    void setState(State &s, const CellControl &cc)
    {
        if((s.open = cc.open()))
        {
            s.cands = cc.cands();
            s.vsum =  cc.neighbours[0] ? cc.neighbours[0]->vsum() : 0;
            s.hsum =  cc.neighbours[1] ? cc.neighbours[1]->hsum() : 0;
        }
        else
        {
            s.cands = 0;
            s.hsum  = cc.hsum();
            s.vsum  = cc.vsum();
        }
    }

    void applyState(const State &s)
    {
        CellControl &cc = sheet.cellAt(pos);
        cc.setOpen(s.open);
        if(s.open)
        {
            cc.setCands(s.cands);
            if(cc.neighbours[0])
                cc.neighbours[0]->setVsum(s.vsum);
            if(cc.neighbours[1])
                cc.neighbours[1]->setHsum(s.hsum);
        }
        else
        {
            cc.setHsum(s.hsum);
            cc.setVsum(s.vsum);
        }
    }

};


CellControl::CellControl(QPoint pos, Sheet &sheet, QWidget *parent)
    : QWidget(parent), m_pos(pos), m_open(false),
      m_cands(0), m_hsum(0), m_vsum(0), m_dir(NoDirection), typing(false),
      digit_font(QApplication::font(this).family(), 28, QFont::Bold),
      cand_font(QApplication::font(this).family(), 11),
      sum_font(QApplication::font(this).family(), 12),
      m_sheet(sheet), undo_command(0)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(40, 40);

    setNeighbours(0, 0, 0, 0);
}

CellControl::~CellControl()
{
}

void CellControl::setOpen(bool open)
{
    if(open != m_open && (!open || (m_pos.x() > 0 && m_pos.y() > 0)))
    {
        if(m_open)
        {
            if(neighbours[0] && !neighbours[0]->open())
                 neighbours[0]->clearVsum();
            if(neighbours[1] && !neighbours[1]->open())
                 neighbours[1]->clearHsum();
        }
        if(neighbours[0] && !neighbours[0]->open())
            neighbours[0]->update();
        if(neighbours[1] && !neighbours[1]->open())
            neighbours[1]->update();

        m_open = open;
        m_hsum = m_vsum = m_cands = 0;
        m_dir = NoDirection;
        update();
    }
}

void CellControl::setDigit(int digit)
{
    if(digit)
        setCands(1<<(digit-1));
    else
        setCands(0);
}

void CellControl::setCands(int cands)
{
    if((cands&511) == cands && cands != m_cands)
    {
        m_cands = cands;
        update();
    }
}

void CellControl::setHsum(int sum)
{
    if(sum >= 0 && sum <= 99 && sum != m_hsum)
    {
        m_hsum = sum;
        update();
    }
}

void CellControl::setVsum(int sum)
{
    if(sum >= 0 && sum <= 99 && sum != m_vsum)
    {
        m_vsum = sum;
        update();
    }
}

void CellControl::setDir(Direction dir)
{
    if(dir != m_dir)
    {
        m_dir = dir;
        update();
    }
}

void CellControl::setNeighbours(
    CellControl *top, CellControl *left, CellControl *bottom, CellControl *right )
{
    neighbours[0] = top;
    neighbours[1] = left;
    neighbours[2] = bottom;
    neighbours[3] = right;
}

void CellControl::paint(QPainter &painter, QRect rect)
{
    if(m_open)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(isEnabled() ? hasFocus() ? Qt::cyan : Qt::white : Qt::gray);
        painter.drawRect(rect);

        if(m_cands)
        {
            if(single(m_cands))
            {
                painter.setPen(Qt::black);
                painter.setBrush(Qt::NoBrush);
                painter.setFont(digit_font);
                painter.drawText(rect, Qt::AlignCenter, QString::number(log2(m_cands) + 1));
            }
            else
            {
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::NoBrush);
                painter.setFont(cand_font);
                const int w = (rect.width() - 4)/3, h = (rect.height() - 4)/3;
                for(int d = 0; d < 9; ++d)
                    if((m_cands>>d)&1)
                        painter.drawText(
                            rect.x() + 1 + (d%3)*w, rect.y() + 2 + (d/3)*h, w, h,
                            Qt::AlignCenter, QString::number(1 + d) );
            }
        }
    }
    else
    if(hClue() || vClue())
    {
        QRect r(rect.x() + 1, rect.y() + 1, rect.width() - 2, rect.height() - 2);
        painter.setPen(Qt::NoPen);
        painter.setBrush(isEnabled() ? Qt::black : Qt::darkGray);
        painter.drawRect(r);

        if(hasFocus())
        {
            painter.setBrush(Qt::darkCyan);
            if(m_dir != Vertical)
            {
                QPoint pts[3] = { r.topLeft(), r.bottomRight(), r.topRight() };
                painter.drawPolygon(pts, 3);
            }
            if(m_dir != Horizontal)
            {
                QPoint pts[3] = { r.topLeft(), r.bottomLeft(), r.bottomRight() };
                painter.drawPolygon(pts, 3);
            }
        }

        QPen pen(Qt::white);
        pen.setWidth(2);
        pen.setJoinStyle(Qt::MiterJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(r);
        painter.drawLine(r.topLeft(), r.bottomRight());

        if(m_hsum || m_vsum)
        {
            painter.setPen(Qt::white);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(sum_font);

            if(m_hsum)
            {
                painter.drawText( r.x(), r.y(), r.width() - 4, r.height() - 14,
                                Qt::AlignRight | Qt::AlignVCenter, QString::number(m_hsum) );
            }

            if(m_vsum)
            {
                painter.drawText( r.x(), r.y(), r.width() - 12, r.height(),
                            Qt::AlignHCenter | Qt::AlignBottom, QString::number(m_vsum) );
            }
        }
    }
    else
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(hasFocus() ? Qt::darkCyan : palette().shadow());
        painter.drawRect(rect);
    }
}

void CellControl::paintEvent(QPaintEvent *event)
{
    (void)event;  // unused
    QPainter painter(this);
    paint(painter, rect());
}

void CellControl::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    default:
        event->ignore();
        return;

    case Qt::Key_Insert:
        beginUndoCommand();
        setOpen(false);
        endUndoCommand();
        break;

    case Qt::Key_Delete:
        beginUndoCommand();
        setOpen(true);
        clearDigit();
        endUndoCommand();
        break;

    case Qt::Key_Up:
        if(neighbours[0])
        {
            neighbours[0]->setFocus();
            if(open() && !neighbours[0]->open())
                neighbours[0]->setDir(Vertical);
        }
        break;

    case Qt::Key_Left:
        if(neighbours[1])
        {
            neighbours[1]->setFocus();
            if(open() && !neighbours[1]->open())
                neighbours[1]->setDir(Horizontal);
        }
        break;

    case Qt::Key_Down:
        if(neighbours[2])
            neighbours[2]->setFocus();
        break;

    case Qt::Key_Right:
        if(neighbours[3])
            neighbours[3]->setFocus();
        break;

    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
        {
            beginUndoCommand();
            int digit = event->key() - Qt::Key_0;
            if(open())
            {
                if(event->modifiers() == Qt::ControlModifier)
                    setCands(cands() ^ (1<<(digit-1)));
                else
                    setDigit(digit);
            }
            else
            if(m_dir != NoDirection)
            {
                int &sum = (m_dir == Horizontal) ? m_hsum : m_vsum;
                sum = 10*sum + digit;
                if(!typing || sum > 45)
                    sum = sum%10;
                typing = true;
                update();
            }
            endUndoCommand();
        } break;

    case Qt::Key_Backspace:
        {
            if(cands() || (m_dir != Horizontal && vsum()) || (m_dir != Vertical && hsum()))
            {
                beginUndoCommand();
                clearDigit();
                if(m_dir != Horizontal)
                    clearVsum();
                if(m_dir != Vertical)
                    clearHsum();
                endUndoCommand();
            }

            if(event->key() == Qt::Key_Backspace)
                focusNextPrevChild(false);
        } break;

    }
    event->accept();

    QWidget::keyPressEvent(event);
}

void CellControl::mousePressEvent(QMouseEvent *event)
{
    switch(event->button())
    {
    default:
        break;

    case Qt::LeftButton:
        if(!m_open)
        {
            int lv = (rect().bottomLeft() - event->pos()).manhattanLength(),
                lh = (rect().topRight() - event->pos()).manhattanLength();
            Direction dir = NoDirection;
            if(std::abs(lh - lv) >= 12)
            {
                if(lv < lh && neighbours[2] && neighbours[2]->open())
                    dir = Vertical;
                if(lv > lh && neighbours[3] && neighbours[3]->open())
                    dir = Horizontal;
            }
            setDir(dir);
        }
        break;

    case Qt::RightButton:
        beginUndoCommand();
        setOpen(!open());
        endUndoCommand();
        break;
    }

    QWidget::mousePressEvent(event);
}

void CellControl::focusInEvent(QFocusEvent *event)
{
    if(!open())
    {
        bool downOpen  = neighbours[2] && neighbours[2]->open(),
             rightOpen = neighbours[3] && neighbours[3]->open();
        if(downOpen && !rightOpen)
            m_dir = Vertical;
        if(rightOpen && !downOpen)
            m_dir = Horizontal;
    }
    QWidget::focusInEvent(event);
}

void CellControl::focusOutEvent(QFocusEvent *event)
{
    m_dir = NoDirection;
    typing = false;
    QWidget::focusOutEvent(event);
}

bool CellControl::hClue() const
{
    return !open() && neighbours[3] && neighbours[3]->open();
}

bool CellControl::vClue() const
{
    return !open() && neighbours[2] && neighbours[2]->open();
}

void CellControl::beginUndoCommand()
{
    undo_command = new CellCommand(*this);
}

void CellControl::endUndoCommand()
{
    if(undo_command)
    {
        undo_command->setAfter(*this);
        if(undo_command->empty())
            delete undo_command;
        else
            m_sheet.undoStack().push(undo_command);
        undo_command = 0;
    }
}
