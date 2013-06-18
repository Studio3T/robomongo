/* This file is part of QJson
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

#ifndef _JSON_SCANNER
#define _JSON_SCANNER

#include <fstream>
#include <string>

#include <QtCore/QIODevice>
#include <QtCore/QVariant>

#define YYSTYPE QVariant

#include "parser_p.h"

namespace yy {
  class location;
  int yylex(YYSTYPE *yylval, yy::location *yylloc, QJson::ParserPrivate* driver);
}

class JSonScanner
{
    public:
        explicit JSonScanner(QIODevice* io);
        int yylex(YYSTYPE* yylval, yy::location *yylloc);
        void allowSpecialNumbers(bool allow);

    protected:
        bool m_quotmarkClosed;
        bool m_allowSpecialNumbers;
        unsigned int m_quotmarkCount;
        QIODevice* m_io;
};

#endif

