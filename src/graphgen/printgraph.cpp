#include "graphgen.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  int V;
  std::vector<pair<unsigned int, unsigned int> > edgelist;
  V = read_edgelist(cin,edgelist);
  cout << "V = " << V <<  endl;
  cout << "E = (" << edgelist.size() << ") { ";
  for(int i=0;i!=edgelist.size();i++) {
    if(i!=0) { cout << ", "; }
    cout << edgelist[i].first << "->" << edgelist[i].second;
  }
  cout << " }" << endl;
  exit(0);
}
