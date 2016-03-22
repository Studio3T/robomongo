#include "robomongo/core/mongodb/SshTunnelWorker.h"

#include <QThread>
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/events/MongoEvents.h"
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

            // We are going to access raw char* buffers, so prepare copies
            // of strings (and rest of the things for symmetry)
            _sshhost = ssh->host();
            _sshport = ssh->port();
            _remotehost = _settings->serverHost();
            _remoteport = _settings->serverPort();
            _localip = "127.0.0.1";
            _localport = 27040;

            _userName = ssh->userName();
            _userPassword = ssh->userPassword();
            _privateKeyFile = ssh->privateKeyFile();
            _publicKeyFile = ssh->publicKeyFile();
            _passphrase = ssh->passphrase();
            _authMethod = ssh->authMethod(); // "password" or "publickey"
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

    void SshTunnelWorker::handle(EstablishSshConnectionRequest *event) {
        try {
            ssh_tunnel_config config;
            config.sshserverip = const_cast<char*>(_sshhost.c_str());
            config.sshserverport = static_cast<unsigned int>(_sshport);
            config.remotehost = const_cast<char*>(_remotehost.c_str());
            config.remoteport = static_cast<unsigned int>(_remoteport);
            config.localip = const_cast<char*>(_localip.c_str());
            config.localport = static_cast<unsigned int>(_localport);

            config.username = const_cast<char*>(_userName.c_str());
            config.password = const_cast<char*>(_userPassword.c_str());
            config.privatekeyfile = const_cast<char*>(_privateKeyFile.c_str());
            config.publickeyfile = _publicKeyFile.empty() ? NULL : const_cast<char*>(_publicKeyFile.c_str());
            config.passphrase = const_cast<char*>(_passphrase.c_str());
            config.authtype = (_authMethod == "publickey") ? AUTH_PUBLICKEY : AUTH_PASSWORD;

//            ssh_open_tunnel(config);

            reply(event->sender(), new EstablishSshConnectionResponse(
                    this, event->worker, event->settings, event->visible));

        } catch (const std::exception& ex) {
            reply(event->sender(),
                new EstablishSshConnectionResponse(this, EventError("Failed to create SSH tunnel"),
                event->worker, event->settings, event->visible));
        }
    }

    /**
     * @brief Send reply event to object 'receiver'
     */
    void SshTunnelWorker::reply(QObject *receiver, Event *event)
    {
        if (_isQuiting)
            return;

        AppRegistry::instance().bus()->send(receiver, event);
    }
}
