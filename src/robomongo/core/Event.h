#pragma once

#include <QObject>
#include <QString>
#include <QEvent>
#include <QMetaType>

#include "robomongo/core/EventError.h"

namespace Robomongo
{
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
        Event(QObject *sender, const EventError &error) :
            _sender(sender),
            _error(error) { }

        virtual ~Event() { }

        /**
         * @brief Type identifier of event. We are using QEvent::Type in order to
         * support usage of events as QEvent with the help of EventWrapper.
         */
        virtual QEvent::Type type() = 0;

        /**
         * @brief typeString
         * @return
         */
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
        const EventError &error() const { return _error; }

    private:
        /**
         * @brief Sender that emits this event.
         */
        QObject *_sender;

        /**
         * @brief Possible error.
         */
        const EventError _error;
    };
}

/**
 * @brief R_EVENT macro declares several event's members that are used by
 * Robomongo event subsystem. It should be used like Q_OBJECT macro:
 *
 *  class SampleEvent : public Event
 *  {
 *      R_EVENT
 *
 *      SampleEvent(QObject *sender, const QString &text) :
 *          Event(sender),
 *          server(server) { }
 *
 *      QString text;
 *  };
 *
 * Warning: for convinience, this macro makes all subsequent declarations to
 * be public by default.
 */
#define R_EVENT                           \
    public:                               \
        const static QEvent::Type Type;   \
        const static int metatype;        \
        virtual const char *typeString(); \
        virtual QEvent::Type type();


/**
 * @brief R_REGISTER_EVENT macro defines several event's members that are used
 * by Robomongo simple event subsystem. This macro should be placed in *.cpp file.
 * @param EVENT_CLASS: class name of Event.
 *
 * For SampleEvent shown above, registration will look like this:
 * R_REGISTER_EVENT(SampleEvent)
 */
#define R_REGISTER_EVENT(EVENT_CLASS)                                                               \
    const QEvent::Type EVENT_CLASS::Type = static_cast<QEvent::Type>(QEvent::registerEventType());  \
    const char *EVENT_CLASS::typeString() { return #EVENT_CLASS"*"; }                                \
    QEvent::Type EVENT_CLASS::type() { return EVENT_CLASS::Type; }                                   \
    const int EVENT_CLASS::metatype = qRegisterMetaType<EVENT_CLASS*>(#EVENT_CLASS"*");
