#!/bin/bash

# Nombres de los ejecutables
CREATOR_PROG="./zombie_creator"
DETECTOR_PROG="./zombie_detector"
NUM_ZOMBIES=7 # Número de zombies a crear para la prueba

echo "--- Test 2: Detect Zombie Processes ---"

# 1. Verificar si los programas existen
if [ ! -x "$CREATOR_PROG" ] || [ ! -x "$DETECTOR_PROG" ]; then
    echo "ERROR: Asegúrate de que $CREATOR_PROG y $DETECTOR_PROG estén compilados y sean ejecutables."
    exit 1
fi

echo "1. Creando $NUM_ZOMBIES procesos zombie en segundo plano..."

# Ejecutar zombie_creator en segundo plano.
# Redirigimos stdin para evitar que espere la tecla ENTER del script.
$CREATOR_PROG $NUM_ZOMBIES < /dev/null & 
CREATOR_PID=$!

if [ -z "$CREATOR_PID" ]; then
    echo "ERROR: Fallo al iniciar $CREATOR_PROG."
    exit 1
fi

echo "  - Proceso Padre (Zombie Creator) PID: $CREATOR_PID"

# Esperar un momento para asegurar que los hijos han terminado y son zombies
sleep 2

# 2. Ejecutar zombie_detector
echo "2. Ejecutando Zombie Detector ($DETECTOR_PROG) para escanear el sistema..."

# Capturar la salida completa del detector
DETECTOR_OUTPUT=$($DETECTOR_PROG)

# Imprimir el reporte para la revisión manual
echo "----------------------------------------"
echo "$DETECTOR_OUTPUT"
echo "----------------------------------------"

# 3. Analizar la salida del detector
# Intentar extraer la línea 'Total Zombies: X' y el PPID del creador
REPORTED_ZOMBIES=$(echo "$DETECTOR_OUTPUT" | grep 'Total Zombies:' | awk '{print $3}')
REPORTED_PPID=$(echo "$DETECTOR_OUTPUT" | grep "PID $CREATOR_PID" | awk '{print $2}')

echo "3. Verificando resultados del reporte..."
echo "  - Zombies reportados por el detector: $REPORTED_ZOMBIES"

# Comprobación 1: La cantidad reportada debe ser igual o mayor a los creados
# (Podrían existir otros zombies en el sistema).
if [ "$REPORTED_ZOMBIES" -ge "$NUM_ZOMBIES" ]; then
    echo "  [ÉXITO PARCIAL] El detector reporta $REPORTED_ZOMBIES zombies (>= $NUM_ZOMBIES)."
else
    echo "  [FALLO] El detector solo reporta $REPORTED_ZOMBIES zombies. Se esperaban al menos $NUM_ZOMBIES."
fi

# Comprobación 2: El análisis de padres debe identificar al creador como el PPID
if echo "$DETECTOR_OUTPUT" | grep -q "PID $CREATOR_PID.*has $NUM_ZOMBIES zombie children"; then
    echo "  [ÉXITO] El análisis de padres identifica correctamente al PID $CREATOR_PID con $NUM_ZOMBIES hijos zombie."
else
    echo "  [FALLO] El análisis de padres no pudo verificar al creador PID $CREATOR_PID con $NUM_ZOMBIES zombies."
fi


# 4. Limpieza: Matar al proceso padre (zombie_creator)
echo "4. Limpiando: Enviando SIGTERM al Proceso Padre PID $CREATOR_PID..."
kill $CREATOR_PID

# Verificación final de limpieza
sleep 1
ZOMBIE_COUNT_FINAL=$(ps -o ppid,state -ax | awk -v pid="$CREATOR_PID" '$1 == pid && $2 == "Z"' | wc -l)

if [ "$ZOMBIE_COUNT_FINAL" -eq 0 ]; then
    echo "  [ÉXITO] Todos los zombies han sido limpiados."
else
    echo "  [ADVERTENCIA] No se pudieron limpiar todos los zombies."
fi

echo "--- Test 2: Finalizado ---"
