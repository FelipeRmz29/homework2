#!/bin/bash

# Nombre del ejecutable
REAPER_PROG="./zombie_reaper"

echo "--- Test 3: Zombie Reaper Strategies Verification ---"

# 1. Verificar si el programa existe
if [ ! -x "$REAPER_PROG" ]; then
    echo "ERROR: Asegúrate de que $REAPER_PROG esté compilado y sea ejecutable."
    exit 1
fi

# Lista de estrategias a probar
STRATEGIES=(1 2 3) # 1: Explicit Wait, 2: SIGCHLD Handler, 3: SIG_IGN

TOTAL_TESTS=0
PASSED_TESTS=0

# Bucle para probar cada estrategia
for strategy in "${STRATEGIES[@]}"; do
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo ""
    echo "=========================================================="
    
    case $strategy in
        1) DESC="Strategy 1: Explicit Wait (waitpid)";;
        2) DESC="Strategy 2: SIGCHLD Handler (sigaction)";;
        3) DESC="Strategy 3: Ignore SIGCHLD (SIG_IGN)";;
        *) DESC="Unknown Strategy";;
    esac
    
    echo "Starting Test for $DESC (strategy code $strategy)"
    echo "=========================================================="
    
    # 2. Ejecutar el reaper con la estrategia actual
    # La salida incluye la creación de 10 hijos, el reaprocesamiento y la verificación
    
    # Ejecutamos el reaper. Capturamos el PID del proceso padre (el reaper en sí).
    $REAPER_PROG $strategy

    # El programa zombie_reaper ya tiene una verificación interna (ps aux | grep defunct).
    # Hacemos una verificación externa final para ser exhaustivos.
    
    sleep 1 # Dar un momento para que el kernel actualice los estados

    # 3. Verificación externa: Contar procesos "defunct" restantes
    # Filtramos por procesos defunct (Z) y excluimos la línea del propio grep
    ZOMBIE_REMAINING=$(ps aux | grep 'defunct' | grep -v grep | wc -l)
    
    echo ""
    echo "--- External Verification ---"
    
    if [ "$ZOMBIE_REMAINING" -eq 0 ]; then
        echo "  [SUCCESS] $DESC: NO zombies remain after reaping."
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo "  [FAILURE] $DESC: $ZOMBIE_REMAINING zombies remain after reaping!"
        # Mostrar los zombies que quedan
        ps aux | grep 'defunct' | grep -v grep
    fi
    
done

echo ""
echo "=========================================================="
echo "--- Summary of Zombie Reaper Tests ---"
echo "Tests Run: $TOTAL_TESTS"
echo "Tests Passed: $PASSED_TESTS"
echo "=========================================================="

if [ "$PASSED_TESTS" -eq "$TOTAL_TESTS" ]; then
    echo "[GLOBAL SUCCESS] Todas las estrategias de cosecha funcionan correctamente."
    exit 0
else
    echo "[GLOBAL FAILURE] Falló la verificación de una o más estrategias de cosecha."
    exit 1
fi
