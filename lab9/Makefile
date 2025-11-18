CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
TARGET = main
SRCS = src/main.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
