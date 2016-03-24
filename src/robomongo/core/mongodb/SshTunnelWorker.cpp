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
        _settings(settings),
        _sshConfig(NULL),
        _sshConnection(NULL)
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

        // We need to delete _sshConfig and _sshConnection somewhere
        // But it is still possible that thread is using them... TODO!
    }

    void SshTunnelWorker::stopAndDelete() {
        _isQuiting = 1;
        _thread->quit();
    }

    void SshTunnelWorker::handle(EstablishSshConnectionRequest *event) {
        try {
            _sshConfig = new ssh_tunnel_config;
            _sshConfig->sshserverip = const_cast<char*>(_sshhost.c_str());
            _sshConfig->sshserverport = static_cast<unsigned int>(_sshport);
            _sshConfig->remotehost = const_cast<char*>(_remotehost.c_str());
            _sshConfig->remoteport = static_cast<unsigned int>(_remoteport);
            _sshConfig->localip = const_cast<char*>(_localip.c_str());
            _sshConfig->localport = static_cast<unsigned int>(_localport);

            _sshConfig->username = const_cast<char*>(_userName.c_str());
            _sshConfig->password = const_cast<char*>(_userPassword.c_str());
            _sshConfig->privatekeyfile = const_cast<char*>(_privateKeyFile.c_str());
            _sshConfig->publickeyfile = _publicKeyFile.empty() ? NULL : const_cast<char*>(_publicKeyFile.c_str());
            _sshConfig->passphrase = const_cast<char*>(_passphrase.c_str());
            _sshConfig->authtype = (_authMethod == "publickey") ? AUTH_PUBLICKEY : AUTH_PASSWORD;

            _sshConnection = new ssh_connection;
            int rc = ssh_esablish_connection(_sshConfig, _sshConnection);
            if (rc != 0) {
                throw std::runtime_error("Failed to establish SSH connection");
            }

            reply(event->sender(), new EstablishSshConnectionResponse(
                    this, event->worker, event->settings, event->visible, _sshConfig->localport));

        } catch (const std::exception& ex) {
            reply(event->sender(),
                new EstablishSshConnectionResponse(this, EventError(ex.what()),
                event->worker, event->settings, event->visible));
        }
    }

    void SshTunnelWorker::handle(ListenSshConnectionRequest *event) {
        try {
            int rc = ssh_open_tunnel(_sshConnection);
            if (rc != 0) {
                throw std::runtime_error("Failed to open SSH tunnel");
            }

        } catch (const std::exception& ex) {
            reply(event->sender(),
                  new ListenSshConnectionResponse(this, EventError("Failed to listen to SSH tunnel")));
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
