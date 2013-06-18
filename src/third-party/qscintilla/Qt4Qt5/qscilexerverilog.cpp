// This module implements the QsciLexerVerilog class.
//
// Copyright (c) 2012 Riverbank Computing Limited <info@riverbankcomputing.com>
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


#include "Qsci/qscilexerverilog.h"

#include <qcolor.h>
#include <qfont.h>
#include <qsettings.h>


// The ctor.
QsciLexerVerilog::QsciLexerVerilog(QObject *parent)
    : QsciLexer(parent),
      fold_atelse(false), fold_comments(false), fold_compact(true),
      fold_preproc(false), fold_atmodule(false)
{
}


// The dtor.
QsciLexerVerilog::~QsciLexerVerilog()
{
}


// Returns the language name.
const char *QsciLexerVerilog::language() const
{
    return "Verilog";
}


// Returns the lexer name.
const char *QsciLexerVerilog::lexer() const
{
    return "verilog";
}


// Return the style used for braces.
int QsciLexerVerilog::braceStyle() const
{
    return Operator;
}


// Returns the set of keywords.
const char *QsciLexerVerilog::keywords(int set) const
{
    if (set == 1)
        return
            "always and assign automatic begin buf bufif0 bufif1 case casex "
            "casez cell cmos config deassign default defparam design disable "
            "edge else end endcase endconfig endfunction endgenerate "
            "endmodule endprimitiveendspecify endtable endtask event for "
            "force forever fork function generate genvar highz0 highz1 if "
            "ifnone incdir include initial inout input instance integer join "
            "large liblist library localparam macromodule medium module nand "
            "negedge nmos nor noshowcancelled not notif0 notif1 or output "
            "parameter pmos posedge primitive pull0 pull1 pulldown pullup "
            "pulsestyle_ondetect pulsestyle_onevent rcmos real realtime reg "
            "release repeat rnmos rpmos rtran rtranif0 rtranif1 scalared "
            "showcancelled signed small specify specparam strong0 strong1 "
            "supply0 supply1 table task time tran tranif0 tranif1 tri tri0 "
            "tri1 triand trior trireg unsigned use vectored wait wand weak0 "
            "weak1 while wire wor xnor xor";

    if (set == 3)
        return
            "$async$and$array $async$and$plane $async$nand$array "
            "$async$nand$plane $async$nor$array $async$nor$plane "
            "$async$or$array $async$or$plane $bitstoreal $countdrivers "
            "$display $displayb $displayh $displayo $dist_chi_square "
            "$dist_erlang $dist_exponential $dist_normal $dist_poisson "
            "$dist_t $dist_uniform $dumpall $dumpfile $dumpflush $dumplimit "
            "$dumpoff $dumpon $dumpportsall $dumpportsflush $dumpportslimit "
            "$dumpportsoff $dumpportson $dumpvars $fclose $fdisplayh "
            "$fdisplay $fdisplayf $fdisplayb $ferror $fflush $fgetc $fgets "
            "$finish $fmonitorb $fmonitor $fmonitorf $fmonitorh $fopen "
            "$fread $fscanf $fseek $fsscanf $fstrobe $fstrobebb $fstrobef "
            "$fstrobeh $ftel $fullskew $fwriteb $fwritef $fwriteh $fwrite "
            "$getpattern $history $hold $incsave $input $itor $key $list "
            "$log $monitorb $monitorh $monitoroff $monitoron $monitor "
            "$monitoro $nochange $nokey $nolog $period $printtimescale "
            "$q_add $q_exam $q_full $q_initialize $q_remove $random "
            "$readmemb $readmemh $readmemh $realtime $realtobits $recovery "
            "$recrem $removal $reset_count $reset $reset_value $restart "
            "$rewind $rtoi $save $scale $scope $sdf_annotate $setup "
            "$setuphold $sformat $showscopes $showvariables $showvars "
            "$signed $skew $sreadmemb $sreadmemh $stime $stop $strobeb "
            "$strobe $strobeh $strobeo $swriteb $swriteh $swriteo $swrite "
            "$sync$and$array $sync$and$plane $sync$nand$array "
            "$sync$nand$plane $sync$nor$array $sync$nor$plane $sync$or$array "
            "$sync$or$plane $test$plusargs $time $timeformat $timeskew "
            "$ungetc $unsigned $value$plusargs $width $writeb $writeh $write "
            "$writeo";

    return 0;
}


// Return the string of characters that comprise a word.
const char *QsciLexerVerilog::wordCharacters() const
{
    return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";
}


// Returns the foreground colour of the text for a style.
QColor QsciLexerVerilog::defaultColor(int style) const
{
    switch (style)
    {
    case Default:
        return QColor(0x80,0x80,0x80);

    case Comment:
    case CommentLine:
        return QColor(0x00,0x7f,0x00);

    case CommentBang:
        return QColor(0x3f,0x7f,0x3f);

    case Number:
    case KeywordSet2:
        return QColor(0x00,0x7f,0x7f);

    case Keyword:
        return QColor(0x00,0x00,0x7f);

    case String:
        return QColor(0x7f,0x00,0x7f);

    case SystemTask:
    case UserKeywordSet:
        return QColor(0x80,0x40,0x20);

    case Preprocessor:
        return QColor(0x7f,0x7f,0x00);

    case Operator:
        return QColor(0x00,0x70,0x70);

    case UnclosedString:
        return QColor(0x00,0x00,0x00);
    }

    return QsciLexer::defaultColor(style);
}


// Returns the end-of-line fill for a style.
bool QsciLexerVerilog::defaultEolFill(int style) const
{
    if (style == CommentBang || style == UnclosedString)
        return true;

    return QsciLexer::defaultEolFill(style);
}


// Returns the font of the text for a style.
QFont QsciLexerVerilog::defaultFont(int style) const
{
    QFont f;

    switch (style)
    {
    case Comment:
    case CommentLine:
    case CommentBang:
    case UserKeywordSet:
#if defined(Q_OS_WIN)
        f = QFont("Comic Sans MS",9);
#elif defined(Q_OS_MAC)
        f = QFont("Comic Sans MS", 12);
#else
        f = QFont("Bitstream Vera Serif",9);
#endif
        break;

    case Keyword:
    case Operator:
        f = QsciLexer::defaultFont(style);
        f.setBold(true);
        break;

    default:
        f = QsciLexer::defaultFont(style);
    }

    return f;
}


// Returns the user name of a style.
QString QsciLexerVerilog::description(int style) const
{
    switch (style)
    {
    case Default:
        return tr("Default");

    case Comment:
        return tr("Comment");

    case CommentLine:
        return tr("Line comment");

    case CommentBang:
        return tr("Bang comment");

    case Number:
        return tr("Number");

    case Keyword:
        return tr("Primary keywords and identifiers");

    case String:
        return tr("String");

    case KeywordSet2:
        return tr("Secondary keywords and identifiers");

    case SystemTask:
        return tr("System task");

    case Preprocessor:
        return tr("Preprocessor block");

    case Operator:
        return tr("Operator");

    case Identifier:
        return tr("Identifier");

    case UnclosedString:
        return tr("Unclosed string");

    case UserKeywordSet:
        return tr("User defined tasks and identifiers");
    }

    return QString();
}


// Returns the background colour of the text for a style.
QColor QsciLexerVerilog::defaultPaper(int style) const
{
    if (style == CommentBang)
        return QColor(0xe0,0xf0,0xff);

    if (style == UnclosedString)
        return QColor(0xe0,0xc0,0xe0);

    return QsciLexer::defaultPaper(style);
}


// Refresh all properties.
void QsciLexerVerilog::refreshProperties()
{
    setAtElseProp();
    setCommentProp();
    setCompactProp();
    setPreprocProp();
}


// Read properties from the settings.
bool QsciLexerVerilog::readProperties(QSettings &qs,const QString &prefix)
{
    int rc = true;

    fold_atelse = qs.value(prefix + "foldatelse", false).toBool();
    fold_comments = qs.value(prefix + "foldcomments", false).toBool();
    fold_compact = qs.value(prefix + "foldcompact", true).toBool();
    fold_preproc = qs.value(prefix + "foldpreprocessor", false).toBool();
    fold_atmodule = qs.value(prefix + "foldverilogflags", false).toBool();

    return rc;
}


// Write properties to the settings.
bool QsciLexerVerilog::writeProperties(QSettings &qs,const QString &prefix) const
{
    int rc = true;

    qs.setValue(prefix + "foldatelse", fold_atelse);
    qs.setValue(prefix + "foldcomments", fold_comments);
    qs.setValue(prefix + "foldcompact", fold_compact);
    qs.setValue(prefix + "foldpreprocessor", fold_preproc);
    qs.setValue(prefix + "foldverilogflags", fold_atmodule);

    return rc;
}


// Return true if else can be folded.
bool QsciLexerVerilog::foldAtElse() const
{
    return fold_atelse;
}


// Set if else can be folded.
void QsciLexerVerilog::setFoldAtElse(bool fold)
{
    fold_atelse = fold;

    setAtElseProp();
}


// Set the "fold.at.else" property.
void QsciLexerVerilog::setAtElseProp()
{
    emit propertyChanged("fold.at.else",(fold_atelse ? "1" : "0"));
}


// Return true if comments can be folded.
bool QsciLexerVerilog::foldComments() const
{
    return fold_comments;
}


// Set if comments can be folded.
void QsciLexerVerilog::setFoldComments(bool fold)
{
    fold_comments = fold;

    setCommentProp();
}


// Set the "fold.comment" property.
void QsciLexerVerilog::setCommentProp()
{
    emit propertyChanged("fold.comment",(fold_comments ? "1" : "0"));
}


// Return true if folds are compact.
bool QsciLexerVerilog::foldCompact() const
{
    return fold_compact;
}


// Set if folds are compact
void QsciLexerVerilog::setFoldCompact(bool fold)
{
    fold_compact = fold;

    setCompactProp();
}


// Set the "fold.compact" property.
void QsciLexerVerilog::setCompactProp()
{
    emit propertyChanged("fold.compact",(fold_compact ? "1" : "0"));
}


// Return true if preprocessor blocks can be folded.
bool QsciLexerVerilog::foldPreprocessor() const
{
    return fold_preproc;
}


// Set if preprocessor blocks can be folded.
void QsciLexerVerilog::setFoldPreprocessor(bool fold)
{
    fold_preproc = fold;

    setPreprocProp();
}


// Set the "fold.preprocessor" property.
void QsciLexerVerilog::setPreprocProp()
{
    emit propertyChanged("fold.preprocessor",(fold_preproc ? "1" : "0"));
}


// Return true if modules can be folded.
bool QsciLexerVerilog::foldAtModule() const
{
    return fold_atmodule;
}


// Set if modules can be folded.
void QsciLexerVerilog::setFoldAtModule(bool fold)
{
    fold_atmodule = fold;

    setAtModuleProp();
}


// Set the "fold.verilog.flags" property.
void QsciLexerVerilog::setAtModuleProp()
{
    emit propertyChanged("fold.verilog.flags",(fold_atmodule ? "1" : "0"));
}
