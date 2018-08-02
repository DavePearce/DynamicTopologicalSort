// (C) Copyright David James Pearce 2005. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: d.pearce@mcs.vuw.ac.nz

#ifndef RANGE_HPP
#define RANGE_HPP

#include <iostream>
#include <string>
#include <stdexcept>

template<class T>
class range {
protected:
  T _start;
  T _end;
  T _value;
  T _increment;
public:
  range() { /* default */ }

  range(T s, T e, T i) 
    : _start(s), _end(e), _value(s), _increment(i) {
  }
  
  range(std::string const &s) {
    // get range from string
    double start,end,step;
    char const *ptr=s.c_str();    
    char *eptr;
    
    start=strtod(ptr,&eptr);
    
    if(eptr != ptr && *eptr == '\0') {
      // catch case when range is empty
      _start = (T) start;
      _end = _start;
      _value = _start;
      _increment = 0;
      return;
    }
    
    if(eptr == ptr || *eptr != ':') {
      throw std::runtime_error(std::string("invalid range \"") + s + "\"");
    }  
    
    ptr=eptr+1;
    end=strtod(ptr,&eptr);
    if(eptr == ptr || *eptr != ':') {
      throw std::runtime_error(std::string("invalid range \"") + s + "\"");
    }
    
    ptr=eptr+1;
    step=strtod(ptr,&eptr);
    
    if(*eptr != '\0') {
      throw std::runtime_error(std::string("invalid range \"") + s + "\"");
    }    
   
    _start = (T) start;
    _end = (T) end;
    _increment = (T) step;
    _value = _start;
  }


  template<class S>
  range(range<S> const &s) {
    _start = (T) s.start();
    _end = (T) s.end();
    _value = (T) s.value();
    _increment = (T) s.increment();
  }

  T value(void) const { return _value; }
  T start(void) const { return _start; }
  T end(void) const { return _end; }
  T increment(void) const { return _increment; }

  bool step(void) {    
    assert(_value <= _end);
    _value += _increment;
    if(_value > _end || _increment == 0) {
      _value = _start;
      return true;
    } else {
      return false;
    }
  }
};


#endif
