// This module implements the QsciAPIs class.
//
// Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of QScintilla.
// 
// This file may be used under the terms of the GNU General Public
// License versions 2.0 or 3.0 as published by the Free Software
// Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
// included in the packaging of this file.  Alternatively you may (at
// your option) use any later version of the GNU General Public
// License if such license has been publicly approved by Riverbank
// Computing Limited (or its successors, if any) and the KDE Free Qt
// Foundation. In addition, as a special exception, Riverbank gives you
// certain additional rights. These rights are described in the Riverbank
// GPL Exception version 1.1, which can be found in the file
// GPL_EXCEPTION.txt in this package.
// 
// If you are unsure which license is appropriate for your use, please
// contact the sales department at sales@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#include <stdlib.h>

#include "Qsci/qsciapis.h"

#include <qapplication.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qevent.h>
#include <qfile.h>
#include <qmap.h>
#include <qtextstream.h>
#include <qthread.h>

#include <stdlib.h>

#include "Qsci/qscilexer.h"



// The version number of the prepared API information format.
const unsigned char PreparedDataFormatVersion = 0;


// This class contains prepared API information.
struct QsciAPIsPrepared
{
    // The word dictionary is a map of individual words and a list of positions
    // each occurs in the sorted list of APIs.  A position is a tuple of the
    // index into the list of APIs and the index into the particular API.
    QMap<QString, QsciAPIs::WordIndexList> wdict;

    // The case dictionary maps the case insensitive words to the form in which
    // they are to be used.  It is only used if the language is case
    // insensitive.
    QMap<QString, QString> cdict;

    // The Qt v3 QMap doesn't support lowerBound() so we keep a separate copy
    // of the sorted keys and use a local implementation.
    QStringList words;

    static QStringList::const_iterator lowerBound(const QStringList &list,
            const QString &word);

    // The raw API information.
    QStringList raw_apis;

    QStringList apiWords(int api_idx, const QStringList &wseps,
            bool strip_image) const;
    static QString apiBaseName(const QString &api);
};


// Implement qLowerBound() for a QStringList for Qt v3.
QStringList::const_iterator QsciAPIsPrepared::lowerBound(const QStringList &list, const QString &word)
{
    // It would be nice to be able to do a binary search but random access of a
    // QValueList is "slow" - it probably does a search from the start of the
    // list.  Therefore we just do a simple linear search and hope it is quick
    // enough.  If not we will have to implement a new data type.
    QStringList::const_iterator it = list.begin();

    while (it != list.end())
    {
        if (word <= (*it))
            break;

        ++it;
    }

    return it;
}

// Return a particular API entry as a list of words.
QStringList QsciAPIsPrepared::apiWords(int api_idx, const QStringList &wseps,
        bool strip_image) const
{
    QString base = apiBaseName(raw_apis[api_idx]);

    // Remove any embedded image reference if necessary.
    if (strip_image)
    {
        int tail = base.find('?');

        if (tail >= 0)
            base.truncate(tail);
    }

    if (wseps.isEmpty())
        return QStringList(base);

    return QStringList::split(wseps.first(), base);
}


// Return the name of an API function, ie. without the arguments.
QString QsciAPIsPrepared::apiBaseName(const QString &api)
{
    QString base = api;
    int tail = base.find('(');

    if (tail >= 0)
        base.truncate(tail);

    return base.simplifyWhiteSpace();
}


// The user event type that signals that the worker thread has started.
const QEvent::Type WorkerStarted = static_cast<QEvent::Type>(QEvent::User + 1012);


// The user event type that signals that the worker thread has finished.
const QEvent::Type WorkerFinished = static_cast<QEvent::Type>(QEvent::User + 1013);


// The user event type that signals that the worker thread has aborted.
const QEvent::Type WorkerAborted = static_cast<QEvent::Type>(QEvent::User + 1014);


// This class is the worker thread that post-processes the API set.
class QsciAPIsWorker : public QThread
{
public:
    QsciAPIsWorker(QsciAPIs *apis);
    virtual ~QsciAPIsWorker();

    virtual void run();

    QsciAPIsPrepared *prepared;

private:
    QsciAPIs *proxy;
    bool abort;
};


// The worker thread ctor.
QsciAPIsWorker::QsciAPIsWorker(QsciAPIs *apis)
    : proxy(apis), prepared(0), abort(false)
{
}


// The worker thread dtor.
QsciAPIsWorker::~QsciAPIsWorker()
{
    // Tell the thread to stop.  There is no need to bother with a mutex.
    abort = true;

    // Wait for it to do so and hit it if it doesn't.
    if (!wait(500))
        terminate();

    if (prepared)
        delete prepared;
}


// The worker thread entry point.
void QsciAPIsWorker::run()
{
    // Sanity check.
    if (!prepared)
        return;

    // Tell the main thread we have started.
    QApplication::postEvent(proxy, new QEvent(WorkerStarted));

    // Sort the full list.
    prepared->raw_apis.sort();

    QStringList wseps = proxy->lexer()->autoCompletionWordSeparators();
    bool cs = proxy->lexer()->caseSensitive();

    // Split each entry into separate words but ignoring any arguments.
    for (int a = 0; a < prepared->raw_apis.count(); ++a)
    {
        // Check to see if we should stop.
        if (abort)
            break;

        QStringList words = prepared->apiWords(a, wseps, true);

        for (int w = 0; w < words.count(); ++w)
        {
            const QString &word = words[w];

            // Add the word's position to any existing list for this word.
            QsciAPIs::WordIndexList wil = prepared->wdict[word];

            // If the language is case insensitive and we haven't seen this
            // word before then save it in the case dictionary.
            if (!cs && wil.count() == 0)
                prepared->cdict[word.upper()] = word;

            wil.append(QsciAPIs::WordIndex(a, w));
            prepared->wdict[word] = wil;
        }
    }

    if (cs)
        prepared->words = prepared->wdict.keys();
    else
        prepared->words = prepared->cdict.keys();

    // Tell the main thread we have finished.
    QApplication::postEvent(proxy, new QEvent(abort ? WorkerAborted : WorkerFinished));
}


// The ctor.
QsciAPIs::QsciAPIs(QsciLexer *lexer, const char *name)
    : QsciAbstractAPIs(lexer, name),
      worker(0), origin_len(0)
{
    prep = new QsciAPIsPrepared;
}


// The dtor.
QsciAPIs::~QsciAPIs()
{
    deleteWorker();
    delete prep;
}


// Delete the worker thread if there is one.
void QsciAPIs::deleteWorker()
{
    if (worker)
    {
        delete worker;
        worker = 0;
    }
}


//! Handle termination events from the worker thread.
bool QsciAPIs::event(QEvent *e)
{
    switch (e->type())
    {
    case WorkerStarted:
        emit apiPreparationStarted();
        return true;

    case WorkerAborted:
        deleteWorker();
        emit apiPreparationCancelled();
        return true;

    case WorkerFinished:
        delete prep;
        old_context.clear();

        prep = worker->prepared;
        worker->prepared = 0;
        deleteWorker();

        // Allow the raw API information to be modified.
        apis = prep->raw_apis;

        emit apiPreparationFinished();

        return true;
    }

    return QObject::event(e);
}


// Clear the current raw API entries.
void QsciAPIs::clear()
{
    apis.clear();
}


// Clear out all API information.
bool QsciAPIs::load(const QString &filename)
{
    QFile f(filename);

    if (!f.open(IO_ReadOnly))
        return false;

    QTextStream ts(&f);

    for (;;)
    {
        QString line = ts.readLine();

        if (line.isEmpty())
            break;

        apis.append(line);
    }

    return true;
}


// Add a single API entry.
void QsciAPIs::add(const QString &entry)
{
    apis.append(entry);
}


// Remove a single API entry.
void QsciAPIs::remove(const QString &entry)
{
    QStringList::iterator it = apis.find(entry);

    if (it != apis.end())
        apis.remove(it);
}


// Position the "origin" cursor into the API entries according to the user
// supplied context.
QStringList QsciAPIs::positionOrigin(const QStringList &context, QString &path)
{
    // Get the list of words and see if the context is the same as last time we
    // were called.
    QStringList new_context;
    bool same_context = (old_context.count() > 0 && old_context.count() < context.count());

    for (int i = 0; i < context.count(); ++i)
    {
        QString word = context[i];

        if (!lexer()->caseSensitive())
            word = word.upper();

        if (i < old_context.count() && old_context[i] != word)
            same_context = false;

        new_context << word;
    }

    // If the context has changed then reset the origin.
    if (!same_context)
        origin_len = 0;

    // If we have a current origin (ie. the user made a specific selection in
    // the current context) then adjust the origin to include the last complete
    // word as the user may have entered more parts of the name without using
    // auto-completion.
    if (origin_len > 0)
    {
        const QString wsep = lexer()->autoCompletionWordSeparators().first();

        int start_new = old_context.count();
        int end_new = new_context.count() - 1;

        if (start_new == end_new)
        {
            path = old_context.join(wsep);
            origin_len = path.length();
        }
        else
        {
            QString fixed = *origin;
            fixed.truncate(origin_len);

            path = fixed;

            while (start_new < end_new)
            {
                // Add this word to the current path.
                path.append(wsep);
                path.append(new_context[start_new]);
                origin_len = path.length();

                // Skip entries in the current origin that don't match the
                // path.
                while (origin != prep->raw_apis.end())
                {
                    // See if the current origin has come to an end.
                    if (!originStartsWith(fixed, wsep))
                        origin = prep->raw_apis.end();
                    else if (originStartsWith(path, wsep))
                        break;
                    else
                        ++origin;
                }

                if (origin == prep->raw_apis.end())
                    break;

                ++start_new;
            }
        }

        // Terminate the path.
        path.append(wsep);

        // If the new text wasn't recognised then reset the origin.
        if (origin == prep->raw_apis.end())
            origin_len = 0;
    }

    if (origin_len == 0)
        path.truncate(0);

    // Save the "committed" context for next time.
    old_context = new_context;
    old_context.remove(old_context.fromLast());

    return new_context;
}


// Return true if the origin starts with the given path.
bool QsciAPIs::originStartsWith(const QString &path, const QString &wsep)
{
    const QString &orig = *origin;

    if (!orig.startsWith(path))
        return false;

    // Check that the path corresponds to the end of a word, ie. that what
    // follows in the origin is either a word separator or a (.
    QString tail = orig.mid(path.length());

    return (!tail.isEmpty() && (tail.startsWith(wsep) || tail.at(0) == '('));
}


// Add auto-completion words to an existing list.
void QsciAPIs::updateAutoCompletionList(const QStringList &context,
        QStringList &list)
{
    QString path;
    QStringList new_context = positionOrigin(context, path);

    if (origin_len > 0)
    {
        const QString wsep = lexer()->autoCompletionWordSeparators().first();
        QStringList::const_iterator it = origin;

        unambiguous_context = path;

        while (it != prep->raw_apis.end())
        {
            QString base = QsciAPIsPrepared::apiBaseName(*it);

            if (!base.startsWith(path))
                break;

            // Make sure we have something after the path.
            if (base != path)
            {
                // Get the word we are interested in (ie. the one after the
                // current origin in path).
                QString w = QStringList::split(wsep, base.mid(origin_len + wsep.length())).first();

                // Append the space, we know the origin is unambiguous.
                w.append(' ');

                if (!list.contains(w))
                    list << w;
            }

            ++it;
        }
    }
    else
    {
        // At the moment we assume we will add words from multiple contexts.
        unambiguous_context.truncate(0);

        bool unambig = true;
        QStringList with_context;

        if (new_context.last().isEmpty())
            lastCompleteWord(new_context[new_context.count() - 2], with_context, unambig);
        else
            lastPartialWord(new_context.last(), with_context, unambig);

        for (int i = 0; i < with_context.count(); ++i)
        {
            // Remove any unambigious context.
            QString noc = with_context[i];

            if (unambig)
            {
                int op = noc.find('(');

                if (op >= 0)
                    noc.truncate(op);
            }

            list << noc;
        }
    }
}


// Get the index list for a particular word if there is one.
const QsciAPIs::WordIndexList *QsciAPIs::wordIndexOf(const QString &word) const
{
    QString csword;

    // Indirect through the case dictionary if the language isn't case
    // sensitive.
    if (lexer()->caseSensitive())
        csword = word;
    else
    {
        csword = prep->cdict[word];

        if (csword.isEmpty())
            return 0;
    }

    // Get the possible API entries if any.
    const WordIndexList *wl = &prep->wdict[csword];

    if (wl->isEmpty())
        return 0;

    return wl;
}


// Add auto-completion words based on the last complete word entered.
void QsciAPIs::lastCompleteWord(const QString &word, QStringList &with_context, bool &unambig)
{
    // Get the possible API entries if any.
    const WordIndexList *wl = wordIndexOf(word);

    if (wl)
        addAPIEntries(*wl, true, with_context, unambig);
}


// Add auto-completion words based on the last partial word entered.
void QsciAPIs::lastPartialWord(const QString &word, QStringList &with_context, bool &unambig)
{
    if (lexer()->caseSensitive())
    {
        QMap<QString, WordIndexList>::const_iterator it;
        QStringList::const_iterator wit = QsciAPIsPrepared::lowerBound(prep->words, word);

        if (wit == prep->words.end())
            it = prep->wdict.end();
        else
            it = prep->wdict.find(*wit);

        while (it != prep->wdict.end())
        {
            if (!it.key().startsWith(word))
                break;

            addAPIEntries(it.data(), false, with_context, unambig);

            ++it;
        }
    }
    else
    {
        QMap<QString, QString>::const_iterator it;
        QStringList::const_iterator wit = QsciAPIsPrepared::lowerBound(prep->words, word);

        if (wit == prep->words.end())
            it = prep->cdict.end();
        else
            it = prep->cdict.find(*wit);

        while (it != prep->cdict.end())
        {
            if (!it.key().startsWith(word))
                break;

            addAPIEntries(prep->wdict[it.data()], false, with_context, unambig);

            ++it;
        }
    }
}


// Handle the selection of an entry in the auto-completion list.
void QsciAPIs::autoCompletionSelected(const QString &selection)
{
    // If the selection is an API (ie. it has a space separating the selected
    // word and the optional origin) then remember the origin.
    QStringList lst = QStringList::split(' ', selection);

    if (lst.count() != 2)
    {
        origin_len = 0;
        return;
    }

    const QString &path = lst[1];
    QString owords;

    if (path.isEmpty())
        owords = unambiguous_context;
    else
    {
        // Check the parenthesis.
        if (!path.startsWith("(") || !path.endsWith(")"))
        {
            origin_len = 0;
            return;
        }

        // Remove the parenthesis.
        owords = path.mid(1, path.length() - 2);
    }

    origin = QsciAPIsPrepared::lowerBound(prep->raw_apis, owords);
    origin_len = owords.length();
}


// Add auto-completion words for a particular word (defined by where it appears
// in the APIs) and depending on whether the word was complete (when it's
// actually the next word in the API entry that is of interest) or not.
void QsciAPIs::addAPIEntries(const WordIndexList &wl, bool complete,
        QStringList &with_context, bool &unambig)
{
    QStringList wseps = lexer()->autoCompletionWordSeparators();

    for (int w = 0; w < wl.count(); ++w)
    {
        const WordIndex &wi = wl[w];

        QStringList api_words = prep->apiWords(wi.first, wseps, false);

        int idx = wi.second;

        if (complete)
        {
            // Skip if this is the last word.
            if (++idx >= api_words.count())
                continue;
        }

        QString api_word;

        if (idx == 0)
            api_word = api_words[0] + ' ';
        else
        {
            QStringList orgl;
    
            for (int i = 0; i < idx; ++i)
                orgl.append(api_words[i]);

            QString org = orgl.join(wseps.first());

            api_word = QString("%1 (%2)").arg(api_words[idx]).arg(org);

            // See if the origin has been used before.
            if (unambig)
                if (unambiguous_context.isEmpty())
                    unambiguous_context = org;
                else if (unambiguous_context != org)
                {
                    unambiguous_context.truncate(0);
                    unambig = false;
                }
        }

        if (!with_context.contains(api_word))
            with_context.append(api_word);
    }
}


// Return the call tip for a function.
QStringList QsciAPIs::callTips(const QStringList &context, int commas,
        QsciScintilla::CallTipsStyle style,
        QValueList<int> &shifts)
{
    QString path;
    QStringList new_context = positionOrigin(context, path);
    QStringList wseps = lexer()->autoCompletionWordSeparators();
    QStringList cts;

    if (origin_len > 0)
    {
        QStringList::const_iterator it = origin;
        QString prev;

        // Work out the length of the context.
        const QString &wsep = wseps.first();
        QStringList strip = QStringList::split(wsep, path);
        strip.remove(strip.fromLast());
        int ctstart = strip.join(wsep).length();

        if (ctstart)
            ctstart += wsep.length();

        int shift;

        if (style == QsciScintilla::CallTipsContext)
        {
            shift = ctstart;
            ctstart = 0;
        }
        else
            shift = 0;

        // Make sure we only look at the functions we are interested in.
        path.append('(');

        while (it != prep->raw_apis.end() && (*it).startsWith(path))
        {
            QString w = (*it).mid(ctstart);

            if (w != prev && enoughCommas(w, commas))
            {
                shifts << shift;
                cts << w;
                prev = w;
            }

            ++it;
        }
    }
    else
    {
        const QString &fname = new_context[new_context.count() - 2];

        // Find everywhere the function name appears in the APIs.
        const WordIndexList *wil = wordIndexOf(fname);

        if (wil)
            for (int i = 0; i < wil->count(); ++i)
            {
                const WordIndex &wi = (*wil)[i];
                QStringList awords = prep->apiWords(wi.first, wseps, true);

                // Check the word is the function name and not part of any
                // context.
                if (wi.second != awords.count() - 1)
                    continue;

                const QString &api = prep->raw_apis[wi.first];

                int tail = api.find('(');

                if (tail < 0)
                    continue;

                if (!enoughCommas(api, commas))
                    continue;

                if (style == QsciScintilla::CallTipsNoContext)
                {
                    shifts << 0;
                    cts << (fname + api.mid(tail));
                }
                else
                {
                    shifts << tail - fname.length();
                    cts << api;
                }
            }
    }

    return cts;
}


// Return true if a string has enough commas in the argument list.
bool QsciAPIs::enoughCommas(const QString &s, int commas)
{
    int end = s.find(')');

    if (end < 0)
        return false;

    QString w = s.left(end);

    return (w.contains(',') >= commas);
}


// Ensure the list is ready.
void QsciAPIs::prepare()
{
    // Handle the trivial case.
    if (worker)
        return;

    QsciAPIsPrepared *new_apis = new QsciAPIsPrepared;
    new_apis->raw_apis = apis;

    worker = new QsciAPIsWorker(this);
    worker->prepared = new_apis;
    worker->start();
}


// Cancel any current preparation.
void QsciAPIs::cancelPreparation()
{
    deleteWorker();
}


// Check that a prepared API file exists.
bool QsciAPIs::isPrepared(const QString &filename) const
{
    QString pname = prepName(filename);

    if (pname.isEmpty())
        return false;

    QFileInfo fi(pname);

    return fi.exists();
}


// Load the prepared API information.
bool QsciAPIs::loadPrepared(const QString &filename)
{
    QString pname = prepName(filename);

    if (pname.isEmpty())
        return false;

    // Read the prepared data and decompress it.
    QFile pf(pname);

    if (!pf.open(IO_ReadOnly))
        return false;

    QByteArray cpdata = pf.readAll();

    pf.close();

    if (cpdata.count() == 0)
        return false;

    QByteArray pdata = qUncompress(cpdata);

    // Extract the data.
    QDataStream pds(pdata, IO_ReadOnly);

    unsigned char vers;
    pds >> vers;

    if (vers > PreparedDataFormatVersion)
        return false;

    char *lex_name;
    pds >> lex_name;

    if (qstrcmp(lex_name, lexer()->lexer()) != 0)
    {
        delete[] lex_name;
        return false;
    }

    delete[] lex_name;

    prep->wdict.clear();
    pds >> prep->wdict;

    if (!lexer()->caseSensitive())
    {
        // Build up the case dictionary.
        prep->cdict.clear();

        QMap<QString, WordIndexList>::const_iterator it = prep->wdict.begin();

        while (it != prep->wdict.end())
        {
            prep->cdict[it.key().upper()] = it.key();

            ++it;
        }

        prep->words = prep->cdict.keys();
    }
    else
        prep->words = prep->wdict.keys();

    prep->raw_apis.clear();
    pds >> prep->raw_apis;

    // Allow the raw API information to be modified.
    apis = prep->raw_apis;

    return true;
}


// Save the prepared API information.
bool QsciAPIs::savePrepared(const QString &filename) const
{
    QString pname = prepName(filename, true);

    if (pname.isEmpty())
        return false;

    // Write the prepared data to a memory buffer.
    QByteArray pdata;
    QDataStream pds(pdata, IO_WriteOnly);

    // Use a serialisation format supported by Qt v3.0 and later.
    pds.setVersion(4);
    pds << PreparedDataFormatVersion;
    pds << lexer()->lexer();
    pds << prep->wdict;
    pds << prep->raw_apis;

    // Compress the data and write it.
    QFile pf(pname);

    if (!pf.open(IO_WriteOnly|IO_Truncate))
        return false;

    if (pf.writeBlock(qCompress(pdata)) < 0)
    {
        pf.close();
        return false;
    }

    pf.close();
    return true;
}


// Return the name of the default prepared API file.
QString QsciAPIs::defaultPreparedName() const
{
    return prepName(QString());
}


// Return the name of a prepared API file.
QString QsciAPIs::prepName(const QString &filename, bool mkpath) const
{
    // Handle the tivial case.
    if (!filename.isEmpty())
        return filename;

    QString pdname;
    char *qsci = getenv("QSCIDIR");

    if (qsci)
        pdname = qsci;
    else
    {
        static const char *qsci_dir = ".qsci";

        QDir pd = QDir::home();

        if (mkpath && !pd.exists(qsci_dir) && !pd.mkdir(qsci_dir))
            return QString();

        pdname = pd.filePath(qsci_dir);
    }

    return QString("%1/%2.pap").arg(pdname).arg(lexer()->lexer());
}


// Return installed API files.
QStringList QsciAPIs::installedAPIFiles() const
{
    const char *qtdir = getenv("QTDIR");

    if (!qtdir)
        return QStringList();

    QDir apidir = QDir(QString("%1/qsci/api/%2").arg(qtdir).arg(lexer()->lexer()));
    QStringList filenames;

    const QFileInfoList *flist = apidir.entryInfoList("*.api", QDir::Files, QDir::IgnoreCase);

    if (flist)
    {
        QPtrListIterator<QFileInfo> it(*flist);
        QFileInfo *fi;

        while ((fi = it.current()) != 0)
        {
            filenames << fi->absFilePath();
            ++it;
        }
    }

    return filenames;
}
