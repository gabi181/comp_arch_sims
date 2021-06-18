SRCS := cacheSim.cpp
OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean

all: cacheSim

cacheSim: $(OBJS)
	$(CXX) -o $@ $^

cacheSim.o: cache.h

clean:
	$(RM) prog $(OBJS)