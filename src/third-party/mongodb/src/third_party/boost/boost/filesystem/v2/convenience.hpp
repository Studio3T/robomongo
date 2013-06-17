//  boost/filesystem/convenience.hpp  ----------------------------------------//

//  Copyright Beman Dawes, 2002-2005
//  Copyright Vladimir Prus, 2002
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/filesystem

//----------------------------------------------------------------------------// 

#ifndef BOOST_FILESYSTEM2_CONVENIENCE_HPP
#define BOOST_FILESYSTEM2_CONVENIENCE_HPP

#include <boost/filesystem/v2/config.hpp>
#include <boost/filesystem/v2/operations.hpp>
#include <boost/system/error_code.hpp>
#include <vector>
#include <stack>

#include <boost/config/abi_prefix.hpp> // must be the last #include

# ifndef BOOST_FILESYSTEM2_NARROW_ONLY
#   define BOOST_FS_FUNC(BOOST_FS_TYPE) \
      template<class Path> typename boost::enable_if<is_basic_path<Path>, \
      BOOST_FS_TYPE>::type
#   define BOOST_FS_FUNC_STRING BOOST_FS_FUNC(typename Path::string_type)
#   define BOOST_FS_TYPENAME typename
# else
#   define BOOST_FS_FUNC(BOOST_FS_TYPE) inline BOOST_FS_TYPE 
    typedef boost::filesystem::path Path;
#   define BOOST_FS_FUNC_STRING inline std::string
#   define BOOST_FS_TYPENAME
# endif

namespace boost
{
  namespace filesystem2
  {

    BOOST_FS_FUNC(bool) create_directories(const Path& ph)
    {
         if (ph.empty() || exists(ph))
         {
           if ( !ph.empty() && !is_directory(ph) )
               boost::throw_exception( basic_filesystem_error<Path>(
                 "boost::filesystem::create_directories", ph,
                 make_error_code( boost::system::errc::file_exists ) ) );
           return false;
         }

         // First create branch, by calling ourself recursively
         create_directories(ph.parent_path());
         // Now that parent's path exists, create the directory
         create_directory(ph);
         return true;
     }

# ifndef BOOST_FILESYSTEM_NO_DEPRECATED

    BOOST_FS_FUNC_STRING extension(const Path& ph)
    {
      typedef BOOST_FS_TYPENAME Path::string_type string_type;
      string_type filename = ph.filename();

      BOOST_FS_TYPENAME string_type::size_type n = filename.rfind('.');
      if (n != string_type::npos)
        return filename.substr(n);
      else
        return string_type();
    }

    BOOST_FS_FUNC_STRING basename(const Path& ph)
    {
      typedef BOOST_FS_TYPENAME Path::string_type string_type;
      string_type filename = ph.filename();
      BOOST_FS_TYPENAME string_type::size_type n = filename.rfind('.');
      return filename.substr(0, n);
    }


    BOOST_FS_FUNC(Path) change_extension( const Path & ph,
      const BOOST_FS_TYPENAME Path::string_type & new_extension )
    {
#   if !defined(_STLPORT_VERSION)
      return ph.parent_path() / (basename(ph) + new_extension); 
#   else
      typedef BOOST_FS_TYPENAME Path::string_type string_type; 
      string_type filename = basename(ph) + new_extension; 
      return ph.parent_path() / filename;
#   endif
    } 
# endif

# ifndef BOOST_FILESYSTEM2_NARROW_ONLY

    // "do-the-right-thing" overloads  ---------------------------------------//

    inline bool create_directories(const path& ph)
      { return create_directories<path>(ph); }
    inline bool create_directories(const wpath& ph)
      { return create_directories<wpath>(ph); }

# ifndef BOOST_FILESYSTEM_NO_DEPRECATED
    inline std::string extension(const path& ph)
      { return extension<path>(ph); }
    inline std::wstring extension(const wpath& ph)
      { return extension<wpath>(ph); }

    inline std::string basename(const path& ph)
      { return basename<path>( ph ); }
    inline std::wstring basename(const wpath& ph)
      { return basename<wpath>( ph ); }

    inline path change_extension( const path & ph, const std::string& new_ex )
      { return change_extension<path>( ph, new_ex ); }
    inline wpath change_extension( const wpath & ph, const std::wstring& new_ex )
      { return change_extension<wpath>( ph, new_ex ); }
# endif

# endif


    //  basic_recursive_directory_iterator helpers  --------------------------//

    namespace detail
    {
      template< class Path >
      struct recur_dir_itr_imp
      {
        typedef basic_directory_iterator< Path > element_type;
        std::stack< element_type, std::vector< element_type > > m_stack;
        int  m_level;
        bool m_no_push;
        bool m_no_throw;

        recur_dir_itr_imp() : m_level(0), m_no_push(false), m_no_throw(false) {}
      };

    } // namespace detail

    //  basic_recursive_directory_iterator  ----------------------------------//

    template< class Path >
    class basic_recursive_directory_iterator
      : public boost::iterator_facade<
          basic_recursive_directory_iterator<Path>,
          basic_directory_entry<Path>,
          boost::single_pass_traversal_tag >
    {
    public:
      typedef Path path_type;

      basic_recursive_directory_iterator(){}  // creates the "end" iterator

      explicit basic_recursive_directory_iterator( const Path & dir_path );
      basic_recursive_directory_iterator( const Path & dir_path,
        system::error_code & ec );

      int level() const { return m_imp->m_level; }

      void pop();
      void no_push()
      {
        BOOST_ASSERT( m_imp.get() && "attempt to no_push() on end iterator" );
        m_imp->m_no_push = true;
      }

      file_status status() const
      {
        BOOST_ASSERT( m_imp.get()
          && "attempt to call status() on end recursive_iterator" );
        return m_imp->m_stack.top()->status();
      }

      file_status symlink_status() const
      {
        BOOST_ASSERT( m_imp.get()
          && "attempt to call symlink_status() on end recursive_iterator" );
        return m_imp->m_stack.top()->symlink_status();
      }

    private:

      // shared_ptr provides shallow-copy semantics required for InputIterators.
      // m_imp.get()==0 indicates the end iterator.
      boost::shared_ptr< detail::recur_dir_itr_imp< Path > >  m_imp;

      friend class boost::iterator_core_access;

      typename boost::iterator_facade< 
        basic_recursive_directory_iterator<Path>,
        basic_directory_entry<Path>,
        boost::single_pass_traversal_tag >::reference
      dereference() const 
      {
        BOOST_ASSERT( m_imp.get() && "attempt to dereference end iterator" );
        return *m_imp->m_stack.top();
      }

      void increment();

      bool equal( const basic_recursive_directory_iterator & rhs ) const
        { return m_imp == rhs.m_imp; }

    };

    typedef basic_recursive_directory_iterator<path> recursive_directory_iterator;
# ifndef BOOST_FILESYSTEM2_NARROW_ONLY
    typedef basic_recursive_directory_iterator<wpath> wrecursive_directory_iterator;
# endif

    //  basic_recursive_directory_iterator implementation  -------------------//

    //  constructors
    template<class Path>
    basic_recursive_directory_iterator<Path>::
      basic_recursive_directory_iterator( const Path & dir_path )
      : m_imp( new detail::recur_dir_itr_imp<Path> )
    {
      m_imp->m_stack.push( basic_directory_iterator<Path>( dir_path ) );
      if ( m_imp->m_stack.top () == basic_directory_iterator<Path>() )
        { m_imp.reset (); }
    }

    template<class Path>
    basic_recursive_directory_iterator<Path>::
      basic_recursive_directory_iterator( const Path & dir_path,
        system::error_code & ec )
      : m_imp( new detail::recur_dir_itr_imp<Path> )
    {
      m_imp->m_no_throw = true;
      m_imp->m_stack.push( basic_directory_iterator<Path>( dir_path, ec ) );
      if ( m_imp->m_stack.top () == basic_directory_iterator<Path>() )
        { m_imp.reset (); }
    }

    //  increment
    template<class Path>
    void basic_recursive_directory_iterator<Path>::increment()
    {
      BOOST_ASSERT( m_imp.get() && "increment on end iterator" );
      
      static const basic_directory_iterator<Path> end_itr;

      if ( m_imp->m_no_push )
        { m_imp->m_no_push = false; }
      else if ( is_directory( m_imp->m_stack.top()->status() ) )
      {
        system::error_code ec;
#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x610))
        if( m_imp->m_no_throw ) {
            m_imp->m_stack.push(
                basic_directory_iterator<Path>( *m_imp->m_stack.top(), ec )
            );
        }
        else {
            m_imp->m_stack.push(
                basic_directory_iterator<Path>( *m_imp->m_stack.top() )
            );
        }
#else
        m_imp->m_stack.push(
          m_imp->m_no_throw
            ? basic_directory_iterator<Path>( *m_imp->m_stack.top(), ec )
            : basic_directory_iterator<Path>( *m_imp->m_stack.top() ) );
#endif
        if ( m_imp->m_stack.top() != end_itr )
        {
          ++m_imp->m_level;
          return;
        }
        m_imp->m_stack.pop();
      }

      while ( !m_imp->m_stack.empty()
        && ++m_imp->m_stack.top() == end_itr )
      {
        m_imp->m_stack.pop();
        --m_imp->m_level;
      }

      if ( m_imp->m_stack.empty() ) m_imp.reset(); // done, so make end iterator
    }

    //  pop
    template<class Path>
    void basic_recursive_directory_iterator<Path>::pop()
    {
      BOOST_ASSERT( m_imp.get() && "pop on end iterator" );
      BOOST_ASSERT( m_imp->m_level > 0 && "pop with level < 1" );

      static const basic_directory_iterator<Path> end_itr;

      do
      {
        m_imp->m_stack.pop();
        --m_imp->m_level;
      }
      while ( !m_imp->m_stack.empty()
        && ++m_imp->m_stack.top() == end_itr );

      if ( m_imp->m_stack.empty() ) m_imp.reset(); // done, so make end iterator
    }

  } // namespace filesystem2
} // namespace boost

#undef BOOST_FS_FUNC_STRING
#undef BOOST_FS_FUNC

//----------------------------------------------------------------------------//

namespace boost
{
  namespace filesystem
  {
    using filesystem2::create_directories;
    using filesystem2::basic_recursive_directory_iterator;
    using filesystem2::recursive_directory_iterator;

# ifndef BOOST_FILESYSTEM_NO_DEPRECATED
    using filesystem2::extension;
    using filesystem2::basename;
    using filesystem2::change_extension;
# endif

# ifndef BOOST_FILESYSTEM2_NARROW_ONLY
    using filesystem2::wrecursive_directory_iterator;
# endif

  }
}

//----------------------------------------------------------------------------//

#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#endif // BOOST_FILESYSTEM2_CONVENIENCE_HPP
