#ifndef MONGOEXCEPTION_H
#define MONGOEXCEPTION_H

#include <iostream>
#include <QString>

namespace Robomongo
{
    class MongoException : public std::exception
    {
    public:
        MongoException(const QString &s) : std::exception(), _msg(s)
        {
            _bytes = _msg.toLatin1();
        }

        ~MongoException() throw() {}
        virtual const char* what() const throw()
        {
            return _bytes.data();
        }

    private:
        QString _msg;
        QByteArray _bytes;
    };
}

#endif // MONGOEXCEPTION_H
