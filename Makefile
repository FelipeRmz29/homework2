# ===============================================
# Configuración
# ===============================================
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -pthread
# -g incluye símbolos de debug.
# -pthread es necesario para la librería zombie.c (mutexes).
LDFLAGS = -L./src -lzombie -pthread
# -L./src indica dónde buscar la librería.
# -lzombie enlaza la librería libzombie.a.

# Archivos fuente y ejecutables
SRCS = src/zombie_creator.c src/zombie_detector.c src/zombie_reaper.c src/process_daemon.c
EXECS = zombie_creator zombie_detector zombie_reaper process_daemon
LIB_SRCS = src/zombie.c src/zombie.h
LIB_TARGET = libzombie.a
TEST_PROG = tests/test_lib.c
TEST_EXEC = test_lib

# ===============================================
# Regla principal (all)
# ===============================================
all: $(EXECS) $(LIB_TARGET) $(TEST_EXEC)

# ===============================================
# Reglas de Programas Ejecutables
# ===============================================

# Parte 1
zombie_creator: src/zombie_creator.c
	$(CC) $(CFLAGS) $< -o $@

# Parte 2
zombie_detector: src/zombie_detector.c
	$(CC) $(CFLAGS) $< -o $@

# Parte 3
zombie_reaper: src/zombie_reaper.c
	$(CC) $(CFLAGS) $< -o $@

# Parte 4
process_daemon: src/process_daemon.c
	$(CC) $(CFLAGS) $< -o $@

# ===============================================
# Reglas de Librería y Ejemplo
# ===============================================

# Compila el archivo objeto de la librería
src/zombie.o: src/zombie.c src/zombie.h
	$(CC) $(CFLAGS) -c src/zombie.c -o src/zombie.o

# Crea la librería estática
$(LIB_TARGET): src/zombie.o
	ar rcs $@ src/zombie.o

# Compila el programa de prueba usando la librería estática
$(TEST_EXEC): $(TEST_PROG) $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ===============================================
# Reglas de Pruebas
# ===============================================
.PHONY: test_all test_creator test_detector test_reaper test_daemon test_lib

test_all: test_creator test_detector test_reaper test_daemon test_lib

test_creator: zombie_creator zombie_detector
	@echo "--- Ejecutando test_creator.sh ---"
	./tests/test_creator.sh

test_detector: zombie_creator zombie_detector
	@echo "--- Ejecutando test_detector.sh ---"
	./tests/test_detector.sh

test_reaper: zombie_reaper
	@echo "--- Ejecutando test_reaper.sh ---"
	./tests/test_reaper.sh

test_daemon: process_daemon
	@echo "--- Ejecutando test_daemon.sh ---"
	./tests/test_daemon.sh

test_lib: $(TEST_EXEC)
	@echo "--- Ejecutando test_lib (Librería) ---"
	./$(TEST_EXEC)

# ===============================================
# Regla de Limpieza
# ===============================================

clean:
	@echo "Limpiando ejecutables, objetos y archivos temporales..."
	rm -f $(EXECS) $(TEST_EXEC) $(LIB_TARGET)
	rm -f src/*.o
	rm -f /tmp/daemon.log

# Reglas Phony
.PHONY: all clean
