#ifndef APPREGISTRY_H
#define APPREGISTRY_H

namespace Robomongo
{
    class AppRegistry
    {
    public:
        ~AppRegistry(void);

        static AppRegistry & instance()
        {
            static AppRegistry _instance;
            return _instance;
        }

    private:
        AppRegistry();

        /*
        ** Singleton support
        */
        AppRegistry(AppRegistry const &);       // To protect from copies of singleton
        void operator=(AppRegistry const &);	// To protect from copies of singleton
    };
}

#endif // APPREGISTRY_H
