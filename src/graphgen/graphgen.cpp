// (C) Copyright David James Pearce 2005. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Email: d.pearce@mcs.vuw.ac.nz

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <getopt.h>
#include <sys/time.h>

#include <set>
#include <vector>
#include <algorithm>
#include <numeric>

// unsure why this is needed
// #define BOOST_NO_STDC_NAMESPACE

#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>
#include "range.hpp"
#include "random_graph.hpp"

using namespace std;
using namespace boost;

#define MAJOR_VERSION 0
#define MINOR_VERSION 6

#define OPT_HELP 0
#define OPT_VERSION 1
#define OPT_VERBOSE 2
#define OPT_NODES 3
#define OPT_EDGES 4 
#define OPT_ACYCLIC 5
#define OPT_OUTDEGREE 6
#define OPT_DENSITY 8
#define OPT_COUNT 10
#define OPT_BINARY 11
#define OPT_OUTPUT 12
#define OPT_PRECISION 13

bool find_replace(string &str, string const &match, string const &replace) {
  unsigned int pos(0);
  string r;
  
  if((pos=str.find(match,pos)) < str.size()) {
    str.replace(pos,match.size(),replace);
    return true;
  }
  return false;
}

unsigned int range2nedges(range<double> r, unsigned int V, unsigned int fmt) {
  switch(fmt) {
  case OPT_EDGES:
    return (unsigned int) (r.value());
    break;
  case OPT_OUTDEGREE:
    return (unsigned int) (r.value() * V);
    break;
  case OPT_DENSITY:
    return (unsigned int) (r.value() * 0.5 * V * (V-1));
    break;
  default:
    throw runtime_error("invalid conversion specified");
  }
}

string double2str(double d, unsigned int precision) {
  ostringstream s;
  s << setprecision(precision) << d;
  return s.str();
}

int main(int argc, char *argv[]) {

  struct option long_options[]={
    {"help",no_argument,NULL,OPT_HELP},
    {"version",no_argument,NULL,OPT_VERSION},
    {"verbose",no_argument,NULL,OPT_VERBOSE},
    {"nodes",required_argument,NULL,OPT_NODES},
    {"edges",required_argument,NULL,OPT_EDGES},
    {"outdegree",required_argument,NULL,OPT_OUTDEGREE},
    {"density",required_argument,NULL,OPT_DENSITY},
    {"binary",required_argument,NULL,OPT_BINARY},
    {"file",required_argument,NULL,OPT_OUTPUT},
    {"precision",required_argument,NULL,OPT_PRECISION},
    {"ngraphs",required_argument,NULL,OPT_COUNT},
    {"acyclic",no_argument,NULL,OPT_ACYCLIC},
    NULL
  };

  char *descriptions[]={
    " -h     --help                    display this information",
    " -v     --version                 display version information",
    "        --verbose                 show addition information",
    " -b     --binary                  produce binary output",
    " -n<x>  --ngraphs                 generate <x> graphs",
    " -v<x>  --nodes=<x>               set value of V (default=50)",
    " -e<x>  --edges=<x>               set value of E (default=50)",
    " -d<x>  --outdegree=<x>           set expected initial outdegree",
    "                                  determintes the probility p to use in the graph",
    "                                  generator and does not *guarantee* an exact outdegree",
    "                                  of x",
    "        --density=<x>             set the ratio of expected actual edges versus the maximum",
    "                                  number of possible edges.",
    " -f<x>  --file=<x>                set output file to x",
    "        --precision=<x>           set precision on output filenames to x",
    "        --acyclic                 generate acyclic graphs only",
    "",
    NULL
  };

  bool verbose = false, binary = false;
  bool acyclic = false;
  double over(0.0005);
  unsigned int over_fixed = 0;
  range<double> Er;
  range<unsigned int> Vr;
  unsigned int conversion = 0;
  unsigned int precision = 5;
  unsigned int count = 1;  
  string outfile("graphs-%V-%E-%N.dat");

  typedef boost::mt19937 UniformRandomNumberGenerator;

  UniformRandomNumberGenerator urng;

  // seed the random number generator
  struct timeval tmp;
  gettimeofday(&tmp,NULL);
  urng.seed((tmp.tv_sec * 1000000) + tmp.tv_usec); 

  try {
    // parse command line arguments
    char v;
    while((v=getopt_long(argc,argv,"hv:n:d:e:o:bf:",long_options,NULL)) != -1) {
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
	case OPT_VERSION:
	  cout << "graphgen v" << MAJOR_VERSION << "." << MINOR_VERSION << endl;
	  cout << "\tWritten by David J. Pearce, January 2005" << endl;
	  exit(1);
	case OPT_VERBOSE:
	  verbose = true;
	  break;
	case 'v':
	case OPT_NODES:
	  Vr = range<unsigned int>(optarg);
	  break;
	case 'e':
	case OPT_EDGES:
	  Er = range<unsigned int>(optarg);
	  conversion = OPT_EDGES;
	  break;
	case 'o':
	case OPT_OUTDEGREE:
	  Er = range<double>(optarg);
	  conversion = OPT_DENSITY;
	  break;
	case 'd':
	case OPT_DENSITY:
	  Er = range<double>(optarg);
	  conversion = OPT_DENSITY;
	  break;
	case 'n':
	case OPT_COUNT:
	  count = atoi(optarg);
	  break;
	case 'b':
	case OPT_BINARY:
	  binary = true;
	  break;
	case 'f':
	case OPT_OUTPUT:
	  outfile = optarg;
	  break;
	case OPT_PRECISION:
	  precision = atoi(optarg);
	  break;
	case OPT_ACYCLIC:
	  acyclic = true;
	  break;
	default:
	  // getopt will print invalid argument for us
	  exit(1);
	}
    }

    // Now, generate the graphs!
    do {
      unsigned int V = Vr.value();

      do {
	
	boost::random_number_generator<UniformRandomNumberGenerator,unsigned int> rng(urng);
	vector<pair<unsigned int, unsigned int> > edges;
	unsigned int E = range2nedges(Er,V,conversion);

	// create filename
	string tmp(outfile);
	string El("e");
	if(conversion == OPT_DENSITY) { El = "d"; }
	else if(conversion == OPT_DENSITY) { El = "o"; }
	while(find_replace(tmp,"%V",string("v") + lexical_cast<string>(V)));
	while(find_replace(tmp,"%E",El + double2str(Er.value(),precision)));
	while(find_replace(tmp,"%N",string("n") + lexical_cast<string>(count)));
	ofstream output(tmp.c_str());

	for(int i=0;i!=count;++i) {   
	  if(verbose) {
	    float p = ((double) i) / count;
	    cerr << "\rv = " << V << ", " << El << " = " << Er.value() << ": Completed " << (p*100) << "%              ";
	  }
	  // generate random edgelist.  Ensure that we always
	  // generate at least E edges.
	  while(E > edges.size()) {
	    edges.clear();
	    if(acyclic) {
	      random_acyclic_edgelist(V,E,edges,urng);
	    } else {
	      random_edgelist(V,E,edges,urng);
	    }
	  }
	  
	  // generate random permutation of edge list
	  random_shuffle(edges.begin(),edges.end(),rng);
	  
	  // now print the graph.    
	  if(binary) {
	    int i = 2, j = 0;
	    int array[512];
	    array[0] = V;
	    array[1] = edges.size();
	    while(j < edges.size()) {
	      for(;i!=512 && j != edges.size();++i,++j) {
		unsigned int w = edges[j].first;
		w = (w << 16) | (edges[j].second & 0xFFFF);
		array[i] = w;
	      }
	      // write the block
	      output.write((char*)array,i*4);
	      i=0;
	    }
	    output.flush();
	  } else {
	    output << "V=" << V << endl;
	    output << "E={";
	    for(int j=0;j!=edges.size();++j) {
	      if(j!=0) { output << ","; }
	      output << edges[j].first << ">" << edges[j].second;
	    }
	    output << "}" << endl;
	  }
	  edges.clear();
	}
	if(verbose) { cout <<  endl; }
      } while(!Er.step());
    } while(!Vr.step());
  } catch(bad_alloc e) {
    cout << "Out of memory (" << e.what() << ")" << endl;
  } catch(runtime_error &e) {
    cout << "Internal failure - " << e.what() << endl;
    exit(1);
  } catch (...) {
    cout << "Internal failure" << endl;
    exit(1);
  }

  fflush(stdout);
  
  exit(0);
}








