#include "SimpleCrypt.h"

#include <string>

namespace Robomongo {

    class RoboCrypt {
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

    private:
        static long long _KEY;
    };

}