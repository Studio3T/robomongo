#ifndef EVENTS_H
#define EVENTS_H

#include <QObject>
#include <QString>
#include <QEvent>

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
        EventError() :
            _isNull(true) {}

        /**
         * @brief Creates error object with specified error message.
         * Subsequent call of isNull() on this object will return false.
         */
        EventError(const QString &errorMessage) :
            _isNull(false),
            _errorMessage(errorMessage) {}

        /**
         * @brief Tests whether error is "null" (or, in other words, empty).
         * Because Error object should be created on stack, this is the only
         * way to support "null" semantic for value objects.
         * @return true, if null or false otherwise.
         */
        bool isNull() const { return _isNull; }

        /**
         * @brief Returns error message that describes this error.
         */
        QString errorMessage() const { return _errorMessage; }

    private:

        /**
         * @brief Flag to support "null" semantic for value object.
         */
        bool _isNull;

        /**
         * @brief Error message
         */
        QString _errorMessage;
    };
}

#endif // EVENTS_H
