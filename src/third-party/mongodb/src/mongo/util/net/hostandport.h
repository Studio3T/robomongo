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

namespace Robomongo
{
#ifdef MONGO_SSL
    struct SSLInfo
    {
        SSLInfo():_sslSupport(false),_sslPEMKeyFile(){}
        SSLInfo(bool ssl,const std::string &key):_sslSupport(ssl),_sslPEMKeyFile(key){}
        explicit SSLInfo(const std::string &sslString):_sslSupport(false),_sslPEMKeyFile() //true:Pem
        {
            for (int i=0; i<sslString.length(); ++i)
            {
                char ch = sslString[i];
                if (ch==':')
                {
                    std::string sslSupport = sslString.substr(0,i);
                    _sslSupport = sslSupport == "1";
                    _sslPEMKeyFile = sslString.substr(i+1);
                    break;
                }
            }
        }
        bool isValid() const {return _sslSupport;}
        bool _sslSupport;
        std::string _sslPEMKeyFile;
    };
    inline std::ostream& operator<< (std::ostream& stream, const SSLInfo& info)
    {
        //[true:Pem]
        stream << "[" << info._sslSupport << ":" << info._sslPEMKeyFile << "]";
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
        static const char delemitr = '+';
        PublicKey():_publicKey(),_privateKey(),_passphrase(){}
        PublicKey(const std::string &publicKey, const std::string &privateKey, const std::string &passphrase):_publicKey(publicKey),_privateKey(privateKey),_passphrase(passphrase){}
        explicit PublicKey(const std::string &keysString):_publicKey(),_privateKey()//abc+dsc+passphrase
        {
            size_t pos = keysString.find_first_of(delemitr);
            int len = keysString.length();
            unsigned short isFirstDelemitr = 0;
            unsigned short posPriv = 0;
            for (int i=0; i<len; ++i)
            {
                char ch = keysString[i];
                if(ch==delemitr){                    
                    if(isFirstDelemitr==0){
                        _publicKey = keysString.substr(0,i);
                        posPriv = i;
                    }
                    else if(isFirstDelemitr==1){
                        _privateKey = keysString.substr(posPriv+1,i);
                        _passphrase = keysString.substr(i+1);
                    }
                    isFirstDelemitr++;
                }
            }
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
        stream << key._publicKey << PublicKey::delemitr << key._privateKey << PublicKey::delemitr << key._passphrase; 
        return stream;
    }

    struct SSHInfo
    {
        SSHInfo():_hostName(DEFAULT_SSH_HOST),_userName(),_port(DEFAULT_SSH_PORT),_password(),_publicKey()
        {

        }
        SSHInfo(const std::string &hostName, const std::string &userName, int port, const std::string &password, const PublicKey &publicKey)
            :_hostName(hostName),_userName(userName),_port(port),_password(password),_publicKey(publicKey)
        {

        }    
        explicit SSHInfo(const std::string &connectionString):_hostName(DEFAULT_SSH_HOST),_userName(),_port(DEFAULT_SSH_PORT),_password(),_publicKey() //username(pass|publ)[password]@hostname[:port]
        {
            int len = connectionString.length();
            int firstSu = 0;
            bool isPass = false;
            bool isPubl = false;
            static const int protLang = 4;
            for (int i=0; i<len; ++i)
            {
                char ch = connectionString[i];
                if (!firstSu && ch == '('){
                    firstSu = i;
                }
                else if(firstSu && ch == ')' ){
                    std::string prot = connectionString.substr(firstSu+1,protLang);
                    if (prot == "publ"){
                        isPubl = true;
                    }
                    else if(prot == "pass"){
                        isPass = true;
                    }                    
                    break;
                }
            }
            for (int i=firstSu; i<len && (isPubl || isPass); ++i)
            {
                char ch = connectionString[i];
                if (ch=='@' && firstSu)
                {
                    _userName = connectionString.substr(0,firstSu);
                    std::string passOrKeys = connectionString.substr(firstSu+protLang+3,i-firstSu-protLang-4);
                    if(isPass){
                        _password = passOrKeys;
                    }
                    else if(isPubl){
                        _publicKey = PublicKey(passOrKeys);
                    }
                    std::string after = connectionString.substr(i+1,len-i);
                    size_t pos = after.find_first_of("[:");
                    if(pos!=std::string::npos){
                        _hostName = after.substr(0,pos);
                        _port = atoi(after.substr(pos+2).c_str());
                    }
                    break;
                }
            }
        }
        bool isValid() const {return !_password.empty() || _publicKey.isValid(); }
        bool isPublicKey() const {return _publicKey.isValid(); }

        std::string _hostName;
        std::string _userName;
        int _port;
        std::string _password;
        PublicKey _publicKey;
    };

    inline std::ostream& operator<< (std::ostream& stream, const SSHInfo& info)
    {
        //[username(pass|publ)[password]@hostname[:port]]
        if(info.isPublicKey()){
            stream << "[" << info._userName << "(publ)[" << info._publicKey << "]@" << info._hostName << "[:" << info._port << "]" << "]";
        }
        else{
            stream << "[" << info._userName << "(pass)[" << info._password << "]@" << info._hostName << "[:" << info._port << "]" << "]";
        }        
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
                int j = i+1;
                int port = 0;
                for (; j < len; ++j)
                {
                    if(isdigit(p[j]))
                        port = port *10 +p[j]-'0';
                    else
                        break;
                }
                _port = port;
                options = p+j;
                break;
            }
        }
        if (!options)
            return;

        const char *endSSL = strchr(options, ']');

        if(endSSL&&*endSSL==']'){
            _sslInfo = Robomongo::SSLInfo(string(options+1,endSSL));
            if(endSSL!=p+lastH)
            _sshInfo = Robomongo::SSHInfo(string(endSSL+2,p+lastH-1));
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
