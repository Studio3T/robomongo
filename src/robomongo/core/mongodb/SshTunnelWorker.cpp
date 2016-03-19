#include "robomongo/core/mongodb/SshTunnelWorker.h"

#include <QThread>
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/ssh/ssh.h"

namespace Robomongo
{
    SshTunnelWorker::SshTunnelWorker(ConnectionSettings *settings) : QObject(),
        _settings(settings)
    {
        _thread = new QThread();
        moveToThread(_thread);
        VERIFY(connect(_thread, SIGNAL(started()), this, SLOT(init())));
        VERIFY(connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater())));
        VERIFY(connect(_thread, SIGNAL(finished()), this, SLOT(deleteLater())));
        _thread->start();
    }

    void SshTunnelWorker::init()
    {
        try {
            SshSettings *ssh = _settings->sshSettings();

            // We are going to access raw char* buffers,
            // so prepare copies of strings
            std::string host = ssh->host();
            std::string userName = ssh->userName();
            std::string userPassword = ssh->userPassword();
            std::string privateKeyFile = ssh->privateKeyFile();
            std::string publicKeyFile = ssh->publicKeyFile();
            std::string passphrase = ssh->passphrase();
            std::string authMethod = ssh->authMethod(); // "password" or "publickey"
            std::string remotehost = _settings->serverHost();

            ssh_tunnel_config config;
            config.localip = const_cast<char*>("127.0.0.1");
            config.localport = 27040;

            config.username = const_cast<char*>(userName.c_str());
            config.password = const_cast<char*>(userPassword.c_str());
            config.privatekeyfile = const_cast<char*>(privateKeyFile.c_str());
            config.publickeyfile = publicKeyFile.empty() ? NULL : const_cast<char*>(publicKeyFile.c_str());
            config.passphrase = const_cast<char*>(passphrase.c_str());
            config.sshserverip = const_cast<char*>(host.c_str());
            config.sshserverport = static_cast<unsigned int>(ssh->port());
            config.remotehost = const_cast<char*>(remotehost.c_str());
            config.remoteport = _settings->serverPort();
            config.authtype = (authMethod == "publickey") ? AUTH_PUBLICKEY : AUTH_PASSWORD;

            ssh_open_tunnel(config);

        }
        catch (const std::exception &ex) {
            LOG_MSG(ex.what(), mongo::logger::LogSeverity::Error());
        }
    }

    SshTunnelWorker::~SshTunnelWorker()
    {
        // QThread "_thread" and MongoWorker itself will be deleted later
        // (see MongoWorker() constructor)

        delete _settings;
    }

    void SshTunnelWorker::stopAndDelete() {
        _isQuiting = 1;
        _thread->quit();
    }
}
