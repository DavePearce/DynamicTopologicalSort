// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz

#ifndef STATS_HPP
#define STATS_HPP

class average {
private:
  double val;
  int count;
public:
  average() : val(0), count(0) {
  }
  
  void operator+=(double v) {
    val = (val*count) / (count+1); 
    val += v / (count+1);
    ++count;
  }

  double value() const { return val; }
};

#endif
