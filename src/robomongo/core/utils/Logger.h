#pragma once
#include <QObject>
#include <QString>
#include <string>
#include "robomongo/core/utils/SingletonPattern.hpp"

namespace Robomongo
{  
    class Logger
        : public QObject, public Patterns::LazySingleton<Logger>
    {
        friend class Patterns::LazySingleton<Logger>;
        Q_OBJECT
    public:
        void print(const char *mess);
        void print(const std::string &mess);
        void print(const QString &mess);        
    Q_SIGNALS:
        void printed(const QString &mess);
    private:
        Logger();
        ~Logger();
    };

    template<typename T>
    inline void LOG_MSG(const T &mess)
    {
        return Logger::instance().print(mess);
    }
}