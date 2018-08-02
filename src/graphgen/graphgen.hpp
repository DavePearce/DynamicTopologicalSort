// (C) Copyright David James Pearce 2005. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: d.pearce@mcs.vuw.ac.nz

#ifndef GRAPHGEN_HPP
#define GRAPHGEN_HPP

#include <iostream>
#include <vector>
#include <utility>
#include <stdexcept>

int read_edgelist(std::istream &input, std::vector<std::pair<unsigned int, unsigned int> > &edgelist) {  
  unsigned int V,E;

  input.read((char*) &V,sizeof(unsigned int));
  input.read((char*) &E,sizeof(unsigned int));

  while(E-- > 0) {
    if(input.eof()) {
      throw new std::runtime_error("binary graph file corrupted");
    }
    // just keep on reading!
    unsigned int data;
    input.read((char*) &data,sizeof(unsigned int));
    unsigned int head = data & 0xFFFF;
    unsigned int tail = data >> 16;
    edgelist.push_back(std::make_pair(tail,head));
  }  
  return V;
}

#endif
