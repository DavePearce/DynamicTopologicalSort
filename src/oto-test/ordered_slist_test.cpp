// (C) Copyright David James Pearce 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: david.pearce@mcs.vuw.ac.nz

#include <iostream>
#include <sys/time.h>
#include <cmath>
#include <getopt.h>

#include <boost/random.hpp>
#include <boost/lexical_cast.hpp>
#include "ordered_slist.hpp"
#include "ordered_slist2.hpp"

using namespace std;
using namespace boost;

#define MAJOR_VERSION 0
#define MINOR_VERSION 1

#define OPT_HELP 0
#define OPT_VERSION 1
#define OPT_OVER 2
#define OPT_SIZE 3
#define OPT_NCHECK 4

class my_timer {
private:
  struct timeval _start;
public:
  my_timer(void) {
    gettimeofday(&_start,NULL);
  }

  double elapsed(void) {
    struct timeval tmp;
    gettimeofday(&tmp,NULL); 
    double end = tmp.tv_sec + (tmp.tv_usec / 1000000.0);
    double start = _start.tv_sec + (_start.tv_usec / 1000000.0);    
    return end - start;
  }  
};

bool check(ordered_slist<void> &l) {
  ordered_slist<void>::iterator i(l.begin());
  ordered_slist<void>::iterator j(i);
  ++i;

  for(;i!=l.end();++i,++j) {
    if(l.order(j) >= l.order(i)) {
      return false;
    }
  }    

  return true;
}

int main(int argc, char *argv[]) {
  unsigned int over(10),size(10);
  unsigned int divisor(1);
  bool checking(true);

  struct option long_options[]={
    {"help",no_argument,NULL,OPT_HELP},
    {"version",no_argument,NULL,OPT_VERSION},
    {"size",required_argument,NULL,OPT_SIZE},
    {"over",required_argument,NULL,OPT_OVER},
    {"no-checking",no_argument,NULL,OPT_NCHECK},
    NULL
  };

  char *descriptions[]={
    " -h     --help                    display this information",
    " -v     --version                 display version information",
    " -s<x>  --size=<x>                set size of list to operate on",
    " -o<x>  --over=<x>                test over x operations",
    " -d<x>                            divide output by x to simplify",
    " -n     --no-checking             don't perform validation checking",
    NULL
  };

  char v;
  while((v=getopt_long(argc,argv,"hvo:s:d:n",long_options,NULL)) != -1) {
    switch(v)
      {      
      case 'h':
      case OPT_HELP:
	cout << "usage: " << argv[0] << " [options]" << endl;
	cout << "options:" << endl;
	for(char **ptr=descriptions;*ptr != NULL; ptr++) {
	  cout << *ptr << endl;
	}    
	exit(1);
      case 'v':
      case OPT_VERSION:
	cout << argv[0] << " v" << MAJOR_VERSION << "." << MINOR_VERSION << endl;
	cout << "\tWritten by David J. Pearce, Feburary 2003" << endl;
	exit(1);
      case 'o':
      case OPT_OVER:
	over = atoi(optarg);
	break;
      case 's':
      case OPT_SIZE:
	size = atoi(optarg);
	break;
      case 'd':
	divisor=atoi(optarg);
	break;
      case 'n':
      case OPT_NCHECK:
	checking=false;
	break;
      default:
	// getopt will print invalid argument for us
	exit(1);
      }    
  }

  // construct the random number generator
  typedef boost::mt19937 random_gen_t;
  random_gen_t rgen;
  // now, seed it.
  struct timeval tmp;
  gettimeofday(&tmp,NULL);
  rgen.seed((tmp.tv_sec * 1000000) + tmp.tv_usec); 
  // use this adapter to simplify life ...
  boost::random_number_generator<random_gen_t,unsigned int> rng(rgen);
  // construct the list
  ordered_slist<void> l(size);
  // perform the operations
  if(checking && !check(l)) {
    cout << "failure during construction : ";
    for(ordered_slist<void>::iterator i(l.begin());
	i!=l.end();++i) {
      cout << l.order(i) / divisor << " ";
    }
    cout << endl;
    exit(1);
  }

  my_timer t; 

  while(over-- > 0) {
    unsigned int v(rng(1000));
    string type, before;

    if(checking) {
      for(ordered_slist<void>::iterator i(l.begin());
	  i!=l.end();++i) {
	before += boost::lexical_cast<string>(l.order(i) / divisor) + " ";
      }
    }

    if(v >= 500 && !l.empty()) {
      // perform insert
      ordered_slist<void>::iterator x(l.begin()); 
      unsigned int pos(rng(l.size()));
      // I used the following to help test
      // relabelling.
      //unsigned int pos(l.size()-1);
      advance(x,pos);
      l.insert_after(x);
      type = string("insert_after(") + boost::lexical_cast<string>(pos) + ")";
      size++;
    } else {
      l.push_front();
      type = "push_front";
      size++;
    } 
    /*
    else if(l.size() > 1) {
      // perform erase
      ordered_slist<void>::iterator x(l.begin()); 
      unsigned int pos(rng(l.size()-1));
      advance(x,pos);
      //      l.erase_after(x);
      //      size--;
      type = "erase_after (" + boost::lexical_cast<string>(pos) + ")";
    }
    */

    // check the state of the list
    if(checking && (size != l.size() || !check(l))) {
      cout << "failure after \"" << type << "\" : " << endl;
      if(size == l.size()) {
	cout << "\tbefore : " << before << endl;
	cout << "\tafter  : ";      
	for(ordered_slist<void>::iterator i(l.begin());
	    i!=l.end();++i) {
	  cout << l.order(i) / divisor << " ";
	}
      } else {
	cout << "\tsize is " << l.size() << ", but should be " << size << endl;
      }
      cout << endl;
      exit(1);
    }
  }
  cout << t.elapsed() << endl;
}



