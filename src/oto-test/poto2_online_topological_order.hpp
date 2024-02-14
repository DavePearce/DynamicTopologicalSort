// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz

#ifndef POTO2_ONLINE_TOPOLOGICAL_ORDER_HPP
#define POTO2_ONLINE_TOPOLOGICAL_ORDER_HPP

#include <boost/graph/topological_sort.hpp>
#include <boost/property_map.hpp>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>
#include <set>
#include <map>

#include "oto_tags.hpp"
#include "myless.hpp"

#if __GNUC__ >= 3
#include <ext/functional>
#define SELECT1ST __gnu_cxx::select1st
#define SELECT2ND __gnu_cxx::select2nd
#define IDENTITY __gnu_cxx::identity
#else 
// everything else for the moment.
#include <functional>
#define SELECT1ST std::select1st
#define SELECT2ND std::select2nd
#define IDENTITY std::identity
#endif

#define EDGE_T std::pair<typename T::vertex_descriptor,typename T::vertex_descriptor>
#define WORKLIST_T std::set<EDGE_T,my_less<EDGE_T, SELECT2ND<EDGE_T> > >

template<class T, class N2I, class I2NMAP>
class poto2_online_topological_order;

template<class T, class N2I, class I2NMAP>
void shift(unsigned int index, std::vector<EDGE_T> &reachables,
	   poto2_online_topological_order<T,N2I,I2NMAP> &g) {
  
  typedef typename T::vertex_descriptor vd_t;
  typedef typename boost::property_map<T, N2I>::type N2iMap;
  N2iMap n2i = get(N2I(),g);

  // now, perform the shift.

  typename std::vector<EDGE_T>::reverse_iterator i(reachables.rbegin());
  unsigned int shift(0);
  while(i != reachables.rend()) {    
    assert(index < g._i2n.size());
    unsigned int w(g._i2n[index]);

    if(g._visited[w]) {
      ++shift;
      // reset the visited relation.
      g._visited[w]=false;
    } else {
      g._i2n[index-shift]=w;
      n2i[w]=index-shift;	
    }
    // check for an end, like.

    while(i != reachables.rend() && i->first == index) {
      // ok, we're at an end point
      // so allocate now.
      --shift;
      g._i2n[index-shift]=i->second;
      n2i[i->second]=index-shift;	      
      ++i;
    }

    ++index;
  }
}

template<class T, class N2I, class I2NMAP>
void find_reachables(unsigned int n, unsigned int ub,
		     std::vector<EDGE_T> &reachables,
		     poto2_online_topological_order<T,N2I,I2NMAP> &g) {
  // some useful typedefs
  
  typedef typename T::out_edge_iterator out_iterator;
  typedef typename boost::property_map<T, N2I>::type N2iMap;

  // could undoubtedly optimise this get away. It
  // might be expensive for all I know ...

  N2iMap n2i = get(N2I(),g);
  
  g._visited[n]=true;
  
  out_iterator i,iend;
  for(tie(i,iend) = out_edges(n,g); i!=iend;++i) {
    unsigned int w(target(*i,g));
    unsigned int wn2i(n2i[w]);
    if(wn2i == ub) {
      // this is the special case
      // where a cycle has been detected
      throw std::runtime_error("CYCLE DETECTED");
    } else if(wn2i < ub && !g._visited[w]) {
      find_reachables(w,ub,reachables,g);
    }
  }
  
  reachables.push_back(EDGE_T(ub,n));
}

template<class T, class N2I, class I2NMAP>
void add_edges(typename std::vector<EDGE_T>::iterator beg, 
	       typename std::vector<EDGE_T>::iterator end, 
	       poto2_online_topological_order<T,N2I,I2NMAP> &g) {

  std::vector<EDGE_T> backedges;
  for(;beg!=end;++beg) {
    std::pair<typename T::edge_descriptor, bool> r;
    EDGE_T e(*beg);
    r = add_edge(static_cast<typename T::vertex_descriptor>(e.first),
		 static_cast<typename T::vertex_descriptor>(e.second),
		 static_cast<T&>(g));
    
    typedef typename boost::property_map<T, N2I>::type N2iMap;
    N2iMap n2i = get(N2I(),g);
    
    unsigned int hn2i(n2i[e.second]);
    unsigned int tn2i(n2i[e.first]);
    
    if(r.second && hn2i < tn2i) {
      backedges.push_back(EDGE_T(n2i[e.first],e.second));
    }
  }

  if(!backedges.empty()) {
    typedef typename boost::property_map<T, N2I>::type N2iMap;
    N2iMap n2i = get(N2I(),g);
    //    my_less<EDGE_T, SELECT1ST<EDGE_T> > comp;
    // sort back edges into increasing order by their tail
    std::sort(backedges.begin(),backedges.end(),my_less<EDGE_T, SELECT1ST<EDGE_T> >());
    std::vector<EDGE_T> reachables;
    unsigned int lb(g._i2n.size());

    // now, identify reachables
    for(typename std::vector<EDGE_T>::reverse_iterator i(backedges.rbegin());
	i!=backedges.rend();++i) {
      if(i->first < lb && !reachables.empty()) { 
	
	shift(lb,reachables,g);
	reachables.clear();
      }
      if(!g._visited[i->second]) {
	find_reachables<T,N2I,I2NMAP>(i->second,i->first,reachables,g);
      }
      lb = std::min(lb,n2i[i->second]);
    }

    if(!reachables.empty()) { 	
      shift(lb,reachables,g);
    }
  }  
}

template<class T, class N2I = n2i_t, class I2NMAP = std::vector<typename T::vertex_descriptor> >
class poto2_online_topological_order : public T {
public:
  typedef poto2_online_topological_order<T,N2I,I2NMAP> self;
  friend void add_edges<>(typename std::vector<EDGE_T>::iterator, typename std::vector<EDGE_T>::iterator, 
			self &);
  friend void find_reachables<>(unsigned int, unsigned int, std::vector<EDGE_T> &,
				self &);
  friend void shift<>(unsigned int, std::vector<EDGE_T> &, self &);
private:
  I2NMAP _i2n;
  std::vector<bool> _visited; // useful and not too expensive.
public:
  poto2_online_topological_order(T const &g, unsigned int acc = 1) 
    : T(g), _visited(num_vertices(g),false)  {
    
    // use the topological sort function to
    // order the vertices correctly ...
    _i2n.reserve(num_vertices(g));
    topological_sort(g,std::back_inserter(_i2n));
    // now reverse them
    reverse(_i2n.begin(),_i2n.end());
    // finally, setup the n2i map correctly
    typedef typename boost::property_map<T, N2I>::type N2iMap;
    N2iMap n2i = get(N2I(),*this);
    
    for(unsigned int i=0;i!=_i2n.size();++i) {
      n2i[_i2n[i]]=i;
    }    
  }

  poto2_online_topological_order(typename T::vertices_size_type n,
			       unsigned int acc = 1) 
    : T(n), _i2n(n), _visited(n,false) {
    typedef typename boost::property_map<T, N2I>::type N2iMap;
    N2iMap n2i = get(N2I(),*this);
    
    unsigned int counter(0);
    typename T::vertex_iterator i,iend;
    for(tie(i,iend) = vertices(*this);i!=iend;++i) {
      _i2n[counter]=*i;
      n2i[*i]=counter++;
    }
  }
};

#undef WORKLIST_T
#undef EDGE_T

#endif



