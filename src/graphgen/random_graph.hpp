// (C) Copyright David James Pearce 2005. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: d.pearce@mcs.vuw.ac.nz

// Basically, these algorithms are used to
// generate random graphs ...

#ifndef RANDOM_GRAPH_H
#define RANDOM_GRAPH_H

#include <algorithm>
#if __GNUC__ >= 3
#include <ext/algorithm>
#define RANDOM_SAMPLE_N __gnu_cxx::random_sample_n
#else
#define RANDOM_SAMPLE_N random_sample_n
#endif
#include <vector>
#include <stdexcept>
#include <assert.h>
#include <limits.h>
#include <iterator>

#include <boost/iterator/counting_iterator.hpp>
#include <boost/random/geometric_distribution.hpp>
#include <boost/random/random_number_generator.hpp>



// =========================================
// Some Configuration Parameters   
//
// #define FAST_RANDOM_DAG_GENERATOR
// #define PRECISE_FAST_RANDOM_DAG_GENERATOR
// =========================================

class random_dag_iterator {
public:
  typedef std::forward_iterator_tag iterator_category;
  typedef std::pair<unsigned int, unsigned int> value_type;  
  typedef size_t difference_type;
  typedef value_type& reference;
  typedef value_type* pointer;
private:
  unsigned int _node;
  unsigned int _edge;
  unsigned int _V;
public:
  random_dag_iterator(unsigned int n, 
		      unsigned int V)
    : _node(n), _edge(0), _V(V) {    
  }
  bool operator==(random_dag_iterator const &src) {
    return _node == src._node && _edge == src._edge;
  }

  bool operator!=(random_dag_iterator const &src) {
    return _node != src._node || _edge != src._edge;
  }

  random_dag_iterator const &operator++(void) {
    ++_edge;
    if(_edge >= (_V-(_node+1))) {
      _edge=0;
      ++_node;
    }
    return *this;
  }

  std::pair<unsigned int,unsigned int> operator*(void) const {
    return std::make_pair(_node,
			  _node+_edge+1);
  }
};

template<class EdgeList, class UniformRandomNumberGenerator>
void random_acyclic_edgelist(unsigned int V, unsigned int E, 
		     EdgeList &edges,
		     UniformRandomNumberGenerator &urng) {

  boost::random_number_generator<UniformRandomNumberGenerator,unsigned int> rng(urng);
  
  // the algorithm doesn't make any guarantee about the
  // distribution of edges in the edgelist. so, using random_shuffle
  // might be a good idea, if that's the desired result
  
  unsigned int V_VM1_D2((V*(V-1)) / 2); 

  assert(V != 0);
  assert(E < V_VM1_D2);
  
  // first, generate a random topological order by
  // first initialising the vector such rtorder[i]=i
  // and then shuffling it around.
  std::vector<unsigned int> rtorder;
  std::copy(boost::make_counting_iterator<unsigned int>(0),
	    boost::make_counting_iterator<unsigned int>(V),
	    std::back_inserter(rtorder));

  random_shuffle(rtorder.begin(),rtorder.end(),rng);

  // obtain E random samples from the enumeration of edges
  std::vector<unsigned int> sample;
  RANDOM_SAMPLE_N(random_dag_iterator(0,V),
		  random_dag_iterator(V-1,V),
		  std::back_inserter(edges),E,rng);

  // now convert indices into actual nodes according
  // to the order

  for(typename EdgeList::iterator i(edges.begin());
      i!=edges.end();++i) {
    i->first = rtorder[i->first];
    i->second = rtorder[i->second];
  }
}

class random_digraph_iterator {
public:
  typedef std::forward_iterator_tag iterator_category;
  typedef std::pair<unsigned int, unsigned int> value_type;  
  typedef size_t difference_type;
  typedef value_type& reference;
  typedef value_type* pointer;
private:
  unsigned int _node;
  unsigned int _edge;
  unsigned int _V;
public:
  random_digraph_iterator(unsigned int n, unsigned int V)
    : _node(n), _edge(0), _V(V) {    
  }

  bool operator==(random_digraph_iterator const &src) {
    return _node == src._node && _edge == src._edge;
  }

  bool operator!=(random_digraph_iterator const &src) {
    return _node != src._node || _edge != src._edge;
  }

  random_digraph_iterator const &operator++(void) {
    if(++_edge >= _V) {
      _edge=0;
      ++_node;
    }
    return *this;
  }

  std::pair<unsigned int,unsigned int> operator*(void) const {
    return std::make_pair(_node,_edge);
  }
};

template<class EdgeList, class UniformRandomNumberGenerator>
void random_edgelist(unsigned int V, unsigned int E, 
		     EdgeList &edges,
		     UniformRandomNumberGenerator &urng) {

  boost::random_number_generator<UniformRandomNumberGenerator,unsigned int> rng(urng);
  
  // the algorithm doesn't make any guarantee about the
  // distribution of edges in the edgelist. so, using random_shuffle
  // might be a good idea, if that's the desired result
  
  assert(V != 0);
  assert(E < (V*V));
  
  // obtain E random samples from by enumeration of edges
  std::vector<unsigned int> sample;
  RANDOM_SAMPLE_N(random_digraph_iterator(0,V),
		  random_digraph_iterator(V,V),
		  std::back_inserter(edges),E,rng);  
}

#endif
