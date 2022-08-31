OBJS = simulation.o
CXX = g++
DEBUG = -g
CXXFLAGS = -pthread
LDFLAGS = -pthread

simulation: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
 