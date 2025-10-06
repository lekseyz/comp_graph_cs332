# Компилятор и пути
CC = gcc
CFLAGS = -Iinclude -IC:/raylib/raylib/src -Wall -std=c99
WORKDIR = src
BUILDDIR = build
LFLAGS = -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm

# Основные цели
all: build-windows

build-windows: $(BUILDDIR)/main.exe

run-windows: $(BUILDDIR)/main.exe
	$(BUILDDIR)/main.exe

# Сборка под Windows
$(BUILDDIR)/main.exe: $(WORKDIR)/main.c | $(BUILDDIR)
	$(CC) $(WORKDIR)/main.c -o $(BUILDDIR)/main.exe $(CFLAGS) $(LFLAGS)

# Создание build каталога
$(BUILDDIR):
	if not exist "$(BUILDDIR)" mkdir "$(BUILDDIR)"

# Очистка
clean:
	if exist "$(BUILDDIR)" rmdir /s /q "$(BUILDDIR)"

.PHONY: all build-windows run-windows clean