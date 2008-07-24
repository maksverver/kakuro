#include "io.h"

QString quoteWord(QString word)
{
    for(int n = 0; n < word.size(); ++n)
        if(word[n].isSpace() || word[n] == '"')
            goto quote;

    return word;

quote:
    QString s;
    s.append('"');
    for(int n = 0; n < word.size(); ++n)
    {
        if(word[n] == '"' || word[n] == '\\')
            s.append('\\');
        s.append(word[n]);
    }
    s.append('"');
    return s;
}

QStringList readCommand(QTextStream &input)
{
    QString line;
    do {
        line = input.readLine().trimmed();
        if(line.isNull())
            return QStringList();
    } while(line.isEmpty() || line[0] == '#');

    QStringList result;
    int p = 0;
    do {
        QString word;
        if(line[p] == '"')
        {
            // Parse quoted word
            bool escape = false;
            while(++p < line.size())
            {
                if(escape)
                {
                    word.append(line[p]);
                    escape = false;
                }
                else
                if(line[p] == '"')
                    break;
                else
                if(line[p] == '\\')
                    escape = true;
                else
                    word.append(line[p]);
            }
            if(p == line.size()) // Unterminated quoted word!
                return QStringList();
            ++p;    // skip terminating quote
        }
        else
        {
            // Parse unquoted word
            while(p < line.size() && !line[p].isSpace())
                word.append(line[p++]);
        }
        while(p < line.size() && line[p].isSpace()) ++p;
        result.append(word);
    } while(p != line.size());
    return result;
}
