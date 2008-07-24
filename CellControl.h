#ifndef CELLCONTROL_H_INCLUDED
#define CELLCONTROL_H_INCLUDED

#include <QWidget>
#include <QPainter>
#include <QFont>
#include <QPoint>
#include <QUndoStack>

#include "Puzzle.h"

class Sheet;

class CellControl : public QWidget
{
    Q_OBJECT

public:
    enum Direction { NoDirection, Horizontal, Vertical };

    CellControl(QPoint pos, Sheet &sheet, QWidget *parent = 0);
    ~CellControl();

    void setNeighbours( CellControl *top, CellControl *left,
                        CellControl *bottom, CellControl *right );
    bool hClue() const;
    bool vClue() const;

    const QPoint &pos() const { return m_pos; }
    bool open() const { return m_open; }
    int digit() const { return (m_cands && single(m_cands)) ? log2(m_cands) : 0; }
    int cands() const { return m_cands; }
    int hsum() const { return m_hsum; }
    int vsum() const { return m_vsum; }
    Direction dir() const { return m_dir; }
    Sheet &sheet() const { return m_sheet; }
    bool empty() const { return !(open() || hClue() || vClue()); };

    void setOpen(bool open);
    void setDigit(int digit);
    void setCands(int cands);
    void setHsum(int sum);
    void setVsum(int sum);
    void setDir(Direction dir);
    void clearDigit() { setDigit(0); };
    void clearHsum() { setHsum(0); };
    void clearVsum() { setVsum(0); };

    void paint(QPainter &painter, QRect rect);

protected:
    void paintEvent(QPaintEvent *event);
    void keyPressEvent(QKeyEvent * event);
    void mousePressEvent(QMouseEvent *event);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

    void beginUndoCommand();
    void endUndoCommand();

private:
    QPoint m_pos;
    bool m_open;
    int m_cands, m_hsum, m_vsum;
    Direction m_dir;
    bool typing;
    QFont digit_font, cand_font, sum_font;
    CellControl *neighbours[4];
    Sheet &m_sheet;

    class CellCommand *undo_command;
    friend class CellCommand;
};

#endif /* ndef CELLCONTROL_H_INCLUDED */
