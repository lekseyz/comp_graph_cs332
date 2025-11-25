cc := g++
cflags := -Iinclude -Wall -O3
lflags := -lraylib -lm -ldl -lpthread -lrt -lX11
workdir := src
builddir := build

all: $(builddir)/main

run: $(builddir)/main
	$(builddir)/main ./langs/spiral.txt

$(builddir)/main: $(workdir)/main.cpp $(builddir)
	$(cc) $(workdir)/main.cpp -o $(builddir)/main $(cflags) $(lflags)

$(builddir):
	mkdir -p $(builddir)

clear:
	rm -rf $(builddir)