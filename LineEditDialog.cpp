#include "LineEditDialog.h"

LineEditDialog::LineEditDialog(QWidget *parent, QString title, QString labelText, QString text)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(title);
    label->setText(labelText);
    lineEdit->setText(text);
}

QString LineEditDialog::text()
{
    return lineEdit->text();
}
