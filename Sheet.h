#ifndef SHEET_H_INCLUDED
#define SHEET_H_INCLUDED

#include "CellControl.h"
#include "Puzzle.h"
#include <QWidget>
#include <QGridLayout>
#include <QList>
#include <QUndoStack>


class Sheet : public QWidget
{
    Q_OBJECT

signals:
    void modified(int index);

public:
    static const int Sheet::cell_size;

    Sheet(QUndoStack &undo_stack, QSize grid_size = QSize(1, 1), QWidget *parent = 0);
    Sheet(QUndoStack &undo_stack, const Puzzle &puzzle, QWidget *parent = 0);

    const QSize &gridSize() { return grid_size; }
    void setGridSize(const QSize &size);

    Puzzle *toPuzzle() const;
    void fromPuzzle(const Puzzle &puzzle);

    bool empty();

    QUndoStack &undoStack() { return undo_stack; }

    CellControl &cellAt(int n) {
        return *reinterpret_cast<CellControl*>(layout()->itemAt(n)->widget()); };
    const CellControl &cellAt(int n) const {
        return *reinterpret_cast<CellControl*>(layout()->itemAt(n)->widget()); };
    CellControl &cellAt(int r, int c) {
        return cellAt(r*grid_size.width() + c); };
    CellControl &cellAt(QPoint &p) {
        return cellAt(p.y(), p.x()); };

    void setName(QString name) { m_name = name; }
    QString name() { return m_name; }

    void paint(QPainter &painter);
    QRect paintRect();

    void setIndex(int index) { m_index = index; }
    int index() { return m_index; }
    void modified() { emit modified(m_index); }


protected:
    void initialize();
    void clear();
    void setPuzzle(const Puzzle &puzzle);

private:
    QString     m_name;
    QUndoStack  &undo_stack;
    QSize       grid_size;
    int         m_index;

    friend class SetPuzzleCommand;
};

#endif /* SHEET_H_INCLUDED */

