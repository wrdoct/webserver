CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g
TARGET = server
OBJS = ../log/*.cpp ../http/*.cpp ../buffer/*.cpp ../server/*.cpp ../main.cpp

all : $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET) -pthread

clean:
	rm -rf ../bin/$(TARGET)