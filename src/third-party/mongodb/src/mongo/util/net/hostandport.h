// hostandport.h

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "mongo/bson/util/builder.h"
#include "mongo/db/cmdline.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/net/sock.h"

#ifdef ROBOMONGO
#define DEFAULT_SSH_PORT 22
#define DEFAULT_SSH_HOST ""
#include "mongo/db/json.h"

namespace Robomongo
{
#ifdef MONGO_SSL
    struct SSLInfo
    {
        SSLInfo():_sslSupport(false),_sslPEMKeyFile(){}
        SSLInfo(bool ssl,const std::string &key):_sslSupport(ssl),_sslPEMKeyFile(key){}
        explicit SSLInfo(const mongo::BSONElement &elem):_sslSupport(false),_sslPEMKeyFile()
        {
            if(!elem.eoo()){
                mongo::BSONObj obj = elem.Obj();
                _sslSupport = obj.getField("sslSupport").Bool();
                _sslPEMKeyFile = obj.getField("sslPEMKeyFile").String();
            }
        }

        mongo::BSONObj toBSONObj() const
        {
            mongo::BSONObjBuilder b;
            b.append("SSL",BSON("sslSupport" << _sslSupport  << "sslPEMKeyFile" << _sslPEMKeyFile));
            return b.obj();
        }
        bool isValid() const {return _sslSupport;}
        bool _sslSupport;
        std::string _sslPEMKeyFile;
    };

    inline std::ostream& operator<< (std::ostream& stream, const SSLInfo& info)
    {
        stream << info.toBSONObj().toString();      
        return stream;
    }

    inline bool operator==(const SSLInfo& r,const SSLInfo& l) 
    { 
        return r._sslSupport == l._sslSupport && r._sslPEMKeyFile == l._sslPEMKeyFile;
    }
#endif
#ifdef SSH_SUPPORT_ENABLED
    struct PublicKey
    {
        PublicKey():_publicKey(),_privateKey(),_passphrase(){}
        PublicKey(const std::string &publicKey, const std::string &privateKey, const std::string &passphrase = ""):_publicKey(publicKey),_privateKey(privateKey),_passphrase(passphrase){}
        explicit PublicKey(const mongo::BSONObj &obj):_publicKey(),_privateKey()//abc+dsc+passphrase
        {
            _publicKey = obj.getField("publicKey").String();
            _privateKey = obj.getField("privateKey").String();
            _passphrase = obj.getField("passphrase").String();
        }

        mongo::BSONObj toBSONObj() const
        {
            return BSON( "publicKey" << _publicKey << "privateKey" << _privateKey << "passphrase" << _passphrase);
        }
        bool isValid() const {return !_privateKey.empty(); }
        std::string _publicKey;
        std::string _privateKey;
        std::string _passphrase;
    };

    inline bool operator==(const PublicKey& r,const PublicKey& l) 
    { 
        return r._publicKey == l._publicKey && r._privateKey == l._privateKey && r._passphrase == l._passphrase;
    }

    inline std::ostream& operator<< (std::ostream& stream, const PublicKey& key)
    {
        stream << key.toBSONObj().toString();   
        return stream;
    }

    struct SSHInfo
    {
        enum SupportedAuthenticationMetods
        {
            UNKNOWN = 0,
            PASSWORD = 1,
            PUBLICKEY = 2
        };

        SSHInfo():_hostName(DEFAULT_SSH_HOST),_userName(),_port(DEFAULT_SSH_PORT),_password(),_publicKey(),_currentMethod(UNKNOWN)
        {

        }

        SSHInfo(const std::string &hostName, int port, const std::string &userName,  const std::string &password, const PublicKey &publicKey, SupportedAuthenticationMetods method)
            :_hostName(hostName),_port(port),_userName(userName),_password(password),_publicKey(publicKey),_currentMethod(method)
        {
           
        }

        explicit SSHInfo(const mongo::BSONElement &elem):_hostName(DEFAULT_SSH_HOST),_userName(),_port(DEFAULT_SSH_PORT),_password(),_publicKey(),_currentMethod(UNKNOWN)
        {
            if(!elem.eoo()){
                mongo::BSONObj obj = elem.Obj();
                _hostName = obj.getField("host").String();            
                _port = obj.getField("port").Int();
                _userName = obj.getField("user").String();
                _password = obj.getField("password").String();
                _publicKey = PublicKey(obj.getField("publicKey").Obj());
                _currentMethod = static_cast<SupportedAuthenticationMetods>(obj.getField("currentMethod").Int());
            }
        }

        mongo::BSONObj toBSONObj() const
        {
            mongo::BSONObjBuilder b;
            b.append("SSH",BSON("host" << _hostName << "port" << _port << "user" << _userName << "password" << _password << "publicKey" << _publicKey.toBSONObj() << "currentMethod" << _currentMethod ));
            return b.obj();
        }
        bool isValid() const { return _currentMethod != UNKNOWN; }
        SupportedAuthenticationMetods authMethod() const { return _currentMethod; }

        std::string _hostName;
        int _port;
        std::string _userName;        
        std::string _password;
        PublicKey _publicKey;
        SupportedAuthenticationMetods _currentMethod;
    };

    inline std::ostream& operator<< (std::ostream& stream, const SSHInfo& info)
    {
        stream << info.toBSONObj().toString();
        return stream;
    }

    inline bool operator==(const SSHInfo& r,const SSHInfo& l) 
    { 
        return r._hostName == l._hostName && r._password == l._password && r._port == l._port && r._publicKey==l._publicKey && r._userName == l._userName && r._publicKey == l._publicKey;
    }
#endif
}
#endif

namespace mongo {

    using namespace mongoutils;

    /** helper for manipulating host:port connection endpoints.
      */
    struct HostAndPort {
        HostAndPort() : _port(-1){ }

        /** From a string hostname[:portnumber]
            Throws user assertion if bad config string or bad port #.
        */
        HostAndPort(const std::string& s);

        /** @param p port number. -1 is ok to use default. */
        HostAndPort(const std::string& h, int p /*= -1*/) : _host(h), _port(p) { 
            verify( !str::startsWith(h, '#') );
        }
#ifdef ROBOMONGO
        HostAndPort(const std::string &h, int p
#ifdef MONGO_SSL
            , Robomongo::SSLInfo sslInfo
#endif
#ifdef SSH_SUPPORT_ENABLED
            , Robomongo::SSHInfo sshInfo
#endif
            ):
        _host(h), _port(p)
#ifdef MONGO_SSL
        ,_sslInfo(sslInfo)
#endif
#ifdef SSH_SUPPORT_ENABLED
            ,_sshInfo(sshInfo)
#endif
        {

        }
#endif // ROBOMONGO

        HostAndPort(const SockAddr& sock ) : _host( sock.getAddr() ) , _port( sock.getPort() ) { }

        static HostAndPort me();

        bool operator<(const HostAndPort& r) const {
            string h = host();
            string rh = r.host();
            if( h < rh )
                return true;
            if( h == rh )
                return port() < r.port();
            return false;
        }
#ifdef ROBOMONGO
        bool operator==(const HostAndPort& r) const { 
            return host() == r.host() && port() == r.port() && sshInfo() == r.sshInfo() && sslInfo() == r.sslInfo(); 
        }
#else
        bool operator==(const HostAndPort& r) const { 
            return host() == r.host() && port() == r.port(); 
        }
#endif
        bool operator!=(const HostAndPort& r) const { return !(*this == r); }

        /* returns true if the host/port combo identifies this process instance. */
        bool isSelf() const; // defined in isself.cpp

        bool isLocalHost() const;

        /**
         * @param includePort host:port if true, host otherwise
         */
        string toString( bool includePort=true ) const;

        operator string() const { return toString(); }

        void append( StringBuilder& ss ) const;

        bool empty() const {
            return _host.empty() && _port < 0;
        }
        string host() const {
            return _host;
        }
#ifdef ROBOMONGO
        void setHost( const string& host ) {
            _host = host;
        }
#ifdef MONGO_SSL
        Robomongo::SSLInfo sslInfo() const {return _sslInfo;}
        void setSslInfo(const Robomongo::SSLInfo &info) {_sslInfo = info;}
#endif
#ifdef SSH_SUPPORT_ENABLED
        Robomongo::SSHInfo sshInfo() const {return _sshInfo;}
        void setSshInfo(const  Robomongo::SSHInfo &info) {_sshInfo = info;}
#endif
#endif
        int port() const {
            if (hasPort())
                return _port;
            return CmdLine::DefaultDBPort;
        }
        bool hasPort() const {
            return _port >= 0;
        }
        void setPort( int port ) {
            _port = port;
        }

    private:
        void init(const char *);
        string _host;
        int _port; // -1 indicates unspecified
#ifdef ROBOMONGO
#ifdef MONGO_SSL
       Robomongo::SSLInfo _sslInfo; 
#endif // MONGO_SSL
#ifdef SSH_SUPPORT_ENABLED
        Robomongo::SSHInfo _sshInfo;
#endif
#endif
    };

    inline HostAndPort HostAndPort::me() {
        const char* ips = cmdLine.bind_ip.c_str();
        while(*ips) {
            string ip;
            const char * comma = strchr(ips, ',');
            if (comma) {
                ip = string(ips, comma - ips);
                ips = comma + 1;
            }
            else {
                ip = string(ips);
                ips = "";
            }
            HostAndPort h = HostAndPort(ip, cmdLine.port);
            if (!h.isLocalHost()) {
                return h;
            }
        }

        string h = getHostName();
        verify( !h.empty() );
        verify( h != "localhost" );
        return HostAndPort(h, cmdLine.port);
    }

    inline string HostAndPort::toString( bool includePort ) const {
        if ( ! includePort )
            return host();

        StringBuilder ss;
        append( ss );
        return ss.str();
    }

    inline void HostAndPort::append( StringBuilder& ss ) const {
        ss << host();

        int p = port();

        if ( p != -1 ) {
            ss << ':';
#if defined(_DEBUG)
            if( p >= 44000 && p < 44100 ) {
                log() << "warning: special debug port 44xxx used" << endl;
                ss << p+1;
            }
            else
                ss << p;
#else
            ss << p;
#endif
        }
#ifdef ROBOMONGO
        std::stringstream urlStream;
#ifdef MONGO_SSL
        urlStream << _sslInfo;
#endif // MONGO_SSL
#ifdef SSH_SUPPORT_ENABLED
        urlStream << _sshInfo;
#endif
        ss << urlStream.str();
#endif
    }


    inline bool HostAndPort::isLocalHost() const {
        string _host = host();
        return (  _host == "localhost"
               || startsWith(_host.c_str(), "127.")
               || _host == "::1"
               || _host == "anonymous unix socket"
               || _host.c_str()[0] == '/' // unix socket
               );
    }

    inline void HostAndPort::init(const char *p) {
        massert(13110, "HostAndPort: host is empty", *p);
#ifdef ROBOMONGO
        int len = strlen(p);
        int lastH = len-1;
        int member = 0;
        _host = p;
        _port = -1;
        _sslInfo = Robomongo::SSLInfo();
        _sshInfo = Robomongo::SSHInfo();
        const char* options = NULL;
        for (int i=0 ; i<len; ++i)
        {
            char ch = p[i];
            if (ch==':')
            {
                _host = string(p, i);
                _port = atoi(p+i+1);
            }
            if (ch=='{')
            {
                options = p+i;
                break;
            }
        }
        if (!options)
            return;

        int jsonLen = strlen(options);
        int offset = 0;
        while(offset!=jsonLen)
        { 
            mongo::BSONObj main = mongo::fromjson(options+offset,&len);
            mongo::BSONElement ssl = main.getField("SSL");
            mongo::BSONElement ssh = main.getField("SSH");
            if(!ssl.eoo()){
                _sslInfo = Robomongo::SSLInfo(ssl);
            }
            else if(!ssh.eoo()){
                _sshInfo = Robomongo::SSHInfo(ssh);
            }
            offset+=len;
        }
        
#else
        const char *colon = strrchr(p, ':');
        if( colon ) {
            int port = atoi(colon+1);
            uassert(13095, "HostAndPort: bad port #", port > 0);
            _host = string(p,colon-p);
            _port = port;
        }
        else {
            // no port specified.
            _host = p;
            _port = -1;
        }
#endif // ROBOMONGO

    }

    inline HostAndPort::HostAndPort(const std::string& s) {
        init(s.c_str());
    }

}
