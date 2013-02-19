// AUTO-GENERATED FILE DO NOT EDIT
// See src/mongo/base/generate_error_codes.py
/*    Copyright 2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "mongo/base/string_data.h"

namespace mongo {

    /**
     * This is a generated class containing a table of error codes and their corresponding error
     * strings. The class is derived from the definitions in src/mongo/base/error_codes.err file.
     *
     * Do not update this file directly. Update src/mongo/base/error_codes.err instead.
     */

    class ErrorCodes {
    public:
        enum Error {
            OK = 0,
            InternalError = 1,
            BadValue = 2,
            DuplicateKey = 3,
            NoSuchKey = 4,
            GraphContainsCycle = 5,
            HostUnreachable = 6,
            HostNotFound = 7,
            UnknownError = 8,
            FailedToParse = 9,
            CannotMutateObject = 10,
            UserNotFound = 11,
            UnsupportedFormat = 12,
            Unauthorized = 13,
            TypeMismatch = 14,
            Overflow = 15,
            InvalidLength = 16,
            ProtocolError = 17,
            AuthenticationFailed = 18,
            CannotReuseObject = 19,
            IllegalOperation = 20,
            EmptyArrayOperation = 21,
            InvalidBSON = 22,
            AlreadyInitialized = 23,
            LockTimeout = 24,
            RemoteValidationError = 25,
            MaxError
        };

        static const char* errorString(Error err);

        /**
         * Parse an Error from its "name".  Returns UnknownError if "name" is unrecognized.
         *
         * NOTE: Also returns UnknownError for the string "UnknownError".
         */
        static Error fromString(const StringData& name);

        /**
         * Parse an Error from its "code".  Returns UnknownError if "code" is unrecognized.
         *
         * NOTE: Also returns UnknownError for the integer code for UnknownError.
         */
        static Error fromInt(int code);

        static bool isNetworkError(Error err);
    };

}  // namespace mongo
