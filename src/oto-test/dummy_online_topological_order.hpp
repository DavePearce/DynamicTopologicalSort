// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz

#ifndef DUMMY_ONLINE_TOPOLOGICAL_ORDER_HPP
#define DUMMY_ONLINE_TOPOLOGICAL_ORDER_HPP

#include <stdexcept>

template<class T, class N2I>
class dummy_online_topological_order;

template<class InputIter, class T, class N2I>
void add_edges(InputIter b, InputIter e, 
	       dummy_online_topological_order<T,N2I> &g) {
  for(;b!=e;++b) {
    add_edge(b->first,b->second,g);
  }
}

template<class T, class N2I = n2i_t>
class dummy_online_topological_order : public T {
public:
private:
  std::vector<bool> _visited; // useful and not too expensive.
public:
  dummy_online_topological_order(T const &g) 
    : T(g){
  }

  dummy_online_topological_order(typename T::vertices_size_type n,
				 unsigned int acc = 1) 
    : T(n) {
    // doing this just ensures that --checking
    // produces errors as expected!
    typedef typename boost::property_map<T, N2I>::type N2iMap;
    N2iMap n2i = get(N2I(),*this);
    
    unsigned int counter(0);
    typename T::vertex_iterator i,iend;
    for(tie(i,iend) = vertices(*this);i!=iend;++i) {
      n2i[*i]=counter++;
    }
  }
};

#endif







