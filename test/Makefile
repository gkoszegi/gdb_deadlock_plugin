all: test

CXXFLAGS = -g3 -O0 -std=c++11 -pthread -Wall

#%: %.cpp
#	$(CXX) $(CXXFLAGS) -o $@ $<

test: deadlock
	gdb --batch -x deadlock.gdb ./deadlock
