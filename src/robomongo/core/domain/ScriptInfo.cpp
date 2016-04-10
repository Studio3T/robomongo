#include "robomongo/core/domain/ScriptInfo.h"

#include <QTextStream>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

namespace
{
    const QString filterForScripts = QObject::tr("JavaScript (*.js);; All Files (*.*)");

    bool loadFromFileText(const QString &filePath, QString &text)
    {
        bool result = false;
        QFile file(filePath);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&file);
            QApplication::setOverrideCursor(Qt::WaitCursor);
            text = in.readAll();
            QApplication::restoreOverrideCursor();
            result = true;
        }
        else {
            QMessageBox::critical(QApplication::activeWindow(), QString("Error"),
                QObject::tr(PROJECT_NAME" can't read from %1:\n%2.")
                    .arg(filePath)
                    .arg(file.errorString()));
        }

        return result;
    }

    bool saveToFileText(QString filePath, const QString &text)
    {
        if (filePath.isEmpty())
            return false;

#ifdef Q_OS_LINUX
        if (QFileInfo(filePath).suffix().isEmpty()) {
            filePath += ".js";
        }
#endif
        bool result = false;
        QFile file(filePath);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&file);
            QApplication::setOverrideCursor(Qt::WaitCursor);
            out << text;
            QApplication::restoreOverrideCursor();
            result = true;
        }
        else {
            QMessageBox::critical(QApplication::activeWindow(), QString("Error"),
                QObject::tr(PROJECT_NAME" can't save to %1:\n%2.")
                    .arg(filePath)
                    .arg(file.errorString()));
        }

        return result;
    }
}

namespace Robomongo
{
    ScriptInfo::ScriptInfo(const QString &script, bool execute, const CursorPosition &position,
                           const QString &title, const QString &filePath) :
        _script(script),
        _execute(execute),
        _title(title),
        _cursor(position),
        _filePath(filePath) {}

    bool ScriptInfo::loadFromFile(const QString &filePath)
    {
        bool result = false;
        QString filepath = QFileDialog::getOpenFileName(QApplication::activeWindow(), filePath, QString(), filterForScripts);
        if (!filepath.isEmpty()) {
            QString out;
            if (loadFromFileText(filepath, out)) {
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

    bool ScriptInfo::saveToFileAs()
    {
        QString filepath = QFileDialog::getSaveFileName(QApplication::activeWindow(),
            QObject::tr("Save As"), _filePath, filterForScripts);

        if (saveToFileText(filepath, _script)) {
            _filePath = filepath;
            return true;
        }
        return false;
    }

    bool ScriptInfo::saveToFile()
    {
        return _filePath.isEmpty() ? saveToFileAs() : saveToFileText(_filePath, _script);
    }
}
