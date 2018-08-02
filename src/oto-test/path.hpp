// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz

#ifndef PATH_HPP
#define PATH_HPP

template<class T>
bool path(typename T::vertex_descriptor from, 
	  typename T::vertex_descriptor to,
	  T const &g) {

  // some useful typedefs
  typedef typename T::vertex_descriptor vd_t;
  typedef typename T::out_edge_iterator oiterator;

  // now the actual code.
  // create worklist and guess at
  // likely max size
  std::vector<vd_t> worklist;    
  // NOTE: this will _not_ work if a
  //       graph with non_integer vertex_descriptors
  //       is used.
  std::vector<bool> visited(num_vertices(g));

  // mark node h as visited  
  visited[from]=true;
  worklist.push_back(from);
  
  // now perform the depth-first-search
  while(worklist.size() > 0) { 
    vd_t n(worklist.back());
    if(n == to) {
      return true;
    } else {
      worklist.pop_back();
      
      oiterator i,iend;
      for(tie(i,iend) = out_edges(n,g); i!=iend;++i) {
	vd_t t(target(*i,g));
	if(!visited[t]) {
	  visited[t]=true;
	  worklist.push_back(t);
	}
      }
    }
  }

  return false;
}

#endif
