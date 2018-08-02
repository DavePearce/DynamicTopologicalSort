BINDIR=$(wildcard ~/pkg/bin)
CXX_FLAGS = -O3 -fomit-frame-pointer -DNDEBUG

oto_test: oto_test.cpp \
	ahrsz_online_topological_order.hpp \
	poto1_online_topological_order.hpp \
	mnr_online_topological_order.hpp \
	dummy_online_topological_order.hpp 
	g++ $(CXX_FLAGS) -o oto_test oto_test.cpp

ordered_slist_test: ordered_slist_test.cpp
	g++ $(CXX_FLAGS) -o ordered_slist_test ordered_slist_test.cpp

install: oto_test
	install -m 0777 oto_test $(BINDIR)/oto_test

clean: 
	rm -r *~ oto_test ordered_slist_test
