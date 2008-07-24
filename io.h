#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include <QString>
#include <QTextStream>
#include <QStringList>

QString quoteWord(QString word);
QStringList readCommand(QTextStream &input);

#endif /* ndef IO_H_INCLUDED */
