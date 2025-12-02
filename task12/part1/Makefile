cc := g++
cflags := -lglfw -L/usr/lib64 -lGLEW -lGL -lX11 -lGLU 
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