// This implements the QScintilla plugin for Qt Designer.


#include <qwidgetplugin.h>

#include <Qsci/qsciscintilla.h>


static const char *qscintilla_pixmap[] = {
	"22 22 35 1",
	"m c #000000",
	"n c #000033",
	"p c #003300",
	"r c #003333",
	"v c #330000",
	"o c #330033",
	"l c #333300",
	"h c #333333",
	"c c #333366",
	"d c #336666",
	"u c #336699",
	"E c #3366cc",
	"k c #663333",
	"i c #663366",
	"b c #666666",
	"e c #666699",
	"A c #6666cc",
	"G c #669966",
	"f c #669999",
	"j c #6699cc",
	"y c #6699ff",
	"t c #996666",
	"a c #999999",
	"g c #9999cc",
	"s c #9999ff",
	"C c #99cc99",
	"x c #99cccc",
	"w c #99ccff",
	"F c #cc99ff",
	"q c #cccccc",
	"# c #ccccff",
	"B c #ccffcc",
	"z c #ccffff",
	"D c #ffffcc",
	". c none",
	"........#abcda........",
	"......abefghdidcf.....",
	".....cadhfaehjheck....",
	"....leh.m.ncbehjddo...",
	"...depn.hqhqhr#mccch..",
	"..bb.hcaeh.hqersjhjcd.",
	".tcm.uqn.hc.uvwxhuygha",
	".feh.n.hb.hhzemcwhmuAm",
	"Bgehghqqme.eo#wlnysbnj",
	"awhdAzn.engjepswhmuyuj",
	"bCh#m.de.jpqwbmcwemlcz",
	"hcb#xh.nd#qrbswfehwzbm",
	"bd#d.A#zor#qmgbzwgjgws",
	"ajbcuqhqzchwwbemewchmr",
	"Dcn#cwmhgwehgsxbmhEjAc",
	".uanauFrhbgeahAAbcbuhh",
	".bohdAegcccfbbebuucmhe",
	"..briuauAediddeclchhh.",
	"...hcbhjccdecbceccch..",
	"....nhcmeccdccephcp...",
	".....crbhchhhrhhck....",
	"......tcmdhohhcnG....."
};


class QScintillaPlugin : public QWidgetPlugin
{
public:
	QScintillaPlugin() {};

	QStringList keys() const;
	QWidget *create(const QString &classname, QWidget *parent = 0, const char *name = 0);
	QString group(const QString &) const;
	QIconSet iconSet(const QString &) const;
	QString includeFile(const QString &) const;
	QString toolTip(const QString &) const;
	QString whatsThis(const QString &) const;
	bool isContainer(const QString &) const;
};


QStringList QScintillaPlugin::keys() const
{
	QStringList list;

	list << "QsciScintilla";

	return list;
}


QWidget *QScintillaPlugin::create(const QString &key, QWidget *parent, const char *name)
{
	if (key == "QsciScintilla")
		return new QsciScintilla(parent, name);

	return 0;
}


QString QScintillaPlugin::group(const QString &feature) const
{
	if (feature == "QsciScintilla")
		return "Input";

	return QString::null;
}


QIconSet QScintillaPlugin::iconSet(const QString &) const
{
	return QIconSet(QPixmap(qscintilla_pixmap));
}


QString QScintillaPlugin::includeFile(const QString &feature) const
{
	if (feature == "QsciScintilla")
		return "qsciscintilla.h";

	return QString::null;
}


QString QScintillaPlugin::toolTip(const QString &feature) const
{
	if (feature == "QsciScintilla")
		return "QScintilla Programmer's Editor";

	return QString::null;
}


QString QScintillaPlugin::whatsThis(const QString &feature) const
{
	if (feature == "QsciScintilla")
		return "A port to Qt of the Scintilla programmer's editor";

	return QString::null;
}


bool QScintillaPlugin::isContainer(const QString &) const
{
	return FALSE;
}


Q_EXPORT_PLUGIN(QScintillaPlugin)
