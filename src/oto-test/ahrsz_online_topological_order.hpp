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
// This is another solution to the online topological 
// order problem.  It was first described in the 
// following:
//
// [1] B. Alpern, R. Hoover, B. K. Rosen, P. F. Sweeney and F. K. 
//     Zadeck, "Incremental Evaluation of Computational Circuits", 
//     In Proc. of the First Annual ACM-SIAM Symposium on Discrete 
//     Algorithms, pages 32-42, 1990.

#ifndef AHRSZ_ONLINE_TOPOLOGICAL_ORDER_HPP
#define AHRSZ_ONLINE_TOPOLOGICAL_ORDER_HPP

#include <queue>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <boost/property_map.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/lexical_cast.hpp>

#include "ordered_slist.hpp"
#include "ordered_slist2.hpp"
#include "oto_tags.hpp"
#include "myless.hpp"
#include "mygreater.hpp"

#if __GNUC__ >= 3
#include <ext/functional>
using __gnu_cxx::select2nd;
using __gnu_cxx::identity;
#else
using std::select2nd;
using std::identity;
#endif

template<class T, class PSPACE, class N2I>
class ahrsz_online_topological_order;

// The arbitrary assignment algorithm
// is slightly simpler but does not
// minimise the number of new priorities 
// created. 

// #define AHRSZ_USE_SIMPLE_REASSIGNMENT

// These globals are not strictly needed
// they are used to generate the metrics
#ifdef AHRSZ_GENERATE_STATS
extern unsigned int ahrsz_K;
extern unsigned int ahrsz_dKfb;
extern unsigned int ahrsz_ninvalid;
#endif

// --------------------------------
// priority compare 
//
// This gives a comparison based
// on the priority of a node
// --------------------------------

template<class T, class N2I, class Comp>
class ahrsz_priority_comp {
private:
  typedef typename boost::property_map<T, N2I>::type priority_map;
private:
  Comp _comp;
  priority_map _pmap; // efficiency  
public:
  ahrsz_priority_comp(priority_map pmap)
    : _pmap(pmap) {
  }

  bool operator()(typename T::vertex_descriptor l, 
		  typename T::vertex_descriptor r) const {
    return _comp(_pmap[l],_pmap[r]);
  }
};

template<class PSPACE = ordered_slist<void> >
class ahrsz_priority_value {
private:
  typename PSPACE::iterator _iter;
  PSPACE  const *_ptr;
public:
  ahrsz_priority_value(void) 
    : _iter(), _ptr(NULL) {
  }

  ahrsz_priority_value(typename PSPACE::iterator const &iter, 
		       PSPACE const &pspace) 
  : _iter(iter), _ptr(&pspace) {    
  }

  bool operator<(ahrsz_priority_value const &src) const {    
    assert(_ptr != NULL);
    return _ptr->order_lt(_iter,src._iter);
  }

  bool operator>(ahrsz_priority_value const &src) const {
    assert(_ptr != NULL);
    return _iter != src._iter && !(*this < src);
  }

  bool operator!=(ahrsz_priority_value const &src) const {
    return _iter != src._iter;
  }

  bool operator==(ahrsz_priority_value const &src) const {
    return _iter == src._iter;
  }

  typename PSPACE::iterator &base(void) {
    return _iter;
  }

  ahrsz_priority_value const &operator++(int) {
    ++_iter;
    return *this;
  }

  ahrsz_priority_value operator++(void) {
    ahrsz_priority_value r(*this);
    ++_iter;
    return r;
  }
};

typedef enum {minus_infinity,plus_infinity} ahrsz_ext_priority_value_inf_t;

template<class PSPACE>
class ahrsz_ext_priority_value {
private:
  ahrsz_priority_value<PSPACE> _val;
  bool _minus_inf;
  bool _plus_inf;
public:
  ahrsz_ext_priority_value(ahrsz_ext_priority_value_inf_t f = minus_infinity) 
    : _val(), _minus_inf(f == minus_infinity), _plus_inf(f == plus_infinity) {    
  }

  ahrsz_ext_priority_value(ahrsz_priority_value<PSPACE> &e) 
    : _val(e), _minus_inf(false), _plus_inf(false) {    
  }
  
  ahrsz_ext_priority_value(typename PSPACE::iterator const &iter,
			   PSPACE const &space) 
    : _val(iter,space), _minus_inf(false), _plus_inf(false) {    
  }

  bool minus_inf(void) const { return _minus_inf; }
  bool plus_inf(void) const { return _plus_inf; }

  bool operator<(ahrsz_ext_priority_value const &src) const {
    if(_minus_inf || src._minus_inf) {
      return _minus_inf && !src._minus_inf;
    } else if(_plus_inf || src._plus_inf) {
      return src._plus_inf && !_plus_inf;
    } 
    return _val < src._val;
  }

  bool operator>=(ahrsz_ext_priority_value const &src) const {
    return !(*this < src);
  }

  bool operator==(ahrsz_ext_priority_value const &src) const {
    if(_minus_inf || src._minus_inf) {
      return _minus_inf == src._minus_inf;
    } else if(_plus_inf || src._plus_inf) {
      return _plus_inf == src._plus_inf;
    }
    return _val == src._val;
  }

  typename PSPACE::iterator &base(void) {
    return _val.base();
  }

  ahrsz_ext_priority_value const &operator++(int) {
    ++_val;
    return *this;
  }

  ahrsz_ext_priority_value operator++(void) {
    ahrsz_ext_priority_value r(*this);
    ++_val;
    return r;
  }
};

// --------------
// the main class
// --------------

template<class T, class PSPACE, class N2I>
std::pair<typename T::edge_descriptor, bool> 
add_edge(typename ahrsz_online_topological_order<T,PSPACE,N2I>::vertex_descriptor t, 
	 typename ahrsz_online_topological_order<T,PSPACE,N2I>::vertex_descriptor h, 
	 ahrsz_online_topological_order<T,PSPACE,N2I> &g) {

  std::pair<typename T::edge_descriptor, bool> r;
  r = add_edge(static_cast<typename T::vertex_descriptor>(t),
	       static_cast<typename T::vertex_descriptor>(h),
	       static_cast<T&>(g));
  if(r.second) {
    typename boost::property_map<T, N2I>::type _pmap; 
    _pmap = get(N2I(),g);
    if(!(_pmap[t] < _pmap[h])) {
      std::vector<typename T::vertex_descriptor> K;     
      g.discovery(t,h,K);  
      g.reassignment(K);
#ifdef AHRSZ_GENERATE_STATS
      ++ahrsz_ninvalid;
      ahrsz_K += K.size();
#endif
    }
  }
}


template<class InputIter, class T, class P, class N2I>
void add_edges(InputIter b, InputIter e, 
	       ahrsz_online_topological_order<T,P,N2I> &g) {
  for(;b!=e;++b) {
    add_edge(b->first,b->second,g);
  }
}

template<class T, class PSPACE = ordered_slist<void>, class N2I = n2i_t>
class ahrsz_online_topological_order : public T {
public:  
  typedef ahrsz_online_topological_order<T,PSPACE,N2I> self;
  friend std::pair<typename T::edge_descriptor, bool> add_edge<T>(typename self::vertex_descriptor, 
								  typename self::vertex_descriptor, 
								  self &);
private:
  typedef typename self::vertex_descriptor vd_t;
  typedef typename T::out_edge_iterator out_iterator;
  typedef typename T::in_edge_iterator in_iterator;
  typedef std::priority_queue<vd_t,std::vector<vd_t>,ahrsz_priority_comp<
    T,N2I,std::less<ahrsz_priority_value<PSPACE> > > > max_priority_queue;
  typedef std::priority_queue<vd_t,std::vector<vd_t>,ahrsz_priority_comp<
    T,N2I,std::greater<ahrsz_priority_value<PSPACE> > > > min_priority_queue;
  typedef typename boost::property_map<T, N2I>::type n2i_t;

  PSPACE _pspace; // the priority space.
  // temporary storage needed by the reassignment stage
  std::vector<ahrsz_ext_priority_value<PSPACE> > _ceiling;
  std::vector<bool> _visited;
  std::vector<bool> _inK;
#ifndef AHRSZ_USE_SIMPLE_REASSIGNMENT
  std::vector<unsigned int> _indegree;
#endif
public:
  ahrsz_online_topological_order(T const &g, unsigned int a = 1) 
    : T(g), 
      _pspace(1), 
      _ceiling(num_vertices(g),minus_infinity),
      _visited(num_vertices(g),false),
      _inK(num_vertices(g),false)
#ifndef AHRSZ_USE_SIMPLE_REASSIGNMENT
      ,_indegree(num_vertices(g),0) 
#endif
  {    
    typename boost::property_map<T, N2I>::type _pmap = get(N2I(),*this);
    
    std::vector<typename T::vertex_descriptor> tmp;

    typename T::vertex_iterator i,iend;
    for(tie(i,iend) = vertices(*this);i!=iend;++i) {
      _pmap[*i] = ahrsz_priority_value<PSPACE>(_pspace.begin(),_pspace);
      tmp.push_back(*i);
    }

    reassignment(tmp);
  }

  ahrsz_online_topological_order(typename T::vertices_size_type n, unsigned int a = 1) 
    : T(n), _pspace(1), _ceiling(n,minus_infinity),
      _visited(n,false), _inK(n,false)
#ifndef AHRSZ_USE_SIMPLE_REASSIGNMENT
      ,_indegree(n,0) 
#endif
{
    typename boost::property_map<T, N2I>::type _pmap = get(N2I(),*this);
    
    typename T::vertex_iterator i,iend;
    for(tie(i,iend) = vertices(*this);i!=iend;++i) {
      _pmap[*i] = ahrsz_priority_value<PSPACE>(_pspace.begin(),_pspace);
      assert(!(_pmap[*i] < _pmap[*i]));
    }
  }

  ahrsz_online_topological_order(ahrsz_online_topological_order const &src) {
    copy(src);
  }

  void operator=(ahrsz_online_topological_order const &src) {
    if(&src != this) {
      copy(src);
    }
  }
protected:

  // -----------------------
  // the discovery algorithm
  // -----------------------
  
  void discovery(vd_t tail, vd_t head,
		 std::vector<vd_t> &K) 
  {
    n2i_t n2i(get(N2I(),*this));

    // -------------
    // Function Code
    // -------------
    
    vd_t f(head);
    vd_t b(tail);  
    min_priority_queue ForwFron(get(N2I(),*this));  // the forward frontier
    max_priority_queue BackFron(get(N2I(),*this));  // the backward frontier
    unsigned int ForwEdges(out_degree(head,*this));
    unsigned int BackEdges(in_degree(tail,*this));
    ForwFron.push(head);
    BackFron.push(tail);

    // notice, I uses visited to indicate when 
    // a node is on one of the queues.
    _visited[head]=true;
    _visited[tail]=true;    

    while(!(n2i[f] > n2i[b]) && !ForwFron.empty() && !BackFron.empty()) {
      unsigned int u=std::min(ForwEdges,BackEdges);
      ForwEdges -= u;
      BackEdges -= u;

      if(ForwEdges == 0) {
	K.push_back(f);
	ForwFron.pop();
#ifdef AHRSZ_GENERATE_STATS
	++ahrsz_dKfb;
#endif
	_visited[f]=false;
	out_iterator i,iend;
	for(tie(i,iend) = out_edges(f,*this);i!=iend;++i) {
	  vd_t w(target(*i,*this));
	  if(!_visited[w]) {
	    ForwFron.push(w);	
	    _visited[w]=true;
	  }
#ifdef AHRSZ_GENERATE_STATS
	  ++ahrsz_dKfb;
#endif
	}
	if(ForwFron.empty()) {
	  f = tail;
	} else {	  
	  f = ForwFron.top();
	}
	ForwEdges = out_degree(f,*this);
      }
      if(BackEdges == 0) {
	K.push_back(b);
	BackFron.pop();
#ifdef AHRSZ_GENERATE_STATS
	++ahrsz_dKfb;
#endif
	_visited[b]=false;
	in_iterator i,iend;
	for(tie(i,iend) = in_edges(b,*this);i!=iend;++i) {
	  vd_t w(source(*i,*this));
	  if(!_visited[w]) {
	    BackFron.push(w);	
	    _visited[w]=true;
	  }
#ifdef AHRSZ_GENERATE_STATS
	  ++ahrsz_dKfb;
#endif
	}
	if(BackFron.empty()) {
	  b = head;
	} else {
	  b = BackFron.top();
	}
	BackEdges = in_degree(b,*this);
      }
    }
    // Finally, unmark all remaining on queues
    while(!ForwFron.empty()) {
      _visited[ForwFron.top()]=false;
      ForwFron.pop();
    }
    while(!BackFron.empty()) {
      _visited[BackFron.top()]=false;
      BackFron.pop();
    }
  }

  std::string a2str(ahrsz_ext_priority_value<PSPACE> x) {
    if(x.minus_inf()) {
      return "-oo";
    } else if(x.plus_inf()){
      return "+oo";
    } 

    uint64_t r(_pspace.order(x.base()));
    return boost::lexical_cast<std::string>(r);
  }

  std::string a2str(ahrsz_priority_value<PSPACE> x) {
    uint64_t r(_pspace.order(x.base()));
    return boost::lexical_cast<std::string>(r);
  }

  // --------------------------
  // the reassignment algorithm
  // --------------------------
  
  void reassignment(std::vector<vd_t> &K) {
    n2i_t n2i(get(N2I(),*this));
    // Initialise temporary data
    for(typename std::vector<vd_t>::iterator i(K.begin());
	i!=K.end();++i) {
      _ceiling[*i] = plus_infinity;
      _inK[*i] = true;
    }
    // first pass - compute ceiling Information
#ifdef AHRSZ_USE_SIMPLE_REASSIGNMENT
    std::vector<vd_t> rto; // reverse topological order
    for(typename std::vector<vd_t>::iterator i(K.begin());
	i!=K.end();++i) {
      if(!_visited[*i]) { compute_ceiling(*i,rto,n2i); }
    }

    // second pass - perform the reassignment
    for(typename std::vector<vd_t>::reverse_iterator i(rto.rbegin());i!=rto.rend();++i) {    
      ahrsz_ext_priority_value<PSPACE> ep(compute_priority(compute_floor(*i,n2i),_ceiling[*i]));
      n2i[*i] = ahrsz_priority_value<PSPACE>(ep.base(),_pspace);
    }    
#else    
    // first pass - compute ceiling Information
    for(typename std::vector<vd_t>::iterator i(K.begin());
	i!=K.end();++i) {
      if(!_visited[*i]) { compute_ceiling(*i,n2i); }
    }
    
    // construct the min-priority queue
    typedef std::pair<vd_t,ahrsz_ext_priority_value<PSPACE> > Q_e;
    typedef my_greater<Q_e,select2nd<Q_e> > Q_c;
    typedef std::priority_queue<Q_e,std::vector<Q_e>,Q_c> Q_t;
    Q_t Q;

    // initialise Q
    for(typename std::vector<vd_t>::iterator i(K.begin());i!=K.end();++i) {
      ahrsz_ext_priority_value<PSPACE> floor(minus_infinity);      
      unsigned int kid = 0;
      typename T::in_edge_iterator j,jend;
      for(tie(j,jend) = in_edges(*i,*this);j!=jend;++j) {
	if(_inK[source(*j,*this)]) { ++kid; }
	floor = std::max(floor,ahrsz_ext_priority_value<PSPACE>(n2i[source(*j,*this)]));
      }
      _indegree[*i] = kid;
      if(kid == 0) { Q.push(Q_e(*i,floor)); }
    }

    // now perform the reassignment
    std::vector<vd_t> Z;
    while(!Q.empty()) {
      Z.clear();
      ahrsz_ext_priority_value<PSPACE> Z_floor = Q.top().second;
      ahrsz_ext_priority_value<PSPACE> Z_ceil = plus_infinity;
      while(!Q.empty() && Q.top().second == Z_floor) {
	vd_t x = Q.top().first;	
	Z_ceil = std::min(Z_ceil,_ceiling[x]);
	Z.push_back(x);
	Q.pop();
      }
      
      assert(Q.empty() || Z_floor < Q.top().second);

      // all of Z must get same priority
      ahrsz_ext_priority_value<PSPACE> Z_p = compute_priority(Z_floor,Z_ceil);

      for(typename std::vector<vd_t>::const_iterator i(Z.begin());i!=Z.end();++i) {
	n2i[*i] = ahrsz_priority_value<PSPACE>(Z_p.base(),_pspace);
	typename T::out_edge_iterator j,jend;
	for(tie(j,jend) = out_edges(*i,*this);j!=jend;++j) {
	  vd_t y(target(*j,*this));
	  if(_inK[y] && --_indegree[y] == 0) {
	    Q.push(std::make_pair(y,compute_floor(y,n2i)));
	  }
	}
      } 
    }
#endif

    // reset visited information
    for(typename std::vector<vd_t>::iterator i(K.begin());i!=K.end();++i) {
      _visited[*i]=false;
      _inK[*i]=false;
    }
  }
  
  void compute_ceiling(vd_t n, 
#ifdef AHRSZ_USE_SIMPLE_REASSIGNMENT
		       std::vector<vd_t> &rto, 
#endif
		       n2i_t n2i) {
    _visited[n]=true;    
    typename T::out_edge_iterator i,iend;
    for(tie(i,iend) = out_edges(n,*this);i!=iend;++i) {
      vd_t j(target(*i,*this));

      if(_inK[j]) {
	// successor is in K
#ifdef AHRSZ_USE_SIMPLE_REASSIGNMENT
	if(!_visited[j]) { compute_ceiling(j,rto,n2i); }
#else
	if(!_visited[j]) { compute_ceiling(j,n2i); }
#endif
	_ceiling[n] = std::min(_ceiling[j],_ceiling[n]);
      } else {
	// successor is not in K
	_ceiling[n] = std::min(_ceiling[n],ahrsz_ext_priority_value<PSPACE>(n2i[j]));
      }       
    }
#ifdef AHRSZ_USE_SIMPLE_REASSIGNMENT
    rto.push_back(n);
#endif
  }  

  ahrsz_ext_priority_value<PSPACE>
  compute_priority(ahrsz_ext_priority_value<PSPACE> floor, 
		   ahrsz_ext_priority_value<PSPACE> ceiling) {
    assert(floor < ceiling);
    ahrsz_ext_priority_value<PSPACE> candidate;

    if(floor.minus_inf()) {
      candidate = ahrsz_ext_priority_value<PSPACE>(_pspace.begin(),_pspace);
      if(candidate >= ceiling) {
	// we need a new priority
	_pspace.push_front();
	candidate = ahrsz_ext_priority_value<PSPACE>(_pspace.begin(),_pspace);
      }
    } else {
      assert(!floor.plus_inf());
      candidate = floor;
      ++candidate;
      if(candidate.base() == _pspace.end() || candidate >= ceiling) {
	// we need a new priority
	candidate = ahrsz_ext_priority_value<PSPACE>(_pspace.insert_after(floor.base()),_pspace);
      }
    }
    assert(floor < candidate && candidate < ceiling);
    return candidate;
  }
  
  ahrsz_ext_priority_value<PSPACE> compute_floor(vd_t v, n2i_t n2i) {
    ahrsz_ext_priority_value<PSPACE> floor(minus_infinity);
    
    typename T::in_edge_iterator j,jend;
    for(tie(j,jend) = in_edges(v,*this);j!=jend;++j) {
      floor = std::max(floor,ahrsz_ext_priority_value<PSPACE>(n2i[source(*j,*this)]));
    }

    return floor;
  }
   
private:
  void copy(ahrsz_online_topological_order const &src) {
    // invoke super assignment operator
    T::operator=(src);    

    // direct copy
    _pspace=PSPACE(1);
    typename boost::property_map<T, N2I>::type _pmap = get(N2I(),*this);
    std::vector<typename T::vertex_descriptor> tmp;

    typename T::vertex_iterator i,iend;
    for(tie(i,iend) = vertices(*this);i!=iend;++i) {
      _pmap[*i] = ahrsz_priority_value<PSPACE>(_pspace.begin(),_pspace);
      tmp.push_back(*i);
    }

    reassignment(tmp);
  }
};

#endif




