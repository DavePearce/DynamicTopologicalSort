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
// A test harness for various online topological order algorithms.
// This can be used for timing, validation and extracting other
// bits of useful info.  This was used to generate the data
// in chapter 2 of my PhD thesis.

#include <iostream>
#include <iomanip>
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

// indicate algorithms should
// generate metrics
#define AHRSZ_GENERATE_STATS
#define MNR_GENERATE_STATS
#define POTO1_GENERATE_STATS
#define SOTO_GENERATE_STATS
#define OL_GENERATE_STATS
#define OL2_GENERATE_STATS

#include <boost/random.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/copy.hpp>
#include <boost/lexical_cast.hpp>
#include "stats.hpp"
#include "range.hpp"
#include "graphgen.hpp"
#include "path.hpp"
#include "mnr_online_topological_order.hpp"
#include "poto1_online_topological_order.hpp"
#include "ahrsz_online_topological_order.hpp"
#include "dummy_online_topological_order.hpp"
#include "simple_topological_order.hpp"

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

using namespace std;
using namespace boost;

// Notice that I use vecS here, which does not prevent parallel edges.  
// However, the input graph model will only ever insert an edge once.

typedef adjacency_list<vecS,vecS, bidirectionalS, property<n2i_t,unsigned int> > subgraph1_t;
typedef adjacency_list<vecS,vecS, bidirectionalS, property<n2i_t,ahrsz_priority_value<> > > subgraph2_t;
typedef adjacency_list<vecS,vecS, bidirectionalS, property<n2i_t,ahrsz_priority_value<ordered_slist2<void> > > > subgraph3_t;

typedef poto1_online_topological_order<subgraph1_t> POTO1_graph_t;
typedef mnr_online_topological_order<subgraph1_t> MNR_graph_t;
typedef ahrsz_online_topological_order<subgraph2_t> AHRSZb_graph_t;
typedef ahrsz_online_topological_order<subgraph3_t, ordered_slist2<void> > AHRSZ_graph_t;
typedef simple_topological_order<subgraph1_t> DFS_graph_t;
typedef dummy_online_topological_order<subgraph1_t> dummy_graph_t;

#define MAJOR_VERSION 3
#define MINOR_VERSION 1

#define OPT_HELP 0
#define OPT_VERSION 1
#define OPT_NODES 2
#define OPT_CHECKING 3
#define OPT_RATIO 6
#define OPT_MNR 7
#define OPT_DFS 8
#define OPT_QUIET 9
#define OPT_CYCLES 10
#define OPT_OUTDEGREE 12
#define OPT_AHRSZ 13
#define OPT_AHRSZE 14
#define OPT_POTO1 16
#define OPT_SAMPLE 20
#define OPT_AHRSZB 22
#define OPT_BATCH 25
#define OPT_DENSITY 26
#define OPT_DUMMY 28
#define OPT_VERBOSE 29
#define OPT_SAMPLEFIXED 30
#define OPT_INPUT 31
#define OPT_NGRAPHS 32
#define OPT_EDGES 33

// ----------------
// Global Variables
// ----------------

unsigned int ahrsz_ninvalid = 0;
unsigned int ahrsz_K = 0;
unsigned int ahrsz_dKfb = 0;
unsigned int mnr_ninvalid = 0;
unsigned int mnr_ARxy = 0;
unsigned int mnr_ddfxy = 0;
unsigned int poto1_ninvalid = 0;
unsigned int poto1_dxy = 0;
unsigned int poto1_ddxy = 0;
unsigned int algo_count = 0;
unsigned int ol_ncreated = 0;
unsigned int ol_nrelabels = 0;
unsigned int ol2_ncreated = 0;
unsigned int ol2_nrelabels = 0;
unsigned int ol2_nrenumbers = 0;

bool verbose = false;

template<class T>
void my_print_graph(T &graph) {
  typedef typename T::out_edge_iterator oiterator;

  cout << "\tgraph = { ";
  typename T::vertex_iterator i,iend;
  for(tie(i,iend) = vertices(graph);i!=iend;++i) {
    oiterator j,jend;
    for(tie(j,jend) = out_edges(*i,graph);
	j!=jend;++j) {
      cout << source(*j,graph) << "->" << target(*j,graph) << " ";
    }    
  }
  cout << " }" << endl;
}

template<class S>
class n2i_comp {
private:
  typedef typename S::vertex_descriptor vd_t;
  typedef typename property_map<S, n2i_t>::type N2iMap;
  N2iMap _n2i;
public:
  n2i_comp(N2iMap n) : _n2i(n) {
  }

  bool operator()(vd_t const &v1, vd_t const &v2) {
    return _n2i[v1] < _n2i[v2];
  }
};

template<class T, class S>
void print_order(T &graph) {
  typedef typename T::vertex_descriptor vd_t;
  typedef typename property_map<S, n2i_t>::type N2iMap;
  N2iMap n2i = get(n2i_t(),graph);

  multiset<vd_t, n2i_comp<S> > ord(get(n2i_t(),graph));

  cout << "\torder = { ";
  typename T::vertex_iterator i,iend;
  for(tie(i,iend) = vertices(graph);i!=iend;++i) {
    ord.insert(*i);
  }

  vd_t last = -1;
  for(typename multiset<vd_t, n2i_comp<S> >::iterator i(ord.begin());
      i!=ord.end();++i) {
    if(last != -1 && !(n2i[last] < n2i[*i])) {
      cout << "," << *i;
    } else {
      cout << " " << *i;
    }
    last = *i;
  }
  cout << " }" << endl;
}

template<class T, class S>
bool check_solution(T &graph, string const &s) {
  typedef typename property_map<S, n2i_t>::type N2iMap;
  N2iMap n2i = get(n2i_t(),graph);

  typename T::vertex_iterator i,iend;
  for(tie(i,iend) = vertices(graph);i!=iend;++i) {
    typename T::vertex_iterator j(i),jend,jdum;
    ++j;
    for(tie(jdum,jend) = vertices(graph);j!=jend;++j) {
      bool f1(n2i[*i] < n2i[*j]);
      bool f2(n2i[*j] < n2i[*i]);
      if(f1 && path(*j,*i,graph)) {
	cerr << "Check failure because n2i[" << *i << "] < n2i[" << *j << "] AND path("; 
	cerr << *j << "," << *i << "). " << s << endl;
	my_print_graph(graph);
	print_order<T,S>(graph);
	return false;
      } else if(f2 && path(*i,*j,graph)) {
	cerr << "Check failure because n2i[" << *j << "] < n2i[" << *i << "] AND path("; 
	cerr << *i << "," << *j << "). " << s << endl;
	my_print_graph(graph);
	print_order<T,S>(graph);
      } else if(!f1 && !f2 && (path(*i,*j,graph) || path(*j,*i,graph))) {
	cerr << "Check failure because n2i[" << *j << "] == n2i[" << *i;
	cerr << "] AND there is at least one path connecting them. " << s << endl;
	my_print_graph(graph);
	print_order<T,S>(graph);
      }
    }
  }
  return true;
}

class work_visitor : public default_dfs_visitor {
protected:
  vector<set<unsigned int> > &_scores;
public:
  work_visitor(vector<set<unsigned int> > &scores) 
    : _scores(scores) {
  }

  template < typename Vertex, typename Graph >
  void finish_vertex(Vertex u, const Graph & g) const {
    // at this point we must compute our own score
    _scores[u].insert(u);
    typename Graph::out_edge_iterator i,iend;
    for(tie(i,iend)=out_edges(u,g);i!=iend;++i) {
      _scores[u].insert(_scores[target(*i,g)].begin(),_scores[target(*i,g)].end());
    }
  }
};

class exp_results {
public:
  double ARXY;
  double DKXY;
  double NCREATED;
  double NRELABELS;
  double INVAL;
  double ACPI;
  double COUNT;
  unsigned int errors;
public:
  exp_results() : NCREATED(0), NRELABELS(0),
		  INVAL(0), ACPI(0), errors(0), COUNT(0) {
  }
};

template<class T, class S>
void build_graph(unsigned int V, unsigned int E, unsigned int O, 
		 vector<pair<unsigned int, unsigned int> > &edges, T &graph,
		 istream &input) {
  unsigned int v = read_edgelist(input,edges);

  if(v != V) {
    cout << "v = " << v << ", V = " << V << endl;
    throw runtime_error("graphs in file have incorrect number of nodes.");
  } else if(O > E || E > edges.size()) {
    throw runtime_error("graphs in file have too few edges.");
  }
  // make sure we have the requested number of edges
  edges.resize(E);
  // create normal graph so we don't end up with a
  // valid topological order after this!
  S tmpg(V);
  // now, add them to the graph by taking from
  // the end of the edgelist
  while(E-- > O) {
    pair<unsigned int, unsigned int> &e(edges.back());    
    add_edge(e.first, e.second, tmpg);
    edges.pop_back();
  }
  graph = T(tmpg);
}
		 
template<class T, class S>
exp_results do_work(unsigned int V, unsigned int E, unsigned int O, unsigned int B, bool checking, istream &input) {
  vector<pair<unsigned int, unsigned int> > edges; // unused edges
  exp_results r;
  T graph(V); 

  // reset metrics
  mnr_ninvalid = poto1_ninvalid = ahrsz_ninvalid = 0;
  ol_nrelabels = ol2_nrelabels = ol2_nrenumbers = 0;
  mnr_ddfxy = poto1_ddxy = ahrsz_dKfb = 0;
  ol_ncreated = ol2_ncreated = 0;
  mnr_ARxy = 0;algo_count = 0;

  build_graph<T,S>(V,E,O,edges,graph,input);

  // because the graph was actually built twice
  ol_ncreated = ol_ncreated >> 1;
  ol2_ncreated = ol2_ncreated >> 1;

  if(checking && !check_solution<T,S>(graph,"Initial Graph")) {
    r.errors++;
  }
  
  // Ok, we start of by adding all of the edges so as to ensure that
  // the adjacency_list edge vectors are all correctly sized before
  // the experiment begins.  This way we eliminate some unnecessary
  // interference.

  for(vector<pair<unsigned int,unsigned int> >::iterator i(edges.begin());
      i != edges.end(); ++i) {
    // notice the cast. this bypasses the online topological order
    // to ensure that it is not being updated by this little trick!
    add_edge(i->first,i->second,(S&) graph);
  }
  
  for(vector<pair<unsigned int,unsigned int> >::iterator i(edges.begin());
      i != edges.end(); ++i) {
    remove_edge(i->first,i->second,(S&) graph);
  }
  
  // begin timing
  my_timer t; 

  vector<pair<unsigned int,unsigned int> >::iterator beg(edges.begin());
  vector<pair<unsigned int,unsigned int> >::iterator end;
  for(end=beg+B; beg != edges.end(); end += min<unsigned int>(B,edges.end()-end), beg += min<unsigned int>(B,edges.end()-beg)) {
    // add new edge batch
    add_edges(beg,end,graph);    

    if(checking && !check_solution<T,S>(graph,"BATCH")) {
      r.errors++;
    }
  }

  // accumulate metrics
  r.INVAL = ((double) (mnr_ninvalid + poto1_ninvalid + ahrsz_ninvalid)) / edges.size();
  r.ARXY = ((double) mnr_ARxy) / edges.size();
  r.DKXY = ((double) (mnr_ddfxy + poto1_ddxy + ahrsz_dKfb)) / edges.size();
  r.NCREATED = ol_ncreated + ol2_ncreated;
  r.NRELABELS = ol_nrelabels + ol2_nrelabels + ol2_nrenumbers;
  r.ACPI = t.elapsed() / edges.size();
  r.COUNT = algo_count / edges.size();  // avg op count per batch

  return r;
}

bool find_replace(string &str, string const &match, string const &replace) {
  unsigned int pos(0);
  string r;
  
  if((pos=str.find(match,pos)) < str.size()) {
    str.replace(pos,match.size(),replace);
    return true;  }
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
    {"checking",no_argument,NULL,OPT_CHECKING},
    {"nodes",required_argument,NULL,OPT_NODES},
    {"outdegree",required_argument,NULL,OPT_OUTDEGREE},
    {"MNR",no_argument,NULL,OPT_MNR},
    {"AHRSZ",no_argument,NULL,OPT_AHRSZ},
    {"POTO1",no_argument,NULL,OPT_POTO1},
    {"AHRSZb",no_argument,NULL,OPT_AHRSZB},
    {"DFS",no_argument,NULL,OPT_DFS},
    {"sample",required_argument,NULL,OPT_SAMPLE},
    {"batch",required_argument,NULL,OPT_BATCH},
    {"density",required_argument,NULL,OPT_DENSITY},
    {"DUMMY",no_argument,NULL,OPT_DUMMY},
    {"verbose",no_argument,NULL,OPT_VERBOSE},
    {"sample-fixed",required_argument,NULL,OPT_SAMPLEFIXED},
    {"num-graphs",required_argument,NULL,OPT_NGRAPHS},
    NULL
  };

  char *descriptions[]={
    " -h     --help                    display this information",
    " -v     --version                 display version information",
    "        --verbose                 show addition information",
    "        --checking                perform correctness checking",
    " -v<x>  --nodes=<x>               set value of N (default=50)",
    " -e<x>  --edges=<x>               set value of E",
    " -s<x>  --sample=<x>              sample x edges over all possible.",
    "        --sample-fixed=<x>        sample over x edges.",
    " -b<x>  --batch=<x>               process edges in x chunks. This is useful for",
    "                                  looking at the non-unit change algorithms",
    " -o<x>  --outdegree=<x>           set expected initial outdegree",
    "                                  determintes the probility p to use in the graph",
    "                                  generator and does not *guarantee* an exact outdegree",
    "                                  of x",
    "                                  as a percentage. (default=50)",
    " -n<x>  --num-graphs=<x>          Iterate over x graphs.",
    " -d<x>  --density=<x>             set the ratio of expected actual edges versus the maximum",
    "                                  number of possible edges.",
    "        --MNR                     use online topological sorting algorithm by",
    "        --POTO1                   use algorithm POTO1 from PhD thesis of David J. Pearce",
    "        --AHRSZ                   Use algorithm by Alpern et al.  This implementation uses",
    "                                  an O(1) amortized time priority space data structure",
    "        --AHRSZb                  Use algorithm by Alpern et al.  This implementation uses",
    "                                  an O(log n) time priority space data structure",
    "        --DFS                     Use (offline) depth-first search algorithm",
    "        --DUMMY                   This does not implement an online topological order.  Instead,",
    "                                  it is useful for estimating the cost of simply adding edges",
    "                                  to the underlying graph type.",
    "",
    "Notes: time is measured using gettimeofday.",
    NULL
  };

  double over(0.0005);
  double density=0,outdegree=0;
  string infile("graph-%V-%E-%N.dat");
  unsigned int algorithm(OPT_POTO1);
  unsigned int over_fixed = 0;
  unsigned int ngraphs = 1;
  unsigned int precision = 5;
  bool rep_ACPI = true;
  bool rep_ARXY = false;
  bool rep_DKXY = false;
  bool rep_INVAL = false;
  bool rep_NCREATED = false;
  bool rep_NRELABELS = false;
  bool rep_COUNT = false;
  bool checking = false;
  range<unsigned int> Vr;
  range<double> Er;
  range<unsigned int> Br(1);
  int conversion = OPT_EDGES;
  
  try {    

    // parse command line arguments
    char v;
    while((v=getopt_long(argc,argv,"b:hv:d:o:n:i:s:f:e:",long_options,NULL)) != -1) {
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
	  cout << argv[0] << " v" << MAJOR_VERSION << "." << MINOR_VERSION << endl;
	  cout << "\tWritten by David J. Pearce, Feburary 2003" << endl;
	  exit(1);
	case OPT_CHECKING:
	  rep_ACPI =  false;
	  checking = true;
	  break;
	case OPT_VERBOSE:
	  verbose = true;
	  break;
	case 'v':
	case OPT_NODES:
	  Vr = range<unsigned int>(optarg);
	  break;
	case 'd':
	case OPT_DENSITY:
	  Er = range<double>(optarg);	  
	  conversion = OPT_DENSITY;
	  break;     
	case 'o':
	case OPT_OUTDEGREE:
	  Er = range<double>(optarg);	  
	  conversion = OPT_OUTDEGREE;
	  break;
	case 'e':
	case OPT_EDGES:
	  Er = range<double>(optarg);	  
	  conversion = OPT_EDGES;
	  break;
	case 'b':
	case OPT_BATCH:
	  Br = range<unsigned int>(optarg);
	  break;
	case 's':
	case OPT_SAMPLE:
	  over = strtod(optarg,NULL);
	  break;
	case OPT_SAMPLEFIXED:
	  over_fixed = atoi(optarg);
	  break;
	case 'n':
	case OPT_NGRAPHS:
	  ngraphs = atoi(optarg);
	  break;
	case 'f':
	case OPT_INPUT:
	  infile = string(optarg);
	  break;	
	  
	  /* === ALGORITHMS === */
	  
	case OPT_DUMMY:
	  algorithm=OPT_DUMMY;
	  break;
	case OPT_MNR:	  
	  algorithm=OPT_MNR;
	  rep_ARXY = true;
	  rep_DKXY = true;
	  rep_INVAL = true;
	  rep_COUNT = true;
	  break;
	case OPT_POTO1:
	  algorithm=OPT_POTO1;
	  rep_DKXY = true;
	  rep_INVAL = true;
	  break;
	case OPT_AHRSZ:
	  algorithm=OPT_AHRSZ;
	  rep_DKXY = true;
	  rep_INVAL = true;
	  rep_NCREATED = true;
	  rep_NRELABELS = true;
	  break;
	case OPT_AHRSZB:
	  algorithm=OPT_AHRSZB;
	  rep_DKXY = true;
	  rep_INVAL = true;
	  rep_NCREATED = true;
	  rep_NRELABELS = true;
	  break;
	case OPT_DFS:
	  algorithm=OPT_DFS;
	  rep_COUNT = true;
	  break;
	  
	default:
	  // getopt will print invalid argument for us
	  exit(1);
	}
    }

    // Print out file headers
    time_t tm = time(NULL);    
    char hostname[128]="unknown";
    gethostname(hostname,128);
    cout << "# TIMESTAMP: " << ctime(&tm);
    cout << "# VERSION: oto_test v" << MAJOR_VERSION << "." << MINOR_VERSION << endl;
    cout << "# HOST: " << hostname << endl;
    if(over_fixed != 0) {
      cout << "# SAMPLE SIZE: " << over_fixed << "(FIXED)" << endl;
    } else {
      cout << "# SAMPLE SIZE: " << over << endl;
    }
    cout << "# NGRAPHS: " << ngraphs << endl;
    switch(algorithm) {
    case OPT_MNR:
      cout << "# ALGORITHM: MNR " << endl;
      break;
    case OPT_POTO1:
      cout << "# ALGORITHM: POTO1 " << endl;
      break;
    case OPT_AHRSZB:
      cout << "# ALGORITHM: AHRSZb " << endl;
      break;
    case OPT_AHRSZ:
      cout << "# ALGORITHM: AHRSZ " << endl;
      break;
    case OPT_DUMMY:
      cout << "# ALGORITHM: CTRL" << endl;
      break;
    case OPT_DFS:
      cout << "# ALGORITHM: SOTO" << endl;
      break;     
    }
    
    cout << "#" << endl;

    switch(conversion) {
    case OPT_EDGES:
      cout << "# V\tE\tB\t";
      break;
    case OPT_OUTDEGREE:
      cout << "# V\tOD\tB\t";
      break;
    case OPT_DENSITY:
      cout << "# V\tD\tB\t";
      break;
    }

    if(rep_ACPI) { cout << "ACPI   \t"; }
    if(rep_ARXY) { cout << "ARxy   \t"; }
    if(rep_DKXY) { 
      switch(algorithm) {
      case OPT_MNR:
	cout << "|>dxy|  \t"; 
	break;
      case OPT_POTO1:
	cout << "|>dxy<| \t"; 
	break;
      case OPT_AHRSZB:
      case OPT_AHRSZ:
	cout << "|>K<|   \t"; 
	break;
      }
    }
    if(rep_INVAL) {cout << "INVAL\t"; }
    if(rep_NCREATED) { cout << "NCREATED\t"; }
    if(rep_NRELABELS) { cout << "NRELABELS\t"; }
    if(rep_COUNT) { cout << "COUNT\t"; }
    if(checking) { cout << "ERRORS"; }
    cout << endl;

    // Now, do the experiments!

    do {
      unsigned int B = Br.value();
      do {
	unsigned int V = Vr.value();
	do {
	  unsigned int E = range2nedges(Er,V,conversion);	
	  // create filename
	  string tmp(infile);
	  string El("e");
	  if(conversion == OPT_DENSITY) { El = "d"; }
	  else if(conversion == OPT_DENSITY) { El = "o"; }
	  while(find_replace(tmp,"%V",string("v") + lexical_cast<string>(V)));
	  while(find_replace(tmp,"%E",El + double2str(Er.value(),precision)));
	  while(find_replace(tmp,"%N",string("n") + lexical_cast<string>(ngraphs)));
	  ifstream input(tmp.c_str());
	  if(verbose) { cerr << "INPUT FILE: " << tmp.c_str() << endl; }
	  
	  // the point of this bit of computation
	  // is to make sure that the number of edges
	  // we're experimenting over is a multiple
	  // of the batch size.
	  unsigned int sample;
	  if(over_fixed != 0) { sample = over_fixed;
	  } else {
	    sample = (unsigned int) (over * 0.5 * V *(V-1));
	  }       
	  
	  unsigned int O = sample;
	  
	  if(B > sample) { B = O; } 
	  
	  if(verbose) {
	    cerr << "Experiment: V = " << V << ", E = " << E << ", S = ";
	    cerr << O << ", B = " << B << " , NGRAPHS = " << ngraphs << endl;	    
	  }   
	  
	  average ARXY,DKXY,ACPI,INVAL,NCREATED,NRELABELS,COUNT;
	  unsigned int errors(0);

	  for(int i=0;i!=ngraphs;++i) {
	    exp_results r;
	    
	    switch(algorithm) {	
	    case OPT_POTO1:
	      r = do_work<POTO1_graph_t,subgraph1_t>(V,E,O,B,checking,input);
	      break;
	    case OPT_MNR:
	      r = do_work<MNR_graph_t,subgraph1_t>(V,E,O,B,checking,input);
	      break;
	    case OPT_AHRSZ:
	      r = do_work<AHRSZ_graph_t,subgraph3_t>(V,E,O,B,checking,input);
	      break;
	    case OPT_AHRSZB:
	      r = do_work<AHRSZb_graph_t,subgraph2_t>(V,E,O,B,checking,input);
	      break;
	    case OPT_DFS:
	      r = do_work<DFS_graph_t,subgraph1_t>(V,E,O,B,checking,input);
	      break;
	    case OPT_DUMMY:
	      r = do_work<dummy_graph_t,subgraph1_t>(V,E,O,B,checking,input);
	      break;
	    default:
	      cerr << "unknown algorithm" << endl;
	      exit(1);
	    }
	    ARXY += r.ARXY;
	    DKXY += r.DKXY;
	    NCREATED += r.NCREATED;
	    NRELABELS += r.NRELABELS;
	    ACPI += r.ACPI;
	    INVAL += r.INVAL;
	    errors += r.errors;
	    COUNT += r.COUNT;
	  }
	  
	  // report final results
	  cout << V << "\t";
	  cout << Er.value() << "\t";
	  cout << B << "\t";
	  if(rep_ACPI) { cout << ACPI.value() << "\t"; }
	  if(rep_ARXY) { cout << ARXY.value() << "\t"; }
	  if(rep_DKXY) { cout << DKXY.value() << "\t"; }
	  if(rep_INVAL) {cout << INVAL.value() << "\t"; }
	  if(rep_NCREATED) { cout << NCREATED.value()<< "\t"; }
	  if(rep_NRELABELS) { cout << NRELABELS.value()<< "\t"; }
	  if(rep_COUNT) { cout << COUNT.value() << "\t"; }
	  if(checking) { cout << errors << "\t"; }
	  cout << endl;
	} while(!Er.step());
      } while(!Vr.step()); 
    } while(!Br.step());   
  } catch(bad_alloc e) {
    cerr << "Out of Memory (" << e.what() << ")" << endl;
  } catch(runtime_error &e) {
    cerr << "Internal failure - " << e.what() << endl;
    exit(1);
  } catch(exception &e) {
    cerr << "Internal failure - " << e.what() << endl;
    exit(1);
  } catch (...) {
    cerr << "Internal failure" << endl;
    exit(1);
  }
  
  exit(0);
}








