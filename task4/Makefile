cc := gcc
cflags := -Iinclude -Wall
lflags := -lraylib -lm -ldl -lpthread -lrt -lX11
workdir := src
builddir := build

run: $(builddir)/main
	$(builddir)/main

$(builddir)/main: $(workdir)/main.c $(builddir)
	$(cc) $(workdir)/main.c -o $(builddir)/main $(cflags) $(lflags)

$(builddir):
	mkdir -p $(builddir)

clear:
	rm -rf $(builddir)