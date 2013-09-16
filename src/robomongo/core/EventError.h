#pragma once
#include <string>

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
        /**
         * @brief Creates "null" (or, in other words, empty) error.
         * Subsequent call of isNull() on this object will return true.
         */
        EventError();

        /**
         * @brief Creates error object with specified error message.
         * Subsequent call of isNull() on this object will return false.
         */
        explicit EventError(const std::string &errorMessage);

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

    private:
        /**
         * @brief Error message
         */
        const std::string _errorMessage;
    };
}
