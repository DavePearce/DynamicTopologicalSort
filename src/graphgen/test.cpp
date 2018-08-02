// (C) Copyright David James Pearce 2005. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: d.pearce@mcs.vuw.ac.nz

#include "graphgen.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  int V;
  std::vector<pair<unsigned int, unsigned int> > edgelist;
  V = read_edgelist(cin,edgelist);
  cout << "V = " << V <<  endl;
  cout << "E = { ";
  for(int i=0;i!=edgelist.size();i++) {
    cout << edgelist[i].first << "->" << edgelist[i].second << ", ";
  }
  cout << "}" << endl;
  exit(0);
}
