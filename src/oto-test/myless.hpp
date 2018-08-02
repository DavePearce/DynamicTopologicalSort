#ifndef MYLESS_HPP
#define MYLESS_HPP

#if __GNUC__ >= 3
#include <ext/functional>
using __gnu_cxx::select2nd;
using __gnu_cxx::identity;
#else
using std::select2nd;
using std::identity;
#endif

template<class T, class S = identity<T> >
class my_less {
private:
  S _s;
public:
  inline bool operator()(T const &a, T const &b) const {
    return _s(a) < _s(b);
  }
};

#endif
