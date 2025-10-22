# Компилятор
CC = gcc

# Флаги компиляции
CFLAGS = -Wall -std=c99

# Флаги линковки для Linux
LDFLAGS = -lraylib -lm -lpthread -ldl

# Имя выходного файла
TARGET = mygame

# Исходные файлы
SRC = main.c

# Правило по умолчанию
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Очистка
clean:
	rm -f $(TARGET)

# Пересборка
rebuild: clean all

.PHONY: all clean rebuild
