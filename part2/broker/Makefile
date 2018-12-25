CXX=g++
CXXFLAGS=-std=c++17 -pthread -Wall -Wextra -O2
SRCS=broker.cpp
OBJS=$(patsubst %.cpp, %.o, $(SRCS))
HDRS=util.hpp
TARGET=broker.out

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

$(OBJS) : $(SRCS) $(HDRS)

clean:
	rm -f *.o *.out
