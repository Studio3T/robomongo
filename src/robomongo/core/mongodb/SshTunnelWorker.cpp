#include "robomongo/core/mongodb/SshTunnelWorker.h"

#include <QThread>
#include <QElapsedTimer>

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
        _sshSession(NULL),
        _configCreator(settings)
    {
        _thread = new QThread();
        moveToThread(_thread);
        VERIFY(connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater())));
        VERIFY(connect(_thread, SIGNAL(finished()), this, SLOT(deleteLater())));
        _thread->start();
    }

    SshTunnelWorker::~SshTunnelWorker()
    {
        // QThread "_thread" and MongoWorker itself will be deleted later
        // (see MongoWorker() constructor)

        delete _settings;
        printf("SSH tunnel closed.\n");
    }

    void SshTunnelWorker::stopAndDelete() {
        _isQuiting = 1;
        _thread->quit();
    }

    void SshTunnelWorker::handle(EstablishSshConnectionRequest *event) {
        try {
            if (_isQuiting)
                return;

            // Additionally configure "rbm_ssh_tunnel_config"
            _configCreator.config()->logcontext = this;
            _configCreator.config()->logcallback = &SshTunnelWorker::logCallbackHandler;
            _configCreator.config()->loglevel = (rbm_ssh_log_type) _settings->sshSettings()->logLevel(); // RBM_SSH_LOG_TYPE_DEBUG;

            if ((_sshSession = rbm_ssh_session_create(_configCreator.config())) == 0) {
                // Not much we can say about this error
                throw std::runtime_error("Failed to create SSH session");
            }

            if (rbm_ssh_session_setup(_sshSession) == -1) {
                // Prepare copy of error message (if any)
                std::string error(_sshSession->lasterror);

                std::stringstream ss;
                ss << "Failed to create SSH tunnel to "
                    << _settings->sshSettings()->host() << ":"
                    << _settings->sshSettings()->port() << ".\n\nError:\n"
                    << error;

                // Cleanup session
                rbm_ssh_session_close(_sshSession);
                _sshSession = NULL;

                throw std::runtime_error(ss.str());
            }

            reply(event->sender(), new EstablishSshConnectionResponse(
                    this, event->serverHandle, event->worker, event->settings, event->connectionType, _configCreator.config()->localport));

        } catch (const std::exception& ex) {
            reply(event->sender(),
                new EstablishSshConnectionResponse(this, event->serverHandle, EventError(ex.what()),
                event->worker, event->settings, event->connectionType));

            // In case of error in connection, we should cleanup SshTunnelWorker
            stopAndDelete();
        }
    }

    void SshTunnelWorker::handle(ListenSshConnectionRequest *event) {
        try {
            if (_isQuiting)
                return;

            if (_sshSession == NULL)
                return;

            // We are running this timer in order to distinguish between two
            // types of errors:
            // 1) SSH tunnel wasn't successfully created
            // 2) SSH tunnel was disconnected
            // This is used now only for UI error message, that we show to user.
            QElapsedTimer timer;
            timer.start();

            // This function will block until all TCP connections disconnects.
            // Initially, it will wait for at least one such connection.
            if (rbm_ssh_open_tunnel(_sshSession) != 0) {

                qint64 elapsed = timer.elapsed();
                bool wasDisconnected = elapsed > 20000; // More than 20 seconds passed

                // Prepare copy of error message (if any)
                std::string error(_sshSession->lasterror);

                std::stringstream ss;

                if (wasDisconnected) {
                    ss << "You are disconnected from SSH tunnel ("
                        << _settings->sshSettings()->host() << ":"
                        << _settings->sshSettings()->port() << "). "
                        << "Please initiate a new connection and reopen all tabs.\n\nError:\n"
                        << error;
                } else {
                    ss << "Cannot establish SSH tunnel ("
                        << _settings->sshSettings()->host() << ":"
                        << _settings->sshSettings()->port() << "). "
                        << "\n\nError:\n"
                        << error;
                }

                // Cleanup session
                rbm_ssh_session_close(_sshSession);
                _sshSession = NULL;

                throw std::runtime_error(ss.str());
            }

            log("SSH tunnel stopped normally.", false);

        } catch (const std::exception& ex) {
            reply(event->sender(),
                  new ListenSshConnectionResponse(this, EventError(ex.what()), event->serverHandle, _settings, event->connectionType));
        }

        // When we done with SSH (i.e. rbm_ssh_open_tunnel exits) regardless
        // of the error or success state, we should cleanup SshTunnelWorker.
        stopAndDelete();
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

    void SshTunnelWorker::logCallbackHandler(void *context, char *message, int level) {
        static_cast<SshTunnelWorker*>(context)->log(message, level);
    }

    /*
     * SshTunnelConfigCreator
     */
    SshTunnelConfigCreator::SshTunnelConfigCreator(ConnectionSettings *settings) {
        SshSettings *ssh = settings->sshSettings();

        // We are going to access raw char* buffers, so prepare copies
        // of strings (and rest of the things for symmetry)
        _sshhost = ssh->host();
        _sshport = ssh->port();
        _remotehost = settings->serverHost();
        _remoteport = settings->serverPort();
        _localip = "127.0.0.1";
        _localport = 27040;

        _userName = ssh->userName();
        _userPassword = ssh->userPassword();
        _privateKeyFile = ssh->privateKeyFile();
        _publicKeyFile = ssh->publicKeyFile();
        _passphrase = ssh->passphrase();
        _authMethod = ssh->authMethod(); // "password" or "publickey"

        // Use "askedPassword" for both passphrase and password if required
        if (ssh->askPassword()) {
            _passphrase = ssh->askedPassword();
            _userPassword = ssh->askedPassword();
        }

        _sshConfig = new rbm_ssh_tunnel_config;
        _sshConfig->sshserverhost = const_cast<char*>(_sshhost.c_str());
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

        // Default settings
        _sshConfig->logcontext = NULL;
        _sshConfig->logcallback = NULL;
        _sshConfig->loglevel = RBM_SSH_LOG_TYPE_ERROR;

    }

    SshTunnelConfigCreator::~SshTunnelConfigCreator() {
        delete _sshConfig;
    }
}


