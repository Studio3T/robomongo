#include "SimpleCrypt.h"

#include <string>

#include <mongo/logger/log_severity.h>

namespace Robomongo {
    
    class RoboCrypt {    
        using LogAndSeverity = std::pair<std::string, mongo::logger::LogSeverity>;
    public: 
        static SimpleCrypt& simpleCrypter() {
            static SimpleCrypt simpleCrypt(_KEY);
            return simpleCrypt;
        }

        static std::string encrypt(std::string passwd) {
            return simpleCrypter().encryptToString(QString::fromStdString(passwd)).toStdString();
        }

        static std::string decrypt(std::string cryptedPasswd) {
            return simpleCrypter().decryptToString(QString::fromStdString(cryptedPasswd)).toStdString();
        }

        // Read key from key file otherwise create a new key and save it into file
        static void initKey();

        static std::vector<LogAndSeverity> const& roboCryptLogs() {
            return _roboCryptLogs;
        }

    private:
        static long long _KEY;
        static std::vector<LogAndSeverity> _roboCryptLogs;
    };

}