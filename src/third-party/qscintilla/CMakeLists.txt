# CMake script is written according to provided qmake file:
# sources/Qt4Qt4/qscintilla.pro

project(QScintilla)

# Automatically generate code for Qt moc, uic and rcc files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# For some reasons AUTOMOC doesn't work here...
# Doing this manually.
qt5_wrap_cpp(moc
    sources/Qt4Qt5/Qsci/qsciabstractapis.h
    sources/Qt4Qt5/Qsci/qsciapis.h
    sources/Qt4Qt5/Qsci/qscilexer.h
    sources/Qt4Qt5/Qsci/qscilexercpp.h
    sources/Qt4Qt5/Qsci/qscilexerjavascript.h
    sources/Qt4Qt5/Qsci/qscimacro.h
    sources/Qt4Qt5/Qsci/qsciscintilla.h
    sources/Qt4Qt5/Qsci/qsciscintillabase.h
)

add_library(qscintilla
    ${moc}
    sources/lexlib/StyleContext.cpp
    sources/lexlib/WordList.cpp
    sources/lexlib/Accessor.cpp
    sources/lexlib/LexerBase.cpp
    sources/lexlib/LexerModule.cpp
    sources/lexlib/CharacterSet.cpp
    sources/lexlib/CharacterCategory.cpp
    sources/lexlib/PropSetSimple.cpp
    sources/lexlib/LexerSimple.cpp
    sources/lexlib/LexerNoExceptions.cpp

    sources/Qt4Qt5/qscilexer.cpp
    sources/Qt4Qt5/qscilexerjavascript.cpp
    sources/Qt4Qt5/qscilexercpp.cpp

    # We do not need these lexers
#    sources/Qt4Qt5/qscilexerpov.cpp
#    sources/Qt4Qt5/qscilexercss.cpp
#    sources/Qt4Qt5/qscilexerd.cpp
#    sources/Qt4Qt5/qscilexerspice.cpp
#    sources/Qt4Qt5/qscilexerbash.cpp
#    sources/Qt4Qt5/qscilexerdiff.cpp
#    sources/Qt4Qt5/qscilexerruby.cpp
#    sources/Qt4Qt5/qscilexerpostscript.cpp
#    sources/Qt4Qt5/qscilexerpascal.cpp
#    sources/Qt4Qt5/qscilexerbatch.cpp
#    sources/Qt4Qt5/qscilexerfortran.cpp
#    sources/Qt4Qt5/qscilexercoffeescript.cpp
#    sources/Qt4Qt5/qscilexertcl.cpp
#    sources/Qt4Qt5/qscilexerjava.cpp
#    sources/Qt4Qt5/qscilexercustom.cpp
#    sources/Qt4Qt5/qscilexercsharp.cpp
#    sources/Qt4Qt5/qscilexertex.cpp
#    sources/Qt4Qt5/qscilexerproperties.cpp
#    sources/Qt4Qt5/qscilexeravs.cpp
#    sources/Qt4Qt5/qscilexerpo.cpp
#    sources/Qt4Qt5/qscilexerfortran77.cpp
#    sources/Qt4Qt5/qscilexersql.cpp
#    sources/Qt4Qt5/qscilexerhtml.cpp
#    sources/Qt4Qt5/qscilexerperl.cpp
#    sources/Qt4Qt5/qscilexermatlab.cpp
#    sources/Qt4Qt5/qscilexervhdl.cpp
#    sources/Qt4Qt5/qscilexerpython.cpp
#    sources/Qt4Qt5/qscilexerxml.cpp
#    sources/Qt4Qt5/qscilexeroctave.cpp
#    sources/Qt4Qt5/qscilexerlua.cpp
#    sources/Qt4Qt5/qscilexeridl.cpp
#    sources/Qt4Qt5/qscilexercmake.cpp
#    sources/Qt4Qt5/qscilexerverilog.cpp
#    sources/Qt4Qt5/qscilexeryaml.cpp
#    sources/Qt4Qt5/qscilexermakefile.cpp

    sources/Qt4Qt5/qscistyle.cpp
    sources/Qt4Qt5/InputMethod.cpp
    sources/Qt4Qt5/qsciabstractapis.cpp
    sources/Qt4Qt5/ScintillaQt.cpp
    sources/Qt4Qt5/qscidocument.cpp
    sources/Qt4Qt5/qsciscintilla.cpp
    sources/Qt4Qt5/qsciscintillabase.cpp
    sources/Qt4Qt5/qscicommand.cpp
    sources/Qt4Qt5/ListBoxQt.cpp
    sources/Qt4Qt5/qscistyledtext.cpp
    sources/Qt4Qt5/SciClasses.cpp
    sources/Qt4Qt5/qsciprinter.cpp
    sources/Qt4Qt5/PlatQt.cpp
    sources/Qt4Qt5/MacPasteboardMime.cpp
    sources/Qt4Qt5/qscimacro.cpp
    sources/Qt4Qt5/qsciapis.cpp
    sources/Qt4Qt5/qscicommandset.cpp

    sources/lexers/LexCPP.cpp


    # We do not need these lexers
#    sources/lexers/LexFlagship.cpp
#    sources/lexers/LexMetapost.cpp
#    sources/lexers/LexOthers.cpp
#    sources/lexers/LexBasic.cpp
#    sources/lexers/LexTxt2tags.cpp
#    sources/lexers/LexDMAP.cpp
#    sources/lexers/LexPB.cpp
#    sources/lexers/LexNsis.cpp
#    sources/lexers/LexLaTeX.cpp
#    sources/lexers/LexProgress.cpp
#    sources/lexers/LexSQL.cpp
#    sources/lexers/LexTADS3.cpp
#    sources/lexers/LexTAL.cpp
#    sources/lexers/LexECL.cpp
#    sources/lexers/LexConf.cpp
#    sources/lexers/LexCSS.cpp
#    sources/lexers/LexHex.cpp
#    sources/lexers/LexPython.cpp
#    sources/lexers/LexRebol.cpp
#    sources/lexers/LexAda.cpp
#    sources/lexers/LexAVS.cpp
#    sources/lexers/LexHaskell.cpp
#    sources/lexers/LexAsm.cpp
#    sources/lexers/LexScriptol.cpp
#    sources/lexers/LexTCMD.cpp
#    sources/lexers/LexR.cpp
#    sources/lexers/LexSML.cpp
#    sources/lexers/LexPLM.cpp
#    sources/lexers/LexBash.cpp
#    sources/lexers/LexTeX.cpp
#    sources/lexers/LexPO.cpp
#    sources/lexers/LexKVIrc.cpp
#    sources/lexers/LexA68k.cpp
#    sources/lexers/LexOScript.cpp
#    sources/lexers/LexGui4Cli.cpp
#    sources/lexers/LexPascal.cpp
#    sources/lexers/LexTACL.cpp
#    sources/lexers/LexMagik.cpp
#    sources/lexers/LexPowerShell.cpp
#    sources/lexers/LexRuby.cpp
#    sources/lexers/LexInno.cpp
#    sources/lexers/LexBibTeX.cpp
#    sources/lexers/LexBaan.cpp
#    sources/lexers/LexCoffeeScript.cpp
#    sources/lexers/LexDMIS.cpp
#    sources/lexers/LexMatlab.cpp
#    sources/lexers/LexLisp.cpp
#    sources/lexers/LexSmalltalk.cpp
#    sources/lexers/LexSpecman.cpp
#    sources/lexers/LexCOBOL.cpp
#    sources/lexers/LexMarkdown.cpp
#    sources/lexers/LexMySQL.cpp
#    sources/lexers/LexVerilog.cpp
#    sources/lexers/LexASY.cpp
#    sources/lexers/LexMPT.cpp
#    sources/lexers/LexCLW.cpp
#    sources/lexers/LexKix.cpp
#    sources/lexers/LexAVE.cpp
#    sources/lexers/LexVB.cpp
#    sources/lexers/LexPowerPro.cpp
#    sources/lexers/LexAU3.cpp
#    sources/lexers/LexBullant.cpp
#    sources/lexers/LexTCL.cpp
#    sources/lexers/LexCsound.cpp
#    sources/lexers/LexNimrod.cpp
#    sources/lexers/LexHTML.cpp
#    sources/lexers/LexPOV.cpp
#    sources/lexers/LexOpal.cpp
#    sources/lexers/LexSpice.cpp
#    sources/lexers/LexModula.cpp
#    sources/lexers/LexRegistry.cpp
#    sources/lexers/LexFortran.cpp
#    sources/lexers/LexErlang.cpp
#    sources/lexers/LexCrontab.cpp
#    sources/lexers/LexMSSQL.cpp
#    sources/lexers/LexPerl.cpp
#    sources/lexers/LexLua.cpp
#    sources/lexers/LexYAML.cpp
#    sources/lexers/LexEiffel.cpp
#    sources/lexers/LexAbaqus.cpp
#    sources/lexers/LexAPDL.cpp
#    sources/lexers/LexSTTXT.cpp
#    sources/lexers/LexLout.cpp
#    sources/lexers/LexForth.cpp
#    sources/lexers/LexCmake.cpp
#    sources/lexers/LexRust.cpp
#    sources/lexers/LexCaml.cpp
#    sources/lexers/LexVisualProlog.cpp
#    sources/lexers/LexVHDL.cpp
#    sources/lexers/LexEScript.cpp
#    sources/lexers/LexAsn1.cpp
#    sources/lexers/LexSorcus.cpp
#    sources/lexers/LexGAP.cpp
#    sources/lexers/LexMMIXAL.cpp
#    sources/lexers/LexD.cpp
#    sources/lexers/LexPS.cpp

    sources/src/AutoComplete.cpp
    sources/src/CallTip.cpp
    sources/src/CaseConvert.cpp
    sources/src/CaseFolder.cpp
    sources/src/Catalogue.cpp
    sources/src/CellBuffer.cpp
    sources/src/CharClassify.cpp
    sources/src/ContractionState.cpp
    sources/src/Decoration.cpp
    sources/src/Document.cpp
    sources/src/Editor.cpp
    sources/src/ExternalLexer.cpp
    sources/src/Indicator.cpp
    sources/src/KeyMap.cpp
    sources/src/LineMarker.cpp
    sources/src/PerLine.cpp
    sources/src/PositionCache.cpp
    sources/src/RESearch.cpp
    sources/src/RunStyles.cpp
    sources/src/ScintillaBase.cpp
    sources/src/Selection.cpp
    sources/src/Style.cpp
    sources/src/UniConversion.cpp
    sources/src/ViewStyle.cpp
    sources/src/XPM.cpp
)

target_link_libraries(qscintilla
    PUBLIC
        Qt5::Widgets
        Qt5::PrintSupport)

if (SYSTEM_MACOSX)
    target_link_libraries(qscintilla PUBLIC Qt5::MacExtras)
endif()

target_include_directories(qscintilla
    PUBLIC
        sources/Qt4Qt5
        sources/include
        sources/lexlib
        sources/src)

target_compile_definitions(qscintilla
    PUBLIC
        SCINTILLA_QT
        SCI_LEXER)

# Project produces three types of warnings that are all not critical.
# We are turning them off to remove noise
if (COMPILER_CLANG OR COMPILER_GCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dangling-else -Wno-parentheses -Wno-switch")
elseif(COMPILER_MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4554 /wd4099")

    # Suppress the following warning: MacPasteboardMime.obj : warning LNK4221: This object
    # file does not define any previously undefined public symbols, so it will not be used
    # by any link operation that consumes this library
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
endif()
