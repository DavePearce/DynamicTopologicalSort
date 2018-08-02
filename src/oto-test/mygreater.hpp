// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz

#ifndef MYGREATER_HPP
#define MYGREATER_HPP

#if __GNUC__ >= 3
#include <ext/functional>
using __gnu_cxx::select2nd;
using __gnu_cxx::identity;
#else
using std::select2nd;
using std::identity;
#endif

template<class T, class S = identity<T> >
class my_greater {
private:
  S _s;
public:
  inline bool operator()(T const &a, T const &b) const {
    return _s(b) < _s(a);
  }
};

#endif
