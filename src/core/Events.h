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
    class Error
    {

    public:

        /**
         * @brief Creates "null" (or, in other words, empty) error.
         * Subsequent call of isNull() on this object will return true.
         */
        Error() :
            _isNull(true) {}

        /**
         * @brief Creates error object with specified error message.
         * Subsequent call of isNull() on this object will return false.
         */
        Error(const QString &errorMessage) :
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
         * @brief Error message
         */
        QString _errorMessage;

        /**
         * @brief Flag to support "null" semantic for value object.
         */
        bool _isNull;
    };


    /**
     * @brief Abstract base class for all events in Robomongo.
     */
    class Event
    {
    public:

        /**
         * @brief Creates "non-error" event.
         */
        Event(QObject *sender) :
            _sender(sender) {}

        /**
         * @brief Creates "error-event" that highlights that state of this
         * event is invalid because of error.
         */
        Event(QObject *sender, Error error) :
            _sender(sender),
            _error(error) {}

        /**
         * @brief Type of event. We are using QEvent::Type in order to support
         * usage of events as QEvent with the help of EventWrapper.
         */
        virtual const QEvent::Type type() = 0;

        virtual const char *typeString() = 0;

        /**
         * @brief Sender that emits this event.
         */
        QObject *sender() const { return _sender; }

        /**
         * @brief Tests whether this event is "error-event".
         */
        bool isError() const { return !_error.isNull(); }

        /**
         * @brief Returns Error object.
         */
        Error error() const { return _error; }

    private:

        /**
         * @brief Possible error.
         */
        Error _error;

        /**
         * @brief Sender that emits this event.
         */
        QObject *_sender;
    };
}

#endif // EVENTS_H
