// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: d.pearce@doc.ic.ac.uk
// 
// Notes
// =====
// This class implements the list of lists structure, briefly 
// described in [1].  It uses the ordered_slist as the base
// and is claimed to provide O(1) amortized time complexity.
//
// Bibliography
// ============
// [1] Two algorithms for Maintaining order in a List, Paul F. Dietz
//     and Daniel D. Sleator, in Proc. 19th Annual ACM Symp. on Theory 
//     of Computing (STOC'87), ACM, New York, 1987, pp. 365-372.
//  
// [2] Two Simplified Algorithms for Maintaining Order in a List,
//     M. A. Bender, R. Cole, E. D. Demaine, M. F.-Colton and J. Zito,
//     In Proc. of the 10th Annual European Symposium on Algorithms,
//     Lecture Notes in Computer Science, 2002, pages 152-164.

#ifndef ORDERED_SLIST2_HPP
#define ORDERED_SLIST2_HPP

#include <cmath>
#include <utility>
#include "ordered_slist.hpp"

#if __GNUC__ >= 3
#include <ext/slist>
#include <ext/functional>
// #include <limits>
// #define std::numeric_limits<unsigned int>::max() OL_UINT_MAX
using __gnu_cxx::slist;
using __gnu_cxx::select2nd;
#else
#include <slist>
using std::slist;
#endif

#ifdef OL2_GENERATE_STATS
extern unsigned int ol2_nrelabels;
extern unsigned int ol2_nrenumbers;
extern unsigned int ol2_ncreated;
#endif

uint32_t ol2_log2M(void) {
  static double r(log(double(OL_Mm1_CONSTANT)+1) / log(2.0));
  return uint32_t(r);
}

#define LOG2M ol2_log2M()

template<class T = ol_dummy_class>
class ordered_slist2 {
private:
  class ol2_label {
  private:
    typedef slist<std::pair<ol2_label,T> > mainlist_t;
    typedef ordered_slist<std::pair<uint32_t, typename mainlist_t::iterator> > sublists_t;
  public:
    typename sublists_t::iterator sublist;
    uint32_t inner_l;
  public:
    ol2_label(void) : inner_l(0) {
    }

    ol2_label(uint32_t il, typename sublists_t::iterator i) 
    : inner_l(il), sublist(i) {
    }
  };
  
  typedef slist<std::pair<ol2_label,T> > mainlist_t;  
  typedef ordered_slist<std::pair<uint32_t, typename mainlist_t::iterator> > sublists_t;

public:
  typedef T value_type;
  typedef T *pointer;
  typedef T &reference;
  typedef T const &const_reference;
  typedef typename mainlist_t::size_type size_type;
  typedef typename mainlist_t::difference_type difference_type; // is this correct ?
  typedef boost::transform_iterator<select2nd<std::pair<ol2_label, T> >, typename mainlist_t::iterator> iterator;
  typedef boost::transform_iterator<select2nd<std::pair<ol2_label, T> >, typename mainlist_t::const_iterator> const_iterator;
private:
  mainlist_t _mainlist;
  sublists_t _sublists;
  typename mainlist_t::iterator _last;
public:
  ordered_slist2(void) {
  }

  ordered_slist2(size_t n) : _mainlist(n) {
    // assign initial priorities
    _initialise();
#ifdef OL2_GENERATE_STATS
    ol2_ncreated += n;
#endif
  }
  
  ordered_slist2(size_t n, T const &t) : _mainlist(n,std::make_pair(ol2_label(),t)) {
    _initialise();
#ifdef OL2_GENERATE_STATS
    ol2_ncreated += n;
#endif
  }

  ordered_slist2(ordered_slist2 const &src) {
    _copy(src);
  }

  ordered_slist2 const &operator=(ordered_slist2 const &src) {
    if(this != &src) {
      _copy(src);
    }

    return *this;
  }

  size_t size(void) const { return _mainlist.size(); }

  bool empty(void) const { return _mainlist.empty(); }

  uint64_t order(const_iterator a) const {
    typename mainlist_t::const_iterator a_x(a.base());
    typename sublists_t::const_iterator a_sublist(a_x->first.sublist);
    uint64_t r(_sublists.order(a_sublist));
    r = r << 32U;
    r += vb(a_x,a_sublist);
    return r;
  }

  bool order_lt(const_iterator a, const_iterator b) const {
    typename mainlist_t::const_iterator a_x(a.base());
    typename sublists_t::const_iterator a_sublist(a_x->first.sublist);
    typename mainlist_t::const_iterator b_x(b.base());
    typename sublists_t::const_iterator b_sublist(b_x->first.sublist);

    if(a_sublist == b_sublist) {
      return vb(a_x,a_sublist) < vb(b_x,a_sublist);
    } else {
      return _sublists.order(a_sublist) < _sublists.order(b_sublist);
    }    
  }

  void push_front(T const &x) {
    if(_mainlist.empty()) {
      _mainlist.push_front(std::make_pair(ol2_label(),x));
      // initial outer list has size 1
      _sublists.push_front(std::make_pair(1,_mainlist.begin()));
      // now setup the back pointer
      _mainlist.begin()->first.sublist = _sublists.begin();
      // initialise _last pointer
      _last=_mainlist.begin();
    } else {
      // compute the new label
      uint32_t label=_compute_label(last(_sublists.begin()),_sublists.begin());
      _mainlist.push_front(std::make_pair(ol2_label(label,_sublists.begin()),x));      
      ++_sublists.begin()->first; // increment list size
      _sublists.begin()->second = _mainlist.begin(); // reseat start pointer
      
      if(_sublists.begin()->first >= LOG2M) {
	// adding new item will push list over
	// the size limit
	_split(_sublists.begin());
      }
    }
  }

  reference front(void) {
    return _mainlist.front().second;
  }

  iterator previous(iterator pos) {
    return iterator(_mainlist.previous(pos.base()));
  }

  iterator insert_after(iterator pos) {
    return insert_after(pos,T());
  }

  iterator insert_after(iterator pos, T const &data) {
    assert(pos != end());
    typename mainlist_t::iterator x(pos.base());
    typename sublists_t::iterator sublist(x->first.sublist);
    
    uint32_t label=_compute_label(x,sublist);
    
    x = _mainlist.insert_after(x,std::make_pair(ol2_label(label,sublist),data));
    if(pos.base() == last(sublist)) { _set_last(x,sublist); }    

    ++sublist->first; // increase size
    if(sublist->first >= LOG2M) {
      _split(sublist);
    } 

    return iterator(x);
  }

  iterator begin(void) { return iterator(_mainlist.begin()); }
  iterator end(void) { return iterator(_mainlist.end()); }

  const_iterator begin(void) const { return const_iterator(_mainlist.begin()); }
  const_iterator end(void) const { return const_iterator(_mainlist.end()); }
private:  
  void _copy(ordered_slist2 const &src) {
    _mainlist = src._mainlist;
    _sublists = src._sublists;
    // now, we must reseat all the
    // internal iterators
    typename mainlist_t::iterator i(_mainlist.begin());
    typename sublists_t::iterator j(_sublists.begin());

    _last = _mainlist.begin();

    while(j != _sublists.end()) {
      j->second = _last; // reseat sublist start
      // assume a sublist can never be empty
      unsigned int count(j->first);
      assert(count != 0);
      while(count > 0) {
	i->first.sublist = j; // reseat backptr
	_last = i;
	++i;--count;
      }
      ++j;
    }

#ifdef OL2_GENERATE_STATS
    ol2_ncreated += _mainlist.size();
#endif
  }

  void _initialise(void) {
    unsigned int n(size());

    typename mainlist_t::iterator i(_mainlist.begin());
    typename sublists_t::iterator j(_sublists.begin());

    while(n > 0) {
      unsigned int chunk(std::min(n,LOG2M));
      n -= chunk;

      if(j == _sublists.end()) {
	_sublists.push_front(std::make_pair(chunk,i));
	j=_sublists.begin();
      } else {
	// notice the subtle use of _last here ...
	j=_sublists.insert_after(j,std::make_pair(chunk,_last));
      }
      
      uint64_t tmp(OL_Mm1_CONSTANT);
      uint32_t gap = (tmp+1) / (chunk+1);
      uint32_t val(gap);
      
      for(;chunk != 0;++i, --chunk, val += gap) {
	i->first.sublist = j;
	i->first.inner_l = val;
	_last = i; 
      }
    }
  }

  void _renumber(typename sublists_t::iterator sublist) {
    unsigned int chunk(sublist->first);
    uint64_t tmp(OL_Mm1_CONSTANT);
    uint32_t gap = (tmp+1) / (chunk+1);
    uint32_t val(gap);
    typename mainlist_t::iterator i(first(sublist));
    
    for(;chunk != 0;++i, --chunk, val += gap) {
      i->first.sublist = sublist;
      i->first.inner_l = val;
    }    
#ifdef OL2_GENERATE_STATS
    ol2_nrenumbers ++;
#endif
  }

  uint32_t _compute_label(typename mainlist_t::iterator x, 
			  typename sublists_t::iterator sublist) {
    // the aim of this function is to compute a new value
    // for a label to come after x.  This may require a certain
    // amount of relabeling to be done.
    
    uint64_t vb_x = vb(x,sublist);
    uint64_t vbs_x = vbs(x,sublist);
    
    if((vb_x+1) == vbs_x) {
      // need to do some relabeling
      _relabel(x,sublist);
      vb_x = vb(x,sublist);
      vbs_x = vbs(x,sublist);
    }
    
    uint64_t tmp = vb_x;
    tmp = ((uint64_t(vbs_x) + tmp) / 2) + first(sublist)->first.inner_l;
    // actually assign
    return tmp;
  }

  void _relabel(typename mainlist_t::iterator pos, 
			  typename sublists_t::iterator sublist) {
    // --------------------------------------------
    // pre: pos points to the first node which must 
    //      be relabeled. We can easily infer the 
    //      order value for the position before as 
    //      it is pos->first - 1 (because otherwise
    //      we would not have needed to call 
    //      relabel!)
    // --------------------------------------------

    typename mainlist_t::iterator ip(pos);
    typename mainlist_t::iterator jp(pos);
    _advance(ip,1,sublist); // as i = 1
    _advance(jp,2,sublist); // as j = 2
    unsigned int v0(pos->first.inner_l);
    unsigned int i(1),j(2),nj;

    while(w(j,jp,v0) <= (4*w(i,ip,v0))) {
      i=i+1;nj=min(2*i,sublist->first);      
      _advance(ip,1,sublist);
      _advance(jp,nj-j,sublist);
      j=nj;
    }

    // now perform the reassignment    
    ip=pos; _advance(ip,1,sublist);
    uint32_t gap = w(j,jp,v0) / j; 
    uint32_t val(gap);

    for(;ip!=jp;val+=gap,_advance(ip,1,sublist)) {      
      ip->first.inner_l = val + v0;
    }
#ifdef OL2_GENERATE_STATS
    ol2_nrelabels ++;
#endif
  }

  void _split(typename sublists_t::iterator sublist) {
    // this function splits sublist into two 
    // half size lists.
    unsigned int count(sublist->first / 2);
    typename mainlist_t::iterator i(first(sublist));
    advance(i,count-1); // move to the halfway point
    typename sublists_t::iterator newlist;
    newlist = _sublists.insert_after(sublist,std::make_pair(sublist->first-count,i));
    sublist->first = count;
    // now, renumber at most log N records.
    _renumber(sublist);
    _renumber(newlist);
  }
  
  uint64_t w(unsigned int index, 
	     typename mainlist_t::iterator x, 
	     unsigned int v0) {
    if(index == x->first.sublist->first) {
      return uint64_t(OL_Mm1_CONSTANT)+1;
    } else {
      return x->first.inner_l - v0;
    }
  }

  void _advance(typename mainlist_t::iterator &j, unsigned int n, 
		typename sublists_t::iterator &sublist) {
    typename mainlist_t::iterator jend(last(sublist));
    while(n > 0) {
      if(j == jend) { 
	j = first(sublist); 
      } else { ++j; }
      n=n-1;
    }
  }

  void _set_last(typename mainlist_t::iterator newlast, 
		 typename sublists_t::iterator sublist) {
    if(++sublist == _sublists.end()) {
      _last=newlast;
    } else {
      sublist->second = newlast;
    }
  }

  typename mainlist_t::iterator first(typename sublists_t::const_iterator sublist) const {
    // returns first iterator of sublist
    typename mainlist_t::iterator r(sublist->second);
    if(sublist != _sublists.begin()) { ++r; }
    return r;
  }
  
  typename mainlist_t::iterator last(typename sublists_t::const_iterator sublist) const {
    // returns last iterator of sublist
    if(++sublist == _sublists.end()) {
      return _last;
    } else {
      // remeber iterator of next "sublist"
      // is actually my last iterator.
      return sublist->second;
    }
  }

  uint64_t vb(typename mainlist_t::const_iterator x,
	      typename sublists_t::const_iterator sublist) const {
    return x->first.inner_l - (first(sublist)->first.inner_l);
  }

  uint64_t vbs(typename mainlist_t::const_iterator x,
	       typename sublists_t::const_iterator sublist) const {
    if(x == last(sublist)) {
      return uint64_t(OL_Mm1_CONSTANT) + 1;
    } else {
      return vb(++x,sublist);;
    }
  }
};

// -------------------------------------------
// The following specialisation is for a void
// element type. This may seem pointless, but
// there are situations when you just want a
// "priority space" and don't need a container
// as such.  Personally, I think the idea of
// a container which doesn't contain anything
// as quite interesting ...
// -------------------------------------------

template<>
class ordered_slist2<void> 
  : public ordered_slist2<ol_dummy_class> {
public:
  ordered_slist2(void) {
  }
  
  ordered_slist2(size_t n) 
    : ordered_slist2<ol_dummy_class>(n) {
  }
  
  void push_front(void) {
    ordered_slist2<ol_dummy_class>::push_front(ol_dummy_class());
  }
};

#endif





