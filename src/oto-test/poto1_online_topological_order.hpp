// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz
//
// Notes
// -----
// This algorithm corresponds to that used in the following works:
//
// [1] David J. Pearce and Paul H. J. Kelly. A dynamic algorithm for 
//     topologically sorting directed acyclic graphs, In Proceedings 
//     of the Workshop on Efficient and experimental Algorithms (WEA04).
// 
// [2] David J. Pearce and Paul H. J. Kelly. A dynamic topological sort
//     algorithm for directed acyclic graphs, submitted to the journal
//     of Experimental Algorithmics (JEA), 2005.
//
// [3] David J. Pearce. Some directed graph algorithms and their 
//     application to pointer analysis. Ph.D. Thesis, Imperial College 
///    of Science, Technology and Medicine, University of London, 
//     February 2005.

#ifndef POTO1_ONLINE_TOPOLOGICAL_ORDER_HPP
#define POTO1_ONLINE_TOPOLOGICAL_ORDER_HPP

#include <stdexcept>
#include <boost/property_map.hpp>
#include <boost/graph/topological_sort.hpp>
#include "oto_tags.hpp"

// these globals are not strictly needed
// they are used to generate the metrics
#ifdef POTO1_GENERATE_STATS
extern unsigned int poto1_dxy;
extern unsigned int poto1_ddxy;
extern unsigned int poto1_ninvalid;
#endif

template<class T, class N2I>
class poto1_online_topological_order;

template<class N2I>
class poto1_oto_comp {
private:
  N2I _n2i;
public:
  poto1_oto_comp(N2I p)
    : _n2i(p) {
  }

  bool operator()(unsigned int x, unsigned int y) const {
    return _n2i[x] < _n2i[y];
  }
};

template<class T, class N2I> 
bool poto1_oto_fwd_dfs(typename T::vertex_descriptor n, 
		       typename T::vertex_descriptor ub, 
		       std::vector<unsigned int> &reachable,
		       typename boost::property_map<T, N2I>::type n2i,		 
		       poto1_online_topological_order<T,N2I> &g) {
  typedef typename T::out_edge_iterator out_iterator;

  reachable.push_back(n);
  g._visited[n]=true;
#ifdef POTO1_GENERATE_STATS
  ++poto1_ddxy;
#endif
  out_iterator i,iend;
  for(tie(i,iend) = out_edges(n,g); i!=iend;++i) {
    unsigned int w(target(*i,g));
    unsigned int wn2i(n2i[w]);
    if(wn2i == ub) {
      // this is the special case
      // where a cycle has been detected
      return true;
    } else if(wn2i < ub && !g._visited[w]) {
      poto1_oto_fwd_dfs(w,ub,reachable,n2i,g);
    }
#ifdef POTO1_GENERATE_STATS
    ++poto1_ddxy;
#endif
  }
  return false;
}

template<class T, class N2I> 
void poto1_oto_back_dfs(typename T::vertex_descriptor n, 
			typename T::vertex_descriptor lb, 
			std::vector<unsigned int> &reaching,
			typename boost::property_map<T, N2I>::type n2i,		 
			poto1_online_topological_order<T,N2I> &g) {

  typedef typename T::in_edge_iterator in_iterator;

  reaching.push_back(n);
  g._visited[n]=true;
#ifdef POTO1_GENERATE_STATS
  ++poto1_ddxy;
#endif
  in_iterator i,iend;
  for(tie(i,iend) = in_edges(n,g); i!=iend;++i) {
    unsigned int w(source(*i,g));
    unsigned int wn2i(n2i[w]);
    if(wn2i > lb && !g._visited[w]) {
      poto1_oto_back_dfs(w,lb,reaching,n2i,g);
    }
#ifdef POTO1_GENERATE_STATS
    ++poto1_ddxy;
#endif
  }
}

template<class T, class N2I>
void poto1_oto_reorder(std::vector<unsigned int> &reachable,
		   std::vector<unsigned int> &reaching,
		   typename boost::property_map<T, N2I>::type n2i, 
		   poto1_online_topological_order<T,N2I> &g) {    
  std::vector<typename T::vertex_descriptor> tmp;
  tmp.reserve(reaching.size() + reachable.size());
  std::vector<unsigned int>::iterator iend(reaching.end());
  for(std::vector<unsigned int>::iterator i(reaching.begin());
      i!=iend;++i) {
    tmp.push_back(*i);
    g._visited[*i]=false;
    *i = n2i[*i]; // dirty trick
  }
  iend=reachable.end();
  for(std::vector<unsigned int>::iterator i(reachable.begin());
      i!=iend;++i) {   
    tmp.push_back(*i);
    g._visited[*i]=false;
    *i = n2i[*i]; // dirty trick
  }
  std::vector<unsigned int>::iterator i(reachable.begin());
  std::vector<unsigned int>::iterator j(reaching.begin());
  std::vector<unsigned int>::iterator jend(reaching.end());
  unsigned int index(0);
  while(i != iend || j != jend) {
    unsigned int w;
    if(j == jend || (i != iend && *i < *j)) {
      w=*i;++i;
    } else {
      w=*j;++j;
    }
    typename T::vertex_descriptor n(tmp[index++]);
    // allocate n at w
    n2i[n]=w;
  }  
}

template<class T, class N2I>
std::pair<typename T::edge_descriptor, bool> 
add_edge(typename poto1_online_topological_order<T,N2I>::vertex_descriptor t, 
	 typename poto1_online_topological_order<T,N2I>::vertex_descriptor h, 
	 poto1_online_topological_order<T,N2I> &g) {
  std::pair<typename T::edge_descriptor, bool> r;
  r = add_edge(static_cast<typename T::vertex_descriptor>(t),
	       static_cast<typename T::vertex_descriptor>(h),
	       static_cast<T&>(g));

  typedef typename boost::property_map<T, N2I>::type N2iMap;
  N2iMap n2i = get(N2I(),g);

  unsigned int hn2i(n2i[h]);
  unsigned int tn2i(n2i[t]);

  if(r.second && hn2i < tn2i) {
    // need to reorder
    std::vector<unsigned int> reaching, reachable;
    if(poto1_oto_fwd_dfs<T,N2I>(h,tn2i,reachable,n2i,g)) {
      throw std::runtime_error("loop detected");
    } else {
      poto1_oto_back_dfs<T,N2I>(t,hn2i,reaching,n2i,g);
      poto1_oto_comp<N2iMap> pc(n2i);
      sort(reaching.begin(),reaching.end(),pc);
      sort(reachable.begin(),reachable.end(),pc);
      poto1_oto_reorder(reachable,reaching,n2i,g);
#ifdef POTO1_GENERATE_STATS
      ++poto1_ninvalid;
      poto1_dxy += reaching.size() + reachable.size();
#endif
    }
  }
}

template<class InputIter, class T, class N2I>
void add_edges(InputIter b, InputIter e, 
	       poto1_online_topological_order<T,N2I> &g) {
  for(;b!=e;++b) {
    add_edge(b->first,b->second,g);
  }
}

template<class T, class N2I = n2i_t>
class poto1_online_topological_order : public T {
public:
  typedef poto1_online_topological_order<T> self;
  friend std::pair<typename T::edge_descriptor, bool> add_edge<T>(typename self::vertex_descriptor, 
								  typename self::vertex_descriptor, 
								  self &);
  friend void poto1_oto_reorder<T,N2I>(std::vector<unsigned int> &,
					  std::vector<unsigned int> &,
					  typename boost::property_map<T, N2I>::type n2i, 
					  poto1_online_topological_order<T,N2I> &g);

  friend bool poto1_oto_fwd_dfs<T,N2I>(typename self::vertex_descriptor n, 
				       typename self::vertex_descriptor ub,
				       std::vector<unsigned int> &reachable,
				       typename boost::property_map<T, N2I>::type n2i,		 
				       self &g);
  
  friend void poto1_oto_back_dfs<T,N2I>(typename self::vertex_descriptor n, 
					typename self::vertex_descriptor lb,
					std::vector<unsigned int> &reaching,
					typename boost::property_map<T, N2I>::type n2i,		 
					self &g);
private:
  std::vector<bool> _visited; // useful and not too expensive.
public:
  poto1_online_topological_order(T const &g, unsigned int acc = 1) 
    : T(g), _visited(num_vertices(g),false) {
    
    // use the topological sort function to
    // order the vertices correctly ...
    std::vector<unsigned int> tmp;
    tmp.reserve(num_vertices(g));
    topological_sort(g,std::back_inserter(tmp));
    // finally, setup the n2i map correctly
    typedef typename boost::property_map<T, N2I>::type N2iMap;
    N2iMap n2i = get(N2I(),*this);
    
    unsigned int index(0);
    for(std::vector<unsigned int>::reverse_iterator i(tmp.rbegin());i!=tmp.rend();
	++i,++index) {
      n2i[*i]=index;
    }    
  }

  poto1_online_topological_order(typename T::vertices_size_type n,
			       unsigned int acc = 1) 
    : T(n), _visited(n, false) {
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







