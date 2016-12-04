#pragma once
#include <string>

#include "robomongo/core/mongodb/ReplicaSet.h"

namespace Robomongo
{
    /**
     * @brief Error object is a part of every Event in Robomongo. Before the
     * use of any event you should check whether error is detected by
     * calling Event::isError() method.
     */
    class EventError
    {

    public:
        // Todo
        enum ErrorCode { Unknown, SetPrimaryUnreachable };

        /**
         * @brief Creates "null" (or, in other words, empty) error.
         * Subsequent call of isNull() on this object will return true.
         */
        EventError();

        /**
         * @brief Creates error object with specified error message.
         * Subsequent call of isNull() on this object will return false.
         */
        explicit EventError(const std::string &errorMessage, ErrorCode errorCode = Unknown);
        
        // todo
        explicit EventError(const std::string &errorMessage, ReplicaSet replicaSetInfo, 
                            bool showErrorWindow = true);

        /**
         * @brief Tests whether error is "null" (or, in other words, empty).
         * Because Error object should be created on stack, this is the only
         * way to support "null" semantic for value objects.
         * @return true, if null or false otherwise.
         */
        bool isNull() const;

        /**
         * @brief Returns error message that describes this error.
         */
        const std::string &errorMessage() const;
        
        // todo
        ErrorCode errorCode() const { return _errorCode; }
        ReplicaSet replicaSetInfo() const { return _replicaSetInfo; }
        bool showErrorWindow() const { return _showErrorWindow; }

    private:
        /**
         * @brief Error message
         */
        const std::string _errorMessage;
        const ErrorCode _errorCode = Unknown;   // todo: not needed?
        ReplicaSet _replicaSetInfo;
        bool _showErrorWindow = true;
        bool _isNull;
    };
}
