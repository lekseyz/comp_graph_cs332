cc := g++
cflags := -Iinclude -Wall -O3 -std=c++17
lflags := -lraylib -lm -ldl -lpthread -lrt -lX11
workdir := src
builddir := build
sources := $(wildcard $(workdir)/*.cpp)
objects := $(patsubst $(workdir)/%.cpp, $(builddir)/%.o, $(sources))

all: $(builddir)/game

run: $(builddir)/game
	./$(builddir)/game

$(builddir)/game: $(objects)
	$(cc) $(objects) -o $@ $(lflags)

$(builddir)/%.o: $(workdir)/%.cpp | $(builddir)
	$(cc) -c $< -o $@ $(cflags)

$(builddir):
	mkdir -p $(builddir)

clear:
	rm -rf $(builddir)