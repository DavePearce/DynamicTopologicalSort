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
// This class implements an ordered linked list, based on the STL
// slist class.  It provides, I believe, an O (log N) complexity result.
//  The ordered asspect means that we can determine in constant 
// time the relative ordering of two nodes in the list.  The data 
// structure was first proposed by Dietz and Sleator [1] and later 
// reformulated [2].
//
// This container has the rather interesting and unusual property
// that it is useful to instantiate it to contain void. This would
// indicate that the user is only interested in the structural 
// information which the container provides. I.e. the relative
// ordering of nodes.
//
// There are potentially some portability issues as I rely on 
// the use of uint32_t and uint64_t and I haven't tested the code
// on different architectures.
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

#ifndef ORDERED_SLIST_HPP
#define ORDERED_SLIST_HPP

#include <cassert>
#include <functional>
#include <boost/iterator_adaptors.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/cstdint.hpp>

// now a slight hack to get unsigned int limits
// on GCC < 3

#if __GNUC__ >= 3
#include <ext/slist>
#include <ext/functional>
// #include <limits>
// #define std::numeric_limits<unsigned int>::max() OL_UINT_MAX
#include <limits.h>
// for some reason UINT_MAX is 4294967295 ?
#define OL_Mm1_CONSTANT UINT_MAX
using __gnu_cxx::slist;
using __gnu_cxx::select2nd;
#else
#include <slist>
#include <limits.h>
#define OL_Mm1_CONSTANT UINT_MAX
using std::slist;
#endif

#ifdef OL_GENERATE_STATS
extern unsigned int ol_ncreated;
extern unsigned int ol_nrelabels;
#endif

class ol_dummy_class {
  // empty!
};

template<class T = ol_dummy_class>
class ordered_slist {
private:
  typedef slist<std::pair<uint32_t, T> > internal_list_t;

  // ==================
  // the basic iterator
  // ==================

  class _iterator {
  public:
    typedef typename internal_list_t::const_iterator::iterator_category iterator_category;
    typedef typename internal_list_t::const_iterator::difference_type difference_type;
    typedef T value_type;
    typedef T* pointer;
    typedef T& reference;
  private:
    typename internal_list_t::iterator _iter;
  public:
    _iterator(void) {}
    _iterator(typename internal_list_t::iterator const &iter) : _iter(iter) {
    }

    T &operator*() { return _iter->second; }
    T *operator->() { return &_iter->second; }

    _iterator const &operator++(void) {
      ++_iter; return *this;
    }
    _iterator operator++(int) {
      _iterator r(*this);++_iter;return r;
    }

    typename internal_list_t::iterator const &base(void) const {
      return _iter;
    }

    bool operator==(_iterator const &src) const {
      return _iter == src._iter;
    }

    bool operator!=(_iterator const &src) const {
      return _iter != src._iter;
    }
  };
  
  // ==================
  // the const iterator
  // ==================

  class _const_iterator {
  public:
    typedef typename internal_list_t::const_iterator::iterator_category iterator_category;
    typedef typename internal_list_t::const_iterator::difference_type difference_type;
    typedef T value_type;
    typedef T const * pointer;
    typedef T const & reference;
  private:
    typename internal_list_t::const_iterator _iter;
  public:
    _const_iterator(void) {}
    _const_iterator(typename internal_list_t::const_iterator const &iter) : _iter(iter) {
    }
    
    _const_iterator(_iterator const &src) : _iter(src.base()) {
    }

    T const &operator*() const { return _iter->second; }
    T const *operator->() const { return &_iter->second; }

    _const_iterator const &operator++(void) {
      ++_iter; return *this;
    }
    _const_iterator &operator++(int) {
      _const_iterator r(*this);++_iter;return r;
    }
    bool operator==(_const_iterator const &src) const {
      return _iter == src._iter;
    }

    bool operator!=(_const_iterator const &src) const {
      return _iter != src._iter;
    }

    typename internal_list_t::const_iterator const &base(void) const {
      return _iter;
    }
  };

public:
  typedef T value_type;
  typedef T *pointer;
  typedef T &reference;
  typedef T const &const_reference;
  typedef typename internal_list_t::size_type size_type;
  typedef typename internal_list_t::difference_type difference_type; // is this correct ?
  typedef _iterator iterator;
  typedef _const_iterator const_iterator;
private:
  struct ol_filler {
    typedef T argument_type;
    typedef std::pair<uint32_t,T> result_type;
    result_type const &operator()(argument_type const &x) const {
      return std::make_pair(0,x);
    }
    result_type &operator()(argument_type &x) {
      return std::make_pair(0,x);
    }
  };
  typedef typename boost::transform_iterator<ol_filler,iterator> fillout_iterator; 
private:
  internal_list_t _list;
  typename internal_list_t::iterator _last;
public:

  // ------------
  // constructors
  // ------------

  ordered_slist(void) : _list() {
  }

  ordered_slist(size_t n) : _list(n) {
    // assign initial priorities
    _init_order();
#ifdef OL_GENERATE_STATS
    ol_ncreated += n;
#endif
  }
  
  ordered_slist(size_t n, T const &t) 
  : _list(n,std::pair<uint32_t,T>(0,t)) {
    _init_order();
#ifdef OL_GENERATE_STATS
    ol_ncreated += n;
#endif
  }

  ordered_slist(ordered_slist const &src) 
    : _list(src._list) {    
    if(!_list.empty()) {
      _last = _list.begin();
      typename internal_list_t::iterator i(_list.begin());
      while(i != _list.end()) { _last=i;++i; }
    }
#ifdef OL_GENERATE_STATS
    ol_ncreated += _list.size();
#endif
  }

  ordered_slist const &operator=(ordered_slist const &src) {
    if(&src != this) {
      _list = src._list;
      if(!_list.empty()) {
	_last = _list.begin();
	typename internal_list_t::iterator i(_list.begin());
	while(i != _list.end()) { _last=i;++i; }
      }      
#ifdef OL_GENERATE_STATS
    ol_ncreated += _list.size();
#endif
    }
  }

  template <class InputIterator>
  ordered_slist(InputIterator f, InputIterator l) 
    : _list(fillout_iterator(f),fillout_iterator(l)) {
    _init_order();
#ifdef OL_GENERATE_STATS
    ol_ncreated += _list.size();
#endif
  }

  // ----------------------
  // const member functions
  // ----------------------

  size_type size(void) const { return _list.size(); }

  size_type max_size(void) const { 
    // this is probably an error
    return _list.max_size(); 
  }

  bool empty(void) const { return _list.empty(); }

  uint32_t order(const_iterator a) const {
    return vb(a.base());
  }

  bool order_lt(const_iterator a, const_iterator b) const {
    return vb(a.base()) < vb(b.base());
  }

  const_iterator begin(void) const { 
    return const_iterator(_list.begin()); 
  }
  const_iterator end(void) const { 
    return const_iterator(_list.end()); 
  }

  const_reference front(void) const {
    return _list.front().second;
  }

  const_iterator previous(const_iterator pos) const {
    return const_iterator(_list.previous(pos.base()));
  }

  // --------------------------
  // non-const member functions  
  // --------------------------

  void swap(ordered_slist &src) {
    _list.swap(src._list);
  }

  void push_front(T const &x) {
    if(!_list.empty()) {
      uint64_t vb_last = vb(_last);
      uint64_t vbs_last = vbs(_last);

      if((vb_last+1) == vbs_last) {
	// need to do some relabeling
	relabel(_last);
	vb_last = vb(_last);
	vbs_last = vbs(_last);
      }

      uint64_t tmp = vb_last;
      tmp = ((uint64_t(vbs_last) + tmp) / 2) + _list.begin()->first;     
      _list.push_front(std::make_pair(tmp,x));    
    } else {
      _list.push_front(std::make_pair(0,x));    
      _last = _list.begin();
    }
#ifdef OL_GENERATE_STATS
    ol_ncreated ++;
#endif
  }

  iterator begin(void) { return iterator(_list.begin()); }
  iterator end(void) { return iterator(_list.end()); }

  reference front(void) {
    return _list.front().second;
  }

  iterator previous(iterator pos) {
    return iterator(_list.previous(pos.base()));
  }

  iterator insert_after(iterator pos) {
    return insert_after(pos,T());
  }

  iterator insert_after(iterator pos, T const &data) {
    assert(pos != end());
    typename internal_list_t::iterator x(pos.base());
    uint64_t vb_x = vb(x);
    uint64_t vbs_x = vbs(x);

    if((vb_x + 1) >= vbs_x) {
      relabel(x);
      vb_x = vb(x);
      vbs_x = vbs(x);
      assert(vb_x < vbs_x);
      assert((vb_x + 1) < vbs_x);
    }

    uint64_t ly = vbs_x;
    ly = (ly + vb_x) / 2 + _list.begin()->first;

    x = _list.insert_after(pos.base(),
				    std::make_pair(ly,data));
    if(pos.base() == _last) { _last = x; }    

#ifdef OL_GENERATE_STATS
    ol_ncreated ++;
#endif

    return iterator(x);
  }

  iterator erase_after(iterator pos) {
    typename internal_list_t::iterator t(pos.base());
    if(++t == _last) {
      _last=pos.base();
    }
    return iterator(_list.erase_after(pos.base()));
  }

  iterator erase_after(iterator before_first, iterator last) {
    if(last == _list.end()) {
      _last=before_first.base();
    }

    return iterator(_list.erase_after(before_first.base(), last.base()));
  }

  // quite a lot left to do here.

private:
  // ----------------
  // helper functions
  // ----------------

  uint64_t vb(typename internal_list_t::const_iterator x) const {
    return x->first - (_list.begin()->first);
  }

  uint64_t vbs(typename internal_list_t::const_iterator x) const {
    if(++x == _list.end()) {
      return uint64_t(OL_Mm1_CONSTANT) + 1;
    } else {
      return vb(x);;
    }
  }

  void _init_order(void) {
    uint64_t tmp(OL_Mm1_CONSTANT);
    uint32_t gap = (tmp+1) / size();
    uint32_t val(gap);
    for(typename internal_list_t::iterator i(_list.begin());
	i!=_list.end();++i, val += gap) {
      i->first = val;
      _last=i;
    }
  }

  void relabel(typename internal_list_t::iterator pos) {
    // --------------------------------------------
    // pre: pos points to the first node which must 
    //      be relabeled. We can easily infer the 
    //      order value for the position before as 
    //      it is pos->first - 1 (because otherwise
    //      we would not have needed to call 
    //      relabel!)
    // --------------------------------------------

    typename internal_list_t::iterator ip(pos);
    typename internal_list_t::iterator jp(pos);
    _advance(ip,1); // as i = 1
    _advance(jp,2); // as j = 2
    unsigned int v0(pos->first);
    unsigned int i(1),j(2),nj;

    while(w(j,jp,v0) <= (4*w(i,ip,v0))) {
      i=i+1;nj=std::min(2*i,size());      
      _advance(ip,1);
      _advance(jp,nj-j);
      j=nj;
    }

    // now perform the reassignment    
    ip=pos; _advance(ip,1);
    uint32_t gap = w(j,jp,v0) / j; 
    uint32_t val(gap);

    for(;ip!=jp;val+=gap,_advance(ip,1)) {      
      ip->first = val + v0;
    }
#ifdef OL_GENERATE_STATS
    ol_nrelabels++;
#endif
  }

  uint64_t w(unsigned int index, 
	     typename internal_list_t::const_iterator x, 
	     unsigned int v0) const {
    if(index == size()) {
      return uint64_t(OL_Mm1_CONSTANT)+1;
    } else {
      return x->first - v0;
    }
  }

  void _advance(typename internal_list_t::iterator &j, unsigned int n) {
    while(n > 0) {
      if(++j == _list.end()) { j=_list.begin(); }
      n=n-1;
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
class ordered_slist<void> 
  : public ordered_slist<ol_dummy_class> {
public:
  ordered_slist(void) 
    : ordered_slist<ol_dummy_class>() {}
  
  ordered_slist(size_t n)
    : ordered_slist<ol_dummy_class>(n) {}
  
  template <class InputIterator>
  ordered_slist(InputIterator f, InputIterator l) 
    : ordered_slist<ol_dummy_class>(f,l) {}

  void push_front(void) {
    ordered_slist<ol_dummy_class>::push_front(ol_dummy_class());
  }
};

#endif








