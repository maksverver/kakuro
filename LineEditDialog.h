#ifndef SHEETNAMEDIALOG_H_INCLUDED
#define SHEETNAMEDIALOG_H_INCLUDED

#include <QDialog>
#include <QString>
#include "ui_LineEditDialog.h"

class LineEditDialog : public QDialog, protected Ui::LineEditDialog
{
    Q_OBJECT

public:
    LineEditDialog(QWidget *parent, QString title, QString labelText, QString text = QString());

    QString text();
};

#endif /* ndef SHEETNAMEDIALOG_H_INCLUDED */
