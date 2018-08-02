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
// This is an alternative solution to the online topological 
// order problem. Although, I designed this algorithm from 
// scratch myself, it turns out to have already been published:
//
// "On-Line Graph Algorithms for Incremental Compilation", 
// Alberto Marchetti-Spaccamela, Umberto Nanni and Hans 
// Rohnert. Information Processing Letters, 1996.

#ifndef MNR_ONLINE_TOPOLOGICAL_ORDER_HPP
#define MNR_ONLINE_TOPOLOGICAL_ORDER_HPP

#include <stdexcept>
#include <boost/property_map.hpp>
#include <boost/graph/topological_sort.hpp>
#include "oto_tags.hpp"

#ifdef MNR_GENERATE_STATS
extern unsigned int mnr_ARxy;
extern unsigned int mnr_ddfxy;
extern unsigned int mnr_ninvalid;
extern unsigned int algo_count;
#endif

template<class T, class N2I, class I2NMAP>
class mnr_online_topological_order;

template<class T, class N2I, class I2NMAP> 
bool mnr_oto_dfs(typename T::vertex_descriptor h, unsigned int lb,
		 unsigned int ub,
		 typename boost::property_map<T, N2I>::type &n2i, 
		 mnr_online_topological_order<T,N2I,I2NMAP> &g) {

  // some useful typedefs

  typedef typename T::vertex_descriptor vd_t;
  typedef typename T::out_edge_iterator oiterator;

  // now the actual code.
  // create worklist and guess at
  // likely max size

  std::vector<vd_t> worklist;    
  worklist.reserve((ub - lb) + 1);

  // mark node h as visited
  
  g._visited[lb]=true;
  worklist.push_back(h);

  // now perform the depth-first-search

  while(worklist.size() > 0) { 
    vd_t n(worklist.back());
    worklist.pop_back();
#ifdef MNR_GENERATE_STATS
    ++mnr_ddfxy;
    ++algo_count;
#endif

    oiterator i,iend;
    for(tie(i,iend) = out_edges(n,g); i!=iend;++i) {
      unsigned int wn2i(n2i[target(*i,g)]);
      if(wn2i == ub) {
	// this is the special case
	// where a cycle has been detected
	return true;
      } else if(wn2i < ub && !g._visited[wn2i]) {
	g._visited[wn2i]=true;
	worklist.push_back(target(*i,g));
      }
#ifdef MNR_GENERATE_STATS
    ++mnr_ddfxy;
    ++algo_count;
#endif
    }
  }
  return false;
}

template<class T, class N2I, class I2NMAP>
void mnr_oto_shift(unsigned int lb, 
		   unsigned int ub,  
		   typename boost::property_map<T, N2I>::type &n2i, 
		   mnr_online_topological_order<T,N2I,I2NMAP> &g) {    

  // some useful typedefs

  typedef typename T::vertex_descriptor vd_t;

  // the body

  unsigned int shift(0);
  std::vector<vd_t> tmp; // temporary storage 
  unsigned int i;
  for(i=lb;i<=ub;++i) {
#ifdef MNR_GENERATE_STATS
    ++algo_count;
#endif
    vd_t w(g._i2n[i]);
    if(g._visited[i]) {
      tmp.push_back(w);
      ++shift;
      g._visited[i]=false;
    } else {
      g._i2n[i-shift]=w;
      n2i[w]=i-shift;	
    }
  }
  i-=shift;
  for(unsigned int j=0;j<tmp.size();++j) {
#ifdef MNR_GENERATE_STATS
    ++algo_count;
#endif
    unsigned int w(tmp[j]);
    g._i2n[i+j]=w;
    n2i[w]=i+j;
  }
}

template<class T, class N2I, class I2NMAP>
std::pair<typename T::edge_descriptor, bool> 
add_edge(typename mnr_online_topological_order<T,N2I,I2NMAP>::vertex_descriptor t, 
	 typename mnr_online_topological_order<T,N2I,I2NMAP>::vertex_descriptor h, 
	 mnr_online_topological_order<T,N2I,I2NMAP> &g) {

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
    if(mnr_oto_dfs<T,N2I,I2NMAP>(h,hn2i,tn2i,n2i,g)) {
      throw std::runtime_error("loop detected");
    } else {
      mnr_oto_shift(hn2i,tn2i,n2i,g);
    }
#ifdef MNR_GENERATE_STATS
    ++mnr_ninvalid;
    mnr_ARxy += (tn2i - hn2i + 1);
#endif
  }
}

template<class InputIter, class T, class N2I, class I2NMAP>
void add_edges(InputIter b, InputIter e, 
	       mnr_online_topological_order<T,N2I,I2NMAP> &g) {
  for(;b!=e;++b) {
    add_edge(b->first,b->second,g);
  }
}

template<class T, class N2I = n2i_t, class I2NMAP = std::vector<typename T::vertex_descriptor> >
class mnr_online_topological_order : public T {
public:
  typedef mnr_online_topological_order<T,N2I,I2NMAP> self;
  friend std::pair<typename T::edge_descriptor, bool> add_edge<>(typename self::vertex_descriptor, 
								 typename self::vertex_descriptor, 
								 self &);
  
  friend bool mnr_oto_dfs<>(typename T::vertex_descriptor , unsigned int, unsigned int, 
			    typename boost::property_map<T, N2I>::type &, 
			    self &);
  
  friend void mnr_oto_shift<>(unsigned int, unsigned int, 
			      typename boost::property_map<T, N2I>::type &, 
			      self &);
private:
  I2NMAP _i2n;
  std::vector<bool> _visited;
public:
  mnr_online_topological_order(T const &g, unsigned int acc = 1) 
    : T(g), _visited(num_vertices(g),false) {
    
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

  mnr_online_topological_order(typename T::vertices_size_type n,
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

#endif



