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

        rbm_ssh_session_close(_sshSession);

        delete _settings;
        delete _sshConfig;
    }

    void SshTunnelWorker::stopAndDelete() {
        _isQuiting = 1;
        _thread->quit();
    }

    void SshTunnelWorker::handle(EstablishSshConnectionRequest *event) {
        try {
            _sshConfig = new rbm_ssh_tunnel_config;
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
            _sshConfig->authtype = (_authMethod == "publickey") ? RBM_SSH_AUTH_TYPE_PUBLICKEY : RBM_SSH_AUTH_TYPE_PASSWORD;

            _sshConfig->context = this;
            _sshConfig->logcallback = &SshTunnelWorker::logCallbackHandler;
            _sshConfig->loglevel = (rbm_ssh_log_type) _settings->sshSettings()->logLevel(); // RBM_SSH_LOG_TYPE_DEBUG;

            if ((_sshSession = rbm_ssh_session_create(_sshConfig)) == 0) {
                // Cleanup config structure
                delete _sshConfig;
                _sshConfig = NULL;

                // Not much we can say about this error
                throw std::runtime_error("Failed to create SSH session");
            }

            if (rbm_ssh_session_setup(_sshSession) == -1) {
                // Prepare copy of error message (if any)
                std::string error(_sshSession->lasterror);

                // Cleanup session
                rbm_ssh_session_close(_sshSession);
                _sshSession = NULL;

                // Cleanup config
                delete _sshConfig;
                _sshConfig = NULL;

                throw std::runtime_error(error);
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
            if (_sshSession == NULL || _sshConfig == NULL)
                return;

            if (rbm_ssh_open_tunnel(_sshSession) != 0) {
                throw std::runtime_error(_sshSession->lasterror);
            }

            log("SSH tunnel stopped.", false);

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

    void SshTunnelWorker::log(const std::string &message, int level) {
        if (_isQuiting)
            return;

        AppRegistry::instance().bus()->send(
            AppRegistry::instance().app(),
            new LogEvent(this, message, (LogEvent::LogLevel)level));
    }

    void SshTunnelWorker::logCallbackHandler(rbm_ssh_session *session, char *message, int level) {
        static_cast<SshTunnelWorker*>(session->config->context)->log(message, level);
    }
}
