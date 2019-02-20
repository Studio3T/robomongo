#include "SimpleCrypt.h"

#include <string>
#include <QString>

namespace Robomongo {

    class RoboCrypt {
    public: 
        static SimpleCrypt& simpleCrypter() {
            static SimpleCrypt simpleCrypt(quint64(0x449d327fe8693558));
            return simpleCrypt;
        }

        static std::string encrypt(std::string passwd) {
            return simpleCrypter().encryptToString(QString::fromStdString(passwd)).toStdString();
        }

        static std::string decrypt(std::string cryptedPasswd) {
            return simpleCrypter().decryptToString(QString::fromStdString(cryptedPasswd)).toStdString();
        }
    };
}