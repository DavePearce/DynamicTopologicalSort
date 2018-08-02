// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz

#include <iostream>
#include "ordered_slist.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  ordered_slist<void> l2(2);
  ordered_slist<double> l(2);
  ordered_slist<double>::iterator i;

  l.push_front(2.2);
  i=l.insert_after(l.begin(),4.4);
  i=l.insert_after(i,8.8);
  l.push_front(7.2);
  l.push_front(2342);

  cout << "list contains: ";
  for(ordered_slist<double>::const_iterator i(l.begin());
      i!=l.end();++i) {
    cout << *i << " ";
  }
  cout << endl;  
}
