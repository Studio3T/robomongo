#pragma once
namespace Patterns
{
    template <class T>
    class Singleton
    {
      static T* _self;
    public:
      void freeInstance();
      static T* instance();
    protected:
      virtual ~Singleton()
      {
          _self=NULL;
      }
      Singleton()
      {
      }
    };
    template <class T>
    T*  Singleton<T>::_self = NULL;
    template <class T>
    T*  Singleton<T>::instance()
    {
      if(!_self)
        _self=new T;
      return _self;
    }
    template <class T>
    void  Singleton<T>::freeInstance()
    {
        delete this;
    }

    template <class T>
    class LazySingleton
    {
    public:
      typedef LazySingleton<T> class_type;
      static T &instance();
    protected:
      LazySingleton()
      {
      }
      ~LazySingleton(){}
    private:
      LazySingleton(class_type const&);
      LazySingleton& operator=(class_type const& rhs);
    };
    template <class T>
    T &LazySingleton<T>::instance()
    {
      static T _self;
      return _self;
    }
}
