OBJS = simulation.o
CXX = g++
DEBUG = -O3
CXXFLAGS = -pthread $(DEBUG)
LDFLAGS = -pthread $(DEBUG)

simulation: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
 