/* A Bison parser, made by GNU Bison 2.3.  */

/* Stack handling for Bison parsers in C++

   Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef BISON_STACK_HH
# define BISON_STACK_HH

#include <deque>

namespace yy
{
  template <class T, class S = std::deque<T> >
  class stack
  {
  public:

    // Hide our reversed order.
    typedef typename S::reverse_iterator iterator;
    typedef typename S::const_reverse_iterator const_iterator;

    stack () : seq_ ()
    {
    }

    stack (unsigned int n) : seq_ (n)
    {
    }

    inline
    T&
    operator [] (unsigned int i)
    {
      return seq_[i];
    }

    inline
    const T&
    operator [] (unsigned int i) const
    {
      return seq_[i];
    }

    inline
    void
    push (const T& t)
    {
      seq_.push_front (t);
    }

    inline
    void
    pop (unsigned int n = 1)
    {
      for (; n; --n)
	seq_.pop_front ();
    }

    inline
    unsigned int
    height () const
    {
      return seq_.size ();
    }

    inline const_iterator begin () const { return seq_.rbegin (); }
    inline const_iterator end () const { return seq_.rend (); }

  private:

    S seq_;
  };

  /// Present a slice of the top of a stack.
  template <class T, class S = stack<T> >
  class slice
  {
  public:

    slice (const S& stack,
	   unsigned int range) : stack_ (stack),
				 range_ (range)
    {
    }

    inline
    const T&
    operator [] (unsigned int i) const
    {
      return stack_[range_ - i];
    }

  private:

    const S& stack_;
    unsigned int range_;
  };
}

#endif // not BISON_STACK_HH
