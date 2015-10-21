#!/usr/bin/python

#    Copyright 2012 10gen Inc.
#
#    This program is free software: you can redistribute it and/or  modify
#    it under the terms of the GNU Affero General Public License, version 3,
#    as published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#    As a special exception, the copyright holders give permission to link the
#    code of portions of this program with the OpenSSL library under certain
#    conditions as described in each individual source file and distribute
#    linked combinations including the program with the OpenSSL library. You
#    must comply with the GNU Affero General Public License in all respects
#    for all of the code used other than as permitted herein. If you modify
#    file(s) with this exception, you may extend this exception to your
#    version of the file(s), but you are not obligated to do so. If you do not
#    wish to do so, delete this exception statement from your version. If you
#    delete this exception statement from all source files in the program,
#    then also delete it in the license file.

"""Generate error_codes.{h,cpp} from error_codes.err.

Format of error_codes.err:

error_code("symbol1", code1)
error_code("symbol2", code2)
...
error_class("class1", ["symbol1", "symbol2, ..."])

Usage:
    python generate_error_codes.py <path to error_codes.err> <header file path> <source file path>
"""

import sys

def main(argv):
    if len(argv) != 4:
        usage("Wrong number of arguments.")

    error_codes, error_classes = parse_error_definitions_from_file(argv[1])
    check_for_conflicts(error_codes, error_classes)
    generate_header(argv[2], error_codes, error_classes)
    generate_source(argv[3], error_codes, error_classes)

def die(message=None):
    sys.stderr.write(message or "Fatal error\n")
    sys.exit(1)

def usage(message=None):
    sys.stderr.write(__doc__)
    die(message)

def parse_error_definitions_from_file(errors_filename):
    errors_file = open(errors_filename, 'r')
    errors_code = compile(errors_file.read(), errors_filename, 'exec')
    error_codes = []
    error_classes = []
    eval(errors_code, dict(error_code=lambda *args: error_codes.append(args),
                           error_class=lambda *args: error_classes.append(args)))
    error_codes.sort(key=lambda x: x[1])
    return error_codes, error_classes

def check_for_conflicts(error_codes, error_classes):
    failed = has_duplicate_error_codes(error_codes)
    if has_duplicate_error_classes(error_classes):
        failed = True
    if has_missing_error_codes(error_codes, error_classes):
        failed = True
    if failed:
        die()

def has_duplicate_error_codes(error_codes):
    sorted_by_name = sorted(error_codes, key=lambda x: x[0])
    sorted_by_code = sorted(error_codes, key=lambda x: x[1])

    failed = False
    prev_name, prev_code = sorted_by_name[0]
    for name, code in sorted_by_name[1:]:
        if name == prev_name:
            sys.stdout.write('Duplicate name %s with codes %s and %s\n' % (name, code, prev_code))
            failed = True
        prev_name, prev_code = name, code

    prev_name, prev_code = sorted_by_code[0]
    for name, code in sorted_by_code[1:]:
        if code == prev_code:
            sys.stdout.write('Duplicate code %s with names %s and %s\n' % (code, name, prev_name))
            failed = True
        prev_name, prev_code = name, code

    return failed

def has_duplicate_error_classes(error_classes):
    names = sorted(ec[0] for ec in error_classes)

    failed = False
    prev_name = names[0]
    for name in names[1:]:
        if prev_name == name:
            sys.stdout.write('Duplicate error class name %s\n' % name)
            failed = True
        prev_name = name
    return failed

def has_missing_error_codes(error_codes, error_classes):
    code_names = set(ec[0] for ec in error_codes)
    failed = False
    for class_name, class_code_names in error_classes:
        for name in class_code_names:
            if name not in code_names:
                sys.stdout.write('Undeclared error code %s in class %s\n' % (name, class_name))
                failed = True
    return failed

def generate_header(filename, error_codes, error_classes):

    enum_declarations = ',\n            '.join('%s = %s' % ec for ec in error_codes)
    predicate_declarations = ';\n        '.join(
        'static bool is%s(Error err)' % ec[0] for ec in error_classes)

    open(filename, 'wb').write(header_template % dict(
            error_code_enum_declarations=enum_declarations,
            error_code_class_predicate_declarations=predicate_declarations))

def generate_source(filename, error_codes, error_classes):
    symbol_to_string_cases = ';\n        '.join(
        'case %s: return "%s"' % (ec[0], ec[0]) for ec in error_codes)
    string_to_symbol_cases = ';\n        '.join(
        'if (name == "%s") return %s' % (ec[0], ec[0])
        for ec in error_codes)
    int_to_symbol_cases = ';\n        '.join(
        'case %s: return %s' % (ec[0], ec[0]) for ec in error_codes)
    predicate_definitions = '\n    '.join(
        generate_error_class_predicate_definition(*ec) for ec in error_classes)
    open(filename, 'wb').write(source_template % dict(
            symbol_to_string_cases=symbol_to_string_cases,
            string_to_symbol_cases=string_to_symbol_cases,
            int_to_symbol_cases=int_to_symbol_cases,
            error_code_class_predicate_definitions=predicate_definitions))

def generate_error_class_predicate_definition(class_name, code_names):
    cases = '\n        '.join('case %s:' % c for c in code_names)
    return error_class_predicate_template % dict(class_name=class_name, cases=cases)

header_template = '''// AUTO-GENERATED FILE DO NOT EDIT
// See src/mongo/base/generate_error_codes.py
/*    Copyright 2014 MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include <string>

#include "mongo/base/string_data.h"
#include "mongo/client/export_macros.h"

namespace mongo {

    /**
     * This is a generated class containing a table of error codes and their corresponding error
     * strings. The class is derived from the definitions in src/mongo/base/error_codes.err file.
     *
     * Do not update this file directly. Update src/mongo/base/error_codes.err instead.
     */

    class MONGO_CLIENT_API ErrorCodes {
    public:
        enum Error {
            %(error_code_enum_declarations)s,
            MaxError
        };

        static std::string errorString(Error err);

        /**
         * Parses an Error from its "name".  Returns UnknownError if "name" is unrecognized.
         *
         * NOTE: Also returns UnknownError for the string "UnknownError".
         */
        static Error fromString(const StringData& name);

        /**
         * Casts an integer "code" to an Error.  Unrecognized codes are preserved, meaning
         * that the result of a call to fromInt() may not be one of the values in the
         * Error enumeration.
         */
        static Error fromInt(int code);

        %(error_code_class_predicate_declarations)s;
    };

}  // namespace mongo
'''

source_template = '''// AUTO-GENERATED FILE DO NOT EDIT
// See src/mongo/base/generate_error_codes.py
/*    Copyright 2014 MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#include "mongo/base/error_codes.h"

#include <boost/static_assert.hpp>

#include "mongo/util/mongoutils/str.h"

namespace mongo {

    std::string ErrorCodes::errorString(Error err) {
        switch (err) {
        %(symbol_to_string_cases)s;
        default: return mongoutils::str::stream() << "Location" << err;
        }
    }

    ErrorCodes::Error ErrorCodes::fromString(const StringData& name) {
        %(string_to_symbol_cases)s;
        return UnknownError;
    }

    ErrorCodes::Error ErrorCodes::fromInt(int code) {
        return static_cast<Error>(code);
    }

    %(error_code_class_predicate_definitions)s

namespace {
    BOOST_STATIC_ASSERT(sizeof(ErrorCodes::Error) == sizeof(int));
}  // namespace
}  // namespace mongo
'''

error_class_predicate_template = '''bool ErrorCodes::is%(class_name)s(Error err) {
        switch (err) {
        %(cases)s
            return true;
        default:
            return false;
        }
    }
'''
if __name__ == '__main__':
    main(sys.argv)
