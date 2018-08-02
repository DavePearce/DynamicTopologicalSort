// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz
//
// Notes
// =====
// This is basically a very simplistic approach
// to topological ordering the graph. It uses
// the standard (exhaustive) topological sort
// function.

#ifndef SIMPLE_TOPOLOGICAL_ORDER_HPP
#define SIMPLE_TOPOLOGICAL_ORDER_HPP

#include <stdexcept>
#include <boost/property_map.hpp>
#include <boost/graph/topological_sort.hpp>
#include "oto_tags.hpp"

#ifdef SOTO_GENERATE_STATS
extern unsigned int algo_count;
#endif

template<class T, class N2I>
class simple_topological_order;

template<class T, class N2I>
void sto_dfs_visit(unsigned int n, unsigned int &count,
		   typename boost::property_map<T, N2I>::type &n2i,
		   simple_topological_order<T,N2I> &g) {
  // some useful typedefs

#ifdef SOTO_GENERATE_STATS
  ++algo_count;
#endif
  
  typedef typename T::out_edge_iterator out_iterator;

  // could undoubtedly optimise this get away. It
  // might be expensive for all I know ...
  
  g._visited[n]=true;
  
  out_iterator i,iend;
  for(tie(i,iend) = out_edges(n,g); i!=iend;++i) {

#ifdef SOTO_GENERATE_STATS
    ++algo_count;
#endif

    unsigned int w(target(*i,g));
    if(!g._visited[w]) {
      sto_dfs_visit(w,count,n2i,g);
    }
  }
  
  n2i[n] = --count;
}

template<class InputIter, class T, class N2I>
void add_edges(InputIter b, InputIter e, 
	       simple_topological_order<T,N2I> &g) {
  bool flag(false);

  typedef typename boost::property_map<T, N2I>::type N2iMap;
  N2iMap n2i = get(N2I(),g);
  
  for(;b!=e;++b) {
    if(add_edge(static_cast<typename T::vertex_descriptor>(b->first),
		static_cast<typename T::vertex_descriptor>(b->second),
		static_cast<T&>(g)).second) {        
      unsigned int hn2i(n2i[b->second]);
      unsigned int tn2i(n2i[b->first]);
      
      if(hn2i < tn2i) { flag = true; }
    }
  }

  if(flag) {
    unsigned int c(num_vertices(g));
    unsigned int count(num_vertices(g));
    for(unsigned int i=0;i!=c;++i) {
      if(!g._visited[i]) {
	sto_dfs_visit(i,count,n2i,g);
      }
    }
    fill(g._visited.begin(),g._visited.end(),false);
  }
}

template<class T, class N2I = n2i_t>
class simple_topological_order : public T {
public:
  typedef simple_topological_order<T,N2I> self;
  friend void add_edges<>(typename std::vector<std::pair<unsigned int, unsigned int> >::iterator, 
			  typename std::vector<std::pair<unsigned int, unsigned int> >::iterator, 
			  self &);
  friend void sto_dfs_visit<>(unsigned int, unsigned int &, 
			      typename boost::property_map<T, N2I>::type &,
			      self &);
private:  
  std::vector<bool> _visited; // useful and not too expensive.
public:
  simple_topological_order(T const &g, unsigned int acc = 1) 
    : T(g), _visited(num_vertices(g),false) {
    
    // use the topological sort function to
    // order the vertices correctly ...

    typedef typename boost::property_map<T, N2I>::type N2iMap;
    N2iMap n2i = get(N2I(),*this);
    
    unsigned int count(num_vertices(g));
    for(unsigned int i=0;i!=num_vertices(g);++i) {
      if(!_visited[i]) {
	sto_dfs_visit(i,count,n2i,*this);
      }
    }
    assert(count == 0);
    fill(_visited.begin(),_visited.end(),false);
  }

  simple_topological_order(typename T::vertices_size_type n,
			       unsigned int acc = 1) 
    : T(n), _visited(n,false) {
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



