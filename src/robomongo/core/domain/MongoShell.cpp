#include "robomongo/core/domain/MongoShell.h"

#include <QTextStream>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
namespace
{
    QString generateFileName()
    {
        static int sequenceNumber = 1;
        return QString("script%1.js").arg(sequenceNumber++);
    }
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
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            QMessageBox::warning(QApplication::activeWindow(), QString(PROJECT_NAME),
                                 QObject::tr("Cannot write file %1:\n%2.")
                                 .arg(file.fileName())
                                 .arg(file.errorString()));
        }
        else
        {
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

    MongoShell::MongoShell(MongoServer *server, const QString &filePath) :
        QObject(),
        _filePath(filePath.isEmpty()?generateFileName():filePath),
        _server(server),
        _client(server->client()),
        _bus(AppRegistry::instance().bus())
    {

    }

    MongoShell::~MongoShell()
    {
    }

    void MongoShell::open(MongoCollection *collection)
    {
    //    _query = QString("db.%1.find()").arg(collection->name());
    //    _client->send(new ExecuteQueryRequest(this, collection->fullName()));
    }

    void MongoShell::open(const QString &script, const QString &dbName)
    {
        _bus->publish(new ScriptExecutingEvent(this));
        _query = script;
        _client->send(new ExecuteScriptRequest(this, _query, dbName));
    }

    void MongoShell::query(int resultIndex, const MongoQueryInfo &info)
    {
        _client->send(new ExecuteQueryRequest(this, resultIndex, info));
    }

    void MongoShell::autocomplete(const QString &prefix)
    {
        _client->send(new AutocompleteRequest(this, prefix));
    }

    void MongoShell::stop()
    {
        mongo::Scope::setInterruptFlag(true);
    }
    void MongoShell::loadFromFile()
    {
        QString filepath = QFileDialog::getOpenFileName(QApplication::activeWindow(),_filePath);
        if (!filepath.isEmpty())
        {
            QString out;
            if(loadFromFileText(filepath,out))
            {
                _bus->publish(new ScriptExecutingEvent(this));
                _query = out;
                _client->send(new ExecuteScriptRequest(this, _query,QString()));//dbName i don't know
                _filePath = filepath;
            }
        }
    }
    void MongoShell::saveToFileAs()
    {
        QString filepath = QFileDialog::getSaveFileName(QApplication::activeWindow(), tr("Save As"),_filePath);
        if(saveToFileText(filepath,_query))
        {
            _filePath = filepath;
        }
    }
    void MongoShell::saveToFile()
    {
        saveToFileText(_filePath,_query);
    }
    void MongoShell::handle(ExecuteQueryResponse *event)
    {
        _bus->publish(new DocumentListLoadedEvent(this, event->resultIndex, event->queryInfo, _query, event->documents));
    }

    void MongoShell::handle(ExecuteScriptResponse *event)
    {
        _bus->publish(new ScriptExecutedEvent(this, event->result, event->empty));
    }

    void MongoShell::handle(AutocompleteResponse *event)
    {
        _bus->publish(new AutocompleteResponse(this, event->list, event->prefix));
    }
}
