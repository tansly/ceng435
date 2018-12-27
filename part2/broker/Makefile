CXX=g++
CXXFLAGS=-std=c++17 -pthread -Wall -Wextra -O0 -ggdb
LDFLAGS=$(shell pkg-config openssl --libs)
SRCS=broker.cpp
OBJS=$(patsubst %.cpp, %.o, $(SRCS))
HDRS=util.hpp
TARGET=broker.out

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

$(OBJS) : $(SRCS) $(HDRS)

clean:
	rm -f *.o *.out
