/*    Copyright 2012 10gen Inc.
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

/**
 * Utility macros for declaring global initializers
 *
 * Should NOT be included by other header files.  Include only in source files.
 *
 * Initializers are arranged in an acyclic directed dependency graph.  Declaring
 * a cycle will lead to a runtime error.
 *
 * Initializer functions take a parameter of type ::mongo::InitializerContext*, and return
 * a Status.  Any status other than Status::OK() is considered a failure that will stop further
 * intializer processing.
 */

#pragma once

#include "mongo/base/initializer.h"
#include "mongo/base/initializer_context.h"
#include "mongo/base/initializer_function.h"
#include "mongo/base/global_initializer.h"
#include "mongo/base/global_initializer_registerer.h"
#include "mongo/base/make_string_vector.h"
#include "mongo/base/status.h"

/**
 * Convenience parameter representing an empty set of prerequisites for an initializer function.
 */
#define MONGO_NO_PREREQUISITES (NULL)

/**
 * Convenience parameter representing an empty set of dependents of an initializer function.
 */
#define MONGO_NO_DEPENDENTS (NULL)

/**
 * Convenience parameter representing the default set of dependents for initializer functions.
 */
#define MONGO_DEFAULT_PREREQUISITES ("default")

/**
 * Macro to define an initializer function named "NAME" with the default prerequisites, and
 * no explicit dependents.
 *
 * See MONGO_INITIALIZER_GENERAL.
 *
 * Usage:
 *     MONGO_INITIALIZER(myModule)(::mongo::InitializerContext* context) {
 *         ...
 *     }
 */
#define MONGO_INITIALIZER(NAME) \
    MONGO_INITIALIZER_WITH_PREREQUISITES(NAME, MONGO_DEFAULT_PREREQUISITES)

/**
 * Macro to define an initializer function named "NAME" that depends on the initializers
 * specified in PREREQUISITES to have been completed, but names no explicit dependents.
 *
 * See MONGO_INITIALIZER_GENERAL.
 *
 * Usage:
 *     MONGO_INITIALIZER_WITH_PREREQUISITES(myGlobalStateChecker,
 *                                         ("globalStateInitialized", "stacktraces"))(
 *            ::mongo::InitializerContext* context) {
 *    }
 */
#define MONGO_INITIALIZER_WITH_PREREQUISITES(NAME, PREREQUISITES) \
    MONGO_INITIALIZER_GENERAL(NAME, PREREQUISITES, MONGO_NO_DEPENDENTS)

/**
 * Macro to define an initializer that depends on PREREQUISITES and has DEPENDENTS as explicit
 * dependents.
 *
 * NAME is any legitimate name for a C++ symbol.
 * PREREQUISITES is a tuple of 0 or more std::string literals, i.e., ("a", "b", "c"), or ()
 * DEPENDENTS is a tuple of 0 or more std::string literals.
 *
 * At run time, the full set of prerequisites for NAME will be computed as the union of the
 * explicit PREREQUISITES and the set of all other mongo initializers that name NAME in their
 * list of dependents.
 *
 * Usage:
 *    MONGO_INITIALIZER_GENERAL(myInitializer,
 *                             ("myPrereq1", "myPrereq2", ...),
 *                             ("myDependent1", "myDependent2", ...))(
 *            ::mongo::InitializerContext* context) {
 *    }
 *
 * TODO: May want to be able to name the initializer separately from the function name.
 * A form that takes an existing function or that lets the programmer supply the name
 * of the function to declare would be options.
 */
#define MONGO_INITIALIZER_GENERAL(NAME, PREREQUISITES, DEPENDENTS)                        \
    ::mongo::Status _MONGO_INITIALIZER_FUNCTION_NAME(NAME)(::mongo::InitializerContext*); \
    namespace {                                                                           \
    ::mongo::GlobalInitializerRegisterer _mongoInitializerRegisterer_##NAME(              \
        #NAME,                                                                            \
        _MONGO_INITIALIZER_FUNCTION_NAME(NAME),                                           \
        MONGO_MAKE_STRING_VECTOR PREREQUISITES,                                           \
        MONGO_MAKE_STRING_VECTOR DEPENDENTS);                                             \
    }                                                                                     \
    ::mongo::Status _MONGO_INITIALIZER_FUNCTION_NAME(NAME)

/**
 * Macro to define an initializer group.
 *
 * An initializer group is an initializer that performs no actions.  It is useful for organizing
 * initialization steps into phases, such as "all global parameter declarations completed", "all
 * global parameters initialized".
 */
#define MONGO_INITIALIZER_GROUP(NAME, PREREQUISITES, DEPENDENTS)                               \
    MONGO_INITIALIZER_GENERAL(NAME, PREREQUISITES, DEPENDENTS)(::mongo::InitializerContext*) { \
        return ::mongo::Status::OK();                                                          \
    }

/**
 * Macro to produce a name for a mongo initializer function for an initializer operation
 * named "NAME".
 */
#define _MONGO_INITIALIZER_FUNCTION_NAME(NAME) _mongoInitializerFunction_##NAME
