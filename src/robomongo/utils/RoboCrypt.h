#include "SimpleCrypt.h"

#include <string>
#include <QString>

namespace Robomongo {

    class RoboCrypt {
    public: 
        static SimpleCrypt& simpleCrypter() {
            static SimpleCrypt simpleCrypt(KEY);
            return simpleCrypt;
        }

        static std::string encrypt(std::string passwd) {
            return simpleCrypter().encryptToString(QString::fromStdString(passwd)).toStdString();
        }

        static std::string decrypt(std::string cryptedPasswd) {
            return simpleCrypter().decryptToString(QString::fromStdString(cryptedPasswd)).toStdString();
        }

    private:
        static const int KEY = 89473829;
    };
}