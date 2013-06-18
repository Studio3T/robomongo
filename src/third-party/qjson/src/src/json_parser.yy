/* This file is part of QJSon
  *
  * Copyright (C) 2008 Flavio Castelli <flavio.castelli@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License version 2.1, as published by the Free Software Foundation.
  * 
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

%skeleton "lalr1.cc"
%defines
%define "parser_class_name" "json_parser"

%{
  #include "parser_p.h"
  #include "json_scanner.h"
  #include "qjson_debug.h"

  #include <QtCore/QByteArray>
  #include <QtCore/QMap>
  #include <QtCore/QString>
  #include <QtCore/QVariant>

  #include <limits>

  class JSonScanner;

  namespace QJson {
    class Parser;
  }

  #define YYERROR_VERBOSE 1
%}

%parse-param { QJson::ParserPrivate* driver }
%lex-param { QJson::ParserPrivate* driver }

%locations

%debug
%error-verbose

%token END 0 "end of file"

%token CURLY_BRACKET_OPEN 1 "{"
%token CURLY_BRACKET_CLOSE 2 "}"
%token SQUARE_BRACKET_OPEN 3 "["
%token SQUARE_BRACKET_CLOSE 4 "]"

%token COLON 5 ":"
%token COMMA 6 ","
%token MINUS 7 "-"
%token DOT 8 "."
%token DIGIT 9 "digit"
%token E 10 "exponential"
%token TRUE_VAL 11 "true"
%token FALSE_VAL 12 "false"
%token NULL_VAL 13 "null"
%token QUOTMARKOPEN 14 "open quotation mark"
%token QUOTMARKCLOSE 15 "close quotation mark"

%token STRING 16 "string"
%token INFINITY_VAL 17 "Infinity"
%token NAN_VAL 18 "NaN"

// define the initial token
%start start

%%

// grammar rules

start: data {
              driver->m_result = $1;
              qjsonDebug() << "json_parser - parsing finished";
            };

data: value { $$ = $1; }
      | error
          {
            qCritical()<< "json_parser - syntax error found, "
                    << "forcing abort, Line" << @$.begin.line << "Column" << @$.begin.column;
            YYABORT;
          }
      | END;

object: CURLY_BRACKET_OPEN members CURLY_BRACKET_CLOSE { $$ = $2; };

members: /* empty */ { $$ = QVariant (QVariantMap()); }
        | pair r_members {
            QVariantMap members = $2.toMap();
            $2 = QVariant(); // Allow reuse of map
            $$ = QVariant(members.unite ($1.toMap()));
          };

r_members: /* empty */ { $$ = QVariant (QVariantMap()); }
        | COMMA pair r_members {
          QVariantMap members = $3.toMap();
          $3 = QVariant(); // Allow reuse of map
          $$ = QVariant(members.unite ($2.toMap()));
          };

pair:   string COLON value {
            QVariantMap pair;
            pair.insert ($1.toString(), QVariant($3));
            $$ = QVariant (pair);
          };

array: SQUARE_BRACKET_OPEN values SQUARE_BRACKET_CLOSE { $$ = $2; };

values: /* empty */ { $$ = QVariant (QVariantList()); }
        | value r_values {
          QVariantList members = $2.toList();
          $2 = QVariant(); // Allow reuse of list
          members.prepend ($1);
          $$ = QVariant(members);
        };

r_values: /* empty */ { $$ = QVariant (QVariantList()); }
          | COMMA value r_values {
            QVariantList members = $3.toList();
            $3 = QVariant(); // Allow reuse of list
            members.prepend ($2);
            $$ = QVariant(members);
          };

value: string { $$ = $1; }
        | special_or_number { $$ = $1; }
        | object { $$ = $1; }
        | array { $$ = $1; }
        | TRUE_VAL { $$ = QVariant (true); }
        | FALSE_VAL { $$ = QVariant (false); }
        | NULL_VAL {
          QVariant null_variant;
          $$ = null_variant;
        };

special_or_number: MINUS INFINITY_VAL { $$ = QVariant(QVariant::Double); $$.setValue( -std::numeric_limits<double>::infinity() ); }
                   | INFINITY_VAL { $$ = QVariant(QVariant::Double); $$.setValue( std::numeric_limits<double>::infinity() ); }
                   | NAN_VAL { $$ = QVariant(QVariant::Double); $$.setValue( std::numeric_limits<double>::quiet_NaN() ); }
                   | number;

number: int {
            if ($1.toByteArray().startsWith('-')) {
              $$ = QVariant (QVariant::LongLong);
              $$.setValue($1.toLongLong());
            }
            else {
              $$ = QVariant (QVariant::ULongLong);
              $$.setValue($1.toULongLong());
            }
          }
        | int fract {
            const QByteArray value = $1.toByteArray() + $2.toByteArray();
            $$ = QVariant(QVariant::Double);
            $$.setValue(value.toDouble());
          }
        | int exp { $$ = QVariant ($1.toByteArray() + $2.toByteArray()); }
        | int fract exp {
            const QByteArray value = $1.toByteArray() + $2.toByteArray() + $3.toByteArray();
            $$ = QVariant (value);
          };

int:  DIGIT digits { $$ = QVariant ($1.toByteArray() + $2.toByteArray()); }
      | MINUS DIGIT digits { $$ = QVariant (QByteArray("-") + $2.toByteArray() + $3.toByteArray()); };

digits: /* empty */ { $$ = QVariant (QByteArray("")); }
        | DIGIT digits {
          $$ = QVariant($1.toByteArray() + $2.toByteArray());
        };

fract: DOT digits {
          $$ = QVariant(QByteArray(".") + $2.toByteArray());
        };

exp: E digits { $$ = QVariant($1.toByteArray() + $2.toByteArray()); };

string: QUOTMARKOPEN string_arg QUOTMARKCLOSE { $$ = $2; };

string_arg: /*empty */ { $$ = QVariant (QString(QLatin1String(""))); }
            | STRING {
                $$ = $1;
              };

%%

int yy::yylex(YYSTYPE *yylval, yy::location *yylloc, QJson::ParserPrivate* driver)
{
  JSonScanner* scanner = driver->m_scanner;
  yylval->clear();
  int ret = scanner->yylex(yylval, yylloc);

  qjsonDebug() << "json_parser::yylex - calling scanner yylval==|"
           << yylval->toByteArray() << "|, ret==|" << QString::number(ret) << "|";
  
  return ret;
}

void yy::json_parser::error (const yy::location& yyloc,
                                 const std::string& error)
{
  /*qjsonDebug() << yyloc.begin.line;
  qjsonDebug() << yyloc.begin.column;
  qjsonDebug() << yyloc.end.line;
  qjsonDebug() << yyloc.end.column;*/
  qjsonDebug() << "json_parser::error [line" << yyloc.end.line << "] -" << error.c_str() ;
  driver->setError(QString::fromLatin1(error.c_str()), yyloc.end.line);
}
