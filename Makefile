OBJS = simulation.o
CXX = g++
DEBUG = -O3
CPPFLAGS = -pthread $(DEBUG)
LDFLAGS = -pthread $(DEBUG)

simulation: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean: 
	rm $(OBJS) simulation