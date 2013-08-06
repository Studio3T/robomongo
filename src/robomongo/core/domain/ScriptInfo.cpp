#include "robomongo/core/domain/ScriptInfo.h"

#include <QTextStream>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

namespace
{
    const QString filterForScripts = QObject::tr("JavaScript (*.js);; All Files (*.*)");

    bool loadFromFileText(const QString &filePath,QString &text)
    {
        bool result =false;
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::warning(QApplication::activeWindow(), QString(PROJECT_NAME),
                QObject::tr("Cannot read file %1:\n%2.")
                .arg(filePath)
                .arg(file.errorString()));
        }
        else{
            QTextStream in(&file);
            QApplication::setOverrideCursor(Qt::WaitCursor);
            text = in.readAll();
            QApplication::restoreOverrideCursor();
            result=true;
        }
        return result;
    }

    bool saveToFileText(const QString &filePath,const QString &text)
    {
        bool result =false;
        QFile file(filePath);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&file);
            QApplication::setOverrideCursor(Qt::WaitCursor);
            out << text;
            QApplication::restoreOverrideCursor();
            result = true;
        }
        return result;
    }
}

namespace Robomongo
{
    ScriptInfo::ScriptInfo(const QString &script, bool execute,const CursorPosition &position,const QString &title,const QString &filePath) 
        : _script(script),_execute(execute),_title(title),_cursor(position),_filePath(filePath)
    {

    }

    bool ScriptInfo::loadFromFile(const QString &filePath)
    {
        bool result = false;
        QString filepath = QFileDialog::getOpenFileName(QApplication::activeWindow(),filePath,QString(),filterForScripts);
        if (!filepath.isEmpty())
        {
            QString out;
            if(loadFromFileText(filepath,out))
            {
                _script = out;
                _filePath = filepath;
                result = true;
            }
        }
        return result;
    }

    bool ScriptInfo::loadFromFile()
    {
        return loadFromFile(_filePath);
    }

    void ScriptInfo::saveToFileAs()
    {
        QString filepath = QFileDialog::getSaveFileName(QApplication::activeWindow(), QObject::tr("Save As"),_filePath,filterForScripts);
        if(saveToFileText(filepath,_script))
        {
            _filePath = filepath;
        }
    }

    void ScriptInfo::saveToFile()
    {
        if(!_filePath.isEmpty())
        {
            saveToFileText(_filePath,_script);
        }
        else
        {
            saveToFileAs();
        }
    }
}
