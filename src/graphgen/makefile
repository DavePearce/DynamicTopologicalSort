all: graphgen printgraph

BINDIR=$(wildcard ~/pkg/bin)
CXXFLAGS = -O3 -DNDEBUG

graphgen: graphgen.cpp
	g++ $(CXXFLAGS) -ftemplate-depth-32 -o graphgen graphgen.cpp

printgraph: printgraph.cpp
	g++ $(CXXFLAGS) -ftemplate-depth-32 -o printgraph printgraph.cpp

install: graphgen printgraph
	install -m 0777 graphgen $(BINDIR)/graphgen
	install -m 0777 printgraph $(BINDIR)/printgraph

clean: 
	rm -r *~ graphgen printgraph
