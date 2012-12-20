#include "PlainJavaScriptEditor.h"
#include "jsedit.h"
#include <QPainter>
#include <QApplication>

using namespace Robomongo;

PlainJavaScriptEditor::PlainJavaScriptEditor(QWidget *parent) : JSEdit(parent)
{
    JSEdit *editor= this;

    // dark color scheme
    editor->setColor(JSEdit::Background,    QColor("#272822"));
    editor->setColor(JSEdit::Normal,        QColor("#FFFFFF"));
    editor->setColor(JSEdit::Comment,       QColor("#666666"));
    editor->setColor(JSEdit::Number,        QColor("#DBF76C"));
    editor->setColor(JSEdit::String,        QColor("#5ED363"));
    editor->setColor(JSEdit::Operator,      QColor("#FF7729"));
    editor->setColor(JSEdit::Identifier,    QColor("#FFFFFF"));
    editor->setColor(JSEdit::Keyword,       QColor("#FDE15D"));
    editor->setColor(JSEdit::BuiltIn,       QColor("#9CB6D4"));
    editor->setColor(JSEdit::Cursor,        QColor("#272822"));
    editor->setColor(JSEdit::Marker,        QColor("#DBF76C"));
    editor->setColor(JSEdit::BracketMatch,  QColor("#1AB0A6"));
    editor->setColor(JSEdit::BracketError,  QColor("#A82224"));
    editor->setColor(JSEdit::FoldIndicator, QColor("#555555"));

    QStringList keywords = editor->keywords();
    keywords << "db";
    keywords << "find";
    keywords << "limit";
    keywords << "forEach";
    editor->setKeywords(keywords);
}


RoboScintilla::RoboScintilla(QWidget *parent) : QsciScintilla(parent)
{
    setViewportMargins(3,3,3,3);
}

void RoboScintilla::paintEvent(QPaintEvent *e)
{
//    QFrame::paintEvent(e);
    QsciScintilla::paintEvent(e);
    //e->accept();

//    QRect region = e->rect();
//    QPainter painter(this);
//    QPen pen(Qt::red); //Note: set line colour like this

//    //(Brush line removed; not necessary when drawing a line)
//    int x = 0; //Note changed
//    int y = height() / 2; //Note changed
//    pen.setWidth(2);
//    painter.setPen(pen);
    //    painter.drawRoundedRect(0, 0, width(), height(), 5, 5);
}

void RoboScintilla::wheelEvent(QWheelEvent *e)
{
    if (this->isActiveWindow())
        QsciScintilla::wheelEvent(e);
    else
    {
        qApp->sendEvent(this->parentWidget(), e);
        e->accept();
    }
}

void RoboScintilla::keyPressEvent(QKeyEvent *keyEvent)
{
    if ((keyEvent->modifiers() & Qt::ControlModifier) &&
        (keyEvent->key()==Qt::Key_F4 || keyEvent->key()==Qt::Key_W))
    {
        keyEvent->ignore();
        //QApplication::sendEvent(parentWidget(), keyEvent);
    } else
        QsciScintilla::keyPressEvent(keyEvent);
}




/*
 *
 *BOOL Style_SelectFont(HWND hwnd,LPWSTR lpszStyle,int cchStyle,BOOL bDefaultStyle)
{
  CHOOSEFONT cf;
  LOGFONT lf;
  WCHAR szNewStyle[512];
  int  iValue;
  WCHAR tch[32];
  HDC hdc;

  ZeroMemory(&cf,sizeof(CHOOSEFONT));
  ZeroMemory(&lf,sizeof(LOGFONT));

  // Map lpszStyle to LOGFONT
  if (Style_StrGetFont(lpszStyle,tch,COUNTOF(tch)))
    lstrcpyn(lf.lfFaceName,tch,COUNTOF(lf.lfFaceName));
  if (Style_StrGetCharSet(lpszStyle,&iValue))
    lf.lfCharSet = iValue;
  if (Style_StrGetSize(lpszStyle,&iValue)) {
    hdc = GetDC(hwnd);
    lf.lfHeight = -MulDiv(iValue,GetDeviceCaps(hdc,LOGPIXELSY),72);
    ReleaseDC(hwnd,hdc);
  }
  lf.lfWeight = (StrStrI(lpszStyle,L"bold")) ? FW_BOLD : FW_NORMAL;
  lf.lfItalic = (StrStrI(lpszStyle,L"italic")) ? 1 : 0;

  // Init cf
  cf.lStructSize = sizeof(CHOOSEFONT);
  cf.hwndOwner = hwnd;
  cf.lpLogFont = &lf;
  cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

  if (HIBYTE(GetKeyState(VK_SHIFT)))
    cf.Flags |= CF_FIXEDPITCHONLY;

  if (!ChooseFont(&cf) || !lstrlen(lf.lfFaceName))
    return FALSE;

  // Map back to lpszStyle
  lstrcpy(szNewStyle,L"font:");
  lstrcat(szNewStyle,lf.lfFaceName);
  if (Style_StrGetFontQuality(lpszStyle,tch,COUNTOF(tch)))
  {
    lstrcat(szNewStyle,L"; smoothing:");
    lstrcat(szNewStyle,tch);
  }
  if (bDefaultStyle &&
      lf.lfCharSet != DEFAULT_CHARSET &&
      lf.lfCharSet != ANSI_CHARSET &&
      lf.lfCharSet != iDefaultCharSet) {
    lstrcat(szNewStyle,L"; charset:");
    wsprintf(tch,L"%i",lf.lfCharSet);
    lstrcat(szNewStyle,tch);
  }
  lstrcat(szNewStyle,L"; size:");
  wsprintf(tch,L"%i",cf.iPointSize/10);
  lstrcat(szNewStyle,tch);
  if (cf.nFontType & BOLD_FONTTYPE)
    lstrcat(szNewStyle,L"; bold");
  if (cf.nFontType & ITALIC_FONTTYPE)
    lstrcat(szNewStyle,L"; italic");

  if (StrStrI(lpszStyle,L"underline"))
    lstrcat(szNewStyle,L"; underline");

  // save colors
  if (Style_StrGetColor(TRUE,lpszStyle,&iValue))
  {
    wsprintf(tch,L"; fore:#%02X%02X%02X",
      (int)GetRValue(iValue),
      (int)GetGValue(iValue),
      (int)GetBValue(iValue));
    lstrcat(szNewStyle,tch);
  }
  if (Style_StrGetColor(FALSE,lpszStyle,&iValue))
  {
    wsprintf(tch,L"; back:#%02X%02X%02X",
      (int)GetRValue(iValue),
      (int)GetGValue(iValue),
      (int)GetBValue(iValue));
    lstrcat(szNewStyle,tch);
  }

  if (StrStrI(lpszStyle,L"eolfilled"))
    lstrcat(szNewStyle,L"; eolfilled");

  if (Style_StrGetCase(lpszStyle,&iValue)) {
    lstrcat(szNewStyle,L"; case:");
    lstrcat(szNewStyle,(iValue == SC_CASE_UPPER) ? L"u" : L"");
  }

  if (Style_StrGetAlpha(lpszStyle,&iValue)) {
    lstrcat(szNewStyle,L"; alpha:");
    wsprintf(tch,L"%i",iValue);
    lstrcat(szNewStyle,tch);
  }

  lstrcpyn(lpszStyle,szNewStyle,cchStyle);
  return TRUE;
}
  */
