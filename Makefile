OBJS = simulation.o
CXX = g++
DEBUG = -g
CXXFLAGS = -pthread $(DEBUG)
LDFLAGS = -pthread $(DEBUG)

simulation: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
 