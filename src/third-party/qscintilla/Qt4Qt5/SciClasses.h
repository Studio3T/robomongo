// The definition of various Qt version independent classes used by the rest of
// the port.
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


#ifndef _SCICLASSES_H
#define _SCICLASSES_H

#include <qglobal.h>
#include <qwidget.h>

#include <Qsci/qsciglobal.h>


QT_BEGIN_NAMESPACE
class QMouseEvent;
class QPaintEvent;
QT_END_NAMESPACE

class QsciScintillaQt;
class QsciListBoxQt;


// A simple QWidget sub-class to implement a call tip.  This is not put into
// the Scintilla namespace because of moc's problems with preprocessor macros.
class QsciSciCallTip : public QWidget
{
    Q_OBJECT

public:
    QsciSciCallTip(QWidget *parent, QsciScintillaQt *sci_);
    ~QsciSciCallTip();

protected:
    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *e);

private:
    QsciScintillaQt *sci;
};


// A popup menu where options correspond to a numeric command.  This is not put
// into the Scintilla namespace because of moc's problems with preprocessor
// macros.

#include <QMenu>
#include <QSignalMapper>

class QsciSciPopup : public QMenu
{
    Q_OBJECT

public:
    QsciSciPopup();

    void addItem(const QString &label, int cmd, bool enabled,
            QsciScintillaQt *sci_);

private slots:
    void on_triggered(int cmd);

private:
    QsciScintillaQt *sci;
    QSignalMapper mapper;
};



// This sub-class of QListBox is needed to provide slots from which we can call
// QsciListBox's double-click callback (and you thought this was a C++
// program).  This is not put into the Scintilla namespace because of moc's
// problems with preprocessor macros.


#include <QListWidget>

class QsciSciListBox : public QListWidget
{
    Q_OBJECT

public:
    QsciSciListBox(QWidget *parent, QsciListBoxQt *lbx_);
    virtual ~QsciSciListBox();

    void addItemPixmap(const QPixmap &pm, const QString &txt);

    int find(const QString &prefix);
    QString text(int n);

protected:
    void keyPressEvent(QKeyEvent *e);

private slots:
    void handleSelection();

private:
    QsciListBoxQt *lbx;
};


#endif
