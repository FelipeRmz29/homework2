#!/bin/bash

# Nombre del ejecutable
CREATOR_PROG="./zombie_creator"
DETECTOR_PROG="./zombie_detector"
NUM_ZOMBIES=10

echo "--- Test 1: Create and Verify Zombies ---"
echo "1. Creando $NUM_ZOMBIES procesos zombie en segundo plano..."

# 1. Ejecutar zombie_creator en segundo plano
# La salida de "Press Enter to exit..." puede interferir, por eso redirigimos stdin y stdout
# Ejecutamos con & para que el padre siga vivo en el fondo.
$CREATOR_PROG $NUM_ZOMBIES < /dev/null & 
CREATOR_PID=$! # Captura el PID del proceso padre (zombie_creator)

if [ -z "$CREATOR_PID" ]; then
    echo "ERROR: Fallo al iniciar $CREATOR_PROG."
    exit 1
fi

echo "  - Proceso Padre (Zombie Creator) PID: $CREATOR_PID"

# Esperar un momento para asegurar que todos los hijos han terminado y son zombies
sleep 2

echo "2. Verificando la existencia de los zombies con ps aux y grep 'Z'..."

# Contar cuántos zombies están asociados al proceso padre (solo por la PID del padre)
ZOMBIE_COUNT=$(ps -o ppid,state -ax | awk -v pid="$CREATOR_PID" '$1 == pid && $2 == "Z"' | wc -l)

echo "  - Zombies encontrados (ps aux): $ZOMBIE_COUNT / $NUM_ZOMBIES"

if [ "$ZOMBIE_COUNT" -eq "$NUM_ZOMBIES" ]; then
    echo "  [ÉXITO] Se encontraron $NUM_ZOMBIES zombies asociados al PID del creador."
else
    echo "  [FALLO] Se esperaba $NUM_ZOMBIES zombies, pero se encontraron $ZOMBIE_COUNT."
    # Mostrar la lista de zombies (solo los que están asociados al PID del creador)
    ps -o pid,ppid,stat,cmd -ax | awk -v pid="$CREATOR_PID" '$2 == pid && $3 == "Z"'
    # exit 1 # No salimos inmediatamente para intentar limpiar
fi

# 3. Verificación usando zombie_detector (si existe)
if [ -x "$DETECTOR_PROG" ]; then
    echo "3. Ejecutando Zombie Detector para generar un reporte..."
    $DETECTOR_PROG
else
    echo "3. Omitiendo Zombie Detector: $DETECTOR_PROG no encontrado o no es ejecutable."
fi

# 4. Limpieza: Matar al proceso padre (zombie_creator)
echo "4. Limpiando: Enviando SIGTERM al Proceso Padre PID $CREATOR_PID para limpiar los zombies..."
kill $CREATOR_PID

# Esperar que el padre termine y que init (PID 1) coseche a los zombies huérfanos.
sleep 1

# 5. Verificación final: No debería quedar nada.
ZOMBIE_COUNT_FINAL=$(ps -o ppid,state -ax | awk -v pid="$CREATOR_PID" '$1 == pid && $2 == "Z"' | wc -l)

if [ "$ZOMBIE_COUNT_FINAL" -eq 0 ]; then
    echo "  [ÉXITO] Todos los zombies han sido limpiados."
else
    echo "  [ADVERTENCIA] Todavía quedan $ZOMBIE_COUNT_FINAL zombies."
fi

echo "--- Test 1: Finalizado ---"
