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
            ssh_tunnel_config config;
            config.localip = const_cast<char*>("127.0.0.1");
            config.localport = 27040;
            config.username = const_cast<char*>(ssh->userName().c_str());
            config.password = const_cast<char*>(ssh->userPassword().c_str());
            config.privatekeyfile = const_cast<char*>(ssh->privateKeyFile().c_str());
            config.publickeyfile = const_cast<char*>(ssh->publicKeyFile().c_str());
            config.passphrase = const_cast<char*>(ssh->passphrase().c_str());
            config.sshserverip = const_cast<char*>(ssh->host().c_str());
            config.sshserverport = static_cast<unsigned int>(ssh->port());
            config.remotehost = const_cast<char*>(_settings->serverHost().c_str());
            config.remoteport = _settings->serverPort();

            printf("* Openning tunnel, yah! \n");
            printf("SshTunnelWorker: username: %s \n", ssh->userName().c_str());
            printf("SshTunnelWorker: username: %s \n", config.username);
            printf("SshTunnelWorker: privatekeyfile: %s \n", const_cast<char*>(ssh->privateKeyFile().c_str()));
            printf("SshTunnelWorker: privatekeyfile: %s \n", config.privatekeyfile);
            ssh_open_tunnel(config);
            printf("* Done with tunnel!\n");

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
        printf("* SSH Tunnel shutdowned\n");
    }

    void SshTunnelWorker::stopAndDelete() {
        _isQuiting = 1;
        _thread->quit();
    }
}
