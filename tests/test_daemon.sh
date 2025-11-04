#!/bin/bash

# Nombre del ejecutable
DAEMON_PROG="./process_daemon"
LOG_FILE="/tmp/daemon.log"
MONITOR_TIME=30 # Tiempo en segundos para monitorear el demonio

echo "--- Test 4: Long-Running Daemon Verification ---"

# 1. Verificar si el programa existe
if [ ! -x "$DAEMON_PROG" ]; then
    echo "ERROR: Asegúrate de que $DAEMON_PROG esté compilado y sea ejecutable."
    exit 1
fi

# Limpiar el log anterior
rm -f $LOG_FILE

echo "1. Iniciando el demonio en segundo plano..."
$DAEMON_PROG

# Dar un momento para que el proceso padre termine y el demonio hijo se establezca
sleep 2

# Encontrar el PID del proceso demonio (el proceso que permanece)
DAEMON_PID=$(ps aux | grep "$DAEMON_PROG" | grep -v grep | awk '{print $2}' | head -n 1)

if [ -z "$DAEMON_PID" ]; then
    echo "ERROR: No se pudo encontrar el PID del proceso demonio."
    exit 1
fi

echo "  - Demonio iniciado. PID: $DAEMON_PID"
echo "  - Monitorizando durante $MONITOR_TIME segundos para detectar zombies..."

ZOMBIES_DETECTED=0

# 2. Bucle de monitoreo
# Cada 5 segundos, verificamos si existe algún zombie 'defunct' cuyo PPID sea el demonio.
for ((i=0; i<$MONITOR_TIME; i+=5)); do
    sleep 5
    
    # Buscar zombies cuyo padre (PPID) sea el PID del demonio
    # ps -o ppid,stat,cmd -ax: Muestra PPID, estado y comando de todos los procesos.
    ZOMBIE_COUNT=$(ps -o ppid,stat -ax | awk -v pid="$DAEMON_PID" '$1 == pid && $2 == "Z"' | wc -l)
    
    if [ "$ZOMBIE_COUNT" -gt 0 ]; then
        echo "  [FAILURE] ¡Zombies detectados! El demonio PID $DAEMON_PID tiene $ZOMBIE_COUNT hijos zombie."
        ZOMBIES_DETECTED=$ZOMBIE_COUNT
        break # Falla inmediatamente
    fi
    
    echo "  - Tiempo: $i segundos. No hay zombies (ok)."
done

# 3. Evaluación del resultado
echo ""
if [ "$ZOMBIES_DETECTED" -eq 0 ]; then
    echo "  [GLOBAL SUCCESS] El demonio funcionó durante $MONITOR_TIME segundos sin crear procesos zombie."
    
    # Muestra las últimas 10 líneas del log para ver la actividad
    echo "--- Últimas líneas del Log ($LOG_FILE) ---"
    tail $LOG_FILE
    echo "----------------------------------------"
    
    PASSED=true
else
    echo "  [GLOBAL FAILURE] El demonio creó $ZOMBIES_DETECTED zombies. El reaprocesamiento falló."
    PASSED=false
fi

# 4. Limpieza: Detener el demonio
echo "4. Deteniendo el demonio ($DAEMON_PID) con SIGTERM (apagado ordenado)..."
kill $DAEMON_PID

# Esperar la terminación
sleep 2

# Verificación final de que el demonio ya no esté
DAEMON_RUNNING=$(ps aux | grep "$DAEMON_PROG" | grep -v grep | wc -l)

if [ "$DAEMON_RUNNING" -eq 0 ]; then
    echo "  [LIMPIEZA ÉXITO] El proceso demonio ha terminado correctamente."
else
    echo "  [LIMPIEZA FALLO] El proceso demonio sigue corriendo."
    kill -9 $DAEMON_PID 2>/dev/null # Intento de limpieza forzada
fi

echo "--- Test 4: Finalizado ---"

if [ "$PASSED" = true ]; then
    exit 0
else
    exit 1
fi
