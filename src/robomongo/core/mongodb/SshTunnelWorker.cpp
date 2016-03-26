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
#include "robomongo/core/domain/App.h"

namespace Robomongo
{
    SshTunnelWorker::SshTunnelWorker(ConnectionSettings *settings) : QObject(),
        _settings(settings),
        _sshConfig(NULL),
        _sshSession(NULL)
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

        log("*** Shutdown of SSH worker ***");

        ssh_session_close(_sshSession);

        delete _settings;
        delete _sshConfig;
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

            _sshConfig->context = this;
            _sshConfig->logcallback = &SshTunnelWorker::logCallbackHandler;

            if ((_sshSession = ssh_session_create(_sshConfig)) == 0) {
                throw std::runtime_error(_sshSession->lasterror);
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
            int rc = ssh_open_tunnel(_sshSession);
            if (rc != 0) {
                throw std::runtime_error("Failed to open SSH tunnel");
            }

        } catch (const std::exception& ex) {
            reply(event->sender(),
                  new ListenSshConnectionResponse(this, EventError(ex.what()), _settings));
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

    void SshTunnelWorker::log(const std::string &message, bool error) {
        if (_isQuiting)
            return;

        AppRegistry::instance().bus()->send(
            AppRegistry::instance().app(),
            new LogEvent(this, message, error));
    }

    void SshTunnelWorker::logCallbackHandler(ssh_session *session, char *message, int iserror) {
        static_cast<SshTunnelWorker*>(session->config->context)->log(message, iserror == 1);
    }
}
