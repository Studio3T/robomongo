// This defines the interface to the QsciMacro class.
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


#ifndef QSCIMACRO_H
#define QSCIMACRO_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>
#include <qstring.h>

#include <qlist.h>

#include <Qsci/qsciglobal.h>


class QsciScintilla;


//! \brief The QsciMacro class represents a sequence of recordable editor
//! commands.
//!
//! Methods are provided to convert convert a macro to and from a textual
//! representation so that they can be easily written to and read from
//! permanent storage.
class QSCINTILLA_EXPORT QsciMacro : public QObject
{
    Q_OBJECT

public:
    //! Construct a QsciMacro with parent \a parent.
    QsciMacro(QsciScintilla *parent);

    //! Construct a QsciMacro from the printable ASCII representation \a asc,
    //! with parent \a parent.
    QsciMacro(const QString &asc, QsciScintilla *parent);

    //! Destroy the QsciMacro instance.
    virtual ~QsciMacro();

    //! Clear the contents of the macro.
    void clear();

    //! Load the macro from the printable ASCII representation \a asc.  Returns
    //! true if there was no error.
    //!
    //! \sa save()
    bool load(const QString &asc);

    //! Return a printable ASCII representation of the macro.  It is guaranteed
    //! that only printable ASCII characters are used and that double quote
    //! characters will not be used.
    //!
    //! \sa load()
    QString save() const;

public slots:
    //! Play the macro.
    virtual void play();

    //! Start recording user commands and add them to the macro.
    virtual void startRecording();

    //! Stop recording user commands.
    virtual void endRecording();

private slots:
    void record(unsigned int msg, unsigned long wParam, void *lParam);

private:
    struct Macro {
        unsigned int msg;
        unsigned long wParam;
        QByteArray text;
    };

    QsciScintilla *qsci;
    QList<Macro> macro;

    QsciMacro(const QsciMacro &);
    QsciMacro &operator=(const QsciMacro &);
};

#ifdef __APPLE__
}
#endif

#endif
