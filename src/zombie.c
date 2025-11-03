#include "zombie.h"
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

// Variables y Mutex para estadísticas compartidas (thread-safe)
static zombie_stats_t stats = {0, 0, 0};
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- Signal Handler para la cosecha automática ---

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    int reaped_count = 0;

    // Bloquear para asegurar que waitpid() y la actualización de estadísticas sean atómicas
    pthread_mutex_lock(&stats_mutex);

    // Bucle para cosechar a todos los hijos terminados (previene race conditions)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        reaped_count++;
        // No usar printf/fprintf aquí. El registro debe hacerse de manera segura.
    }
    
    // Actualizar estadísticas
    stats.zombies_reaped += reaped_count;
    stats.zombies_active = stats.zombies_created - stats.zombies_reaped;

    pthread_mutex_unlock(&stats_mutex);
}

// --- API de la Librería ---

void zombie_init(void) {
    struct sigaction sa;

    // 1. Inicializar Mutex (aunque PTHREAD_MUTEX_INITIALIZER lo hizo)
    // pthread_mutex_init(&stats_mutex, NULL); 

    // 2. Configurar el SIGCHLD Handler
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    // SA_RESTART: para reanudar llamadas al sistema interrumpidas por la señal
    sa.sa_flags = SA_RESTART; 

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction ZOMBIE_INIT");
        exit(EXIT_FAILURE);
    }
}

pid_t zombie_safe_fork(void) {
    pid_t pid = fork();

    if (pid > 0) { 
        // Padre: Bloqueo y actualización de conteo de hijos creados
        pthread_mutex_lock(&stats_mutex);
        stats.zombies_created++;
        stats.zombies_active++;
        pthread_mutex_unlock(&stats_mutex);
    } 
    // Si pid == 0 (Hijo) o pid < 0 (Error), no se actualizan las estadísticas.

    return pid;
}

int zombie_safe_spawn(const char *command, char *args[]) {
    pid_t pid = zombie_safe_fork(); // Utiliza la función segura con actualización de estadísticas

    if (pid == -1) {
        perror("zombie_safe_spawn fork");
        return -1;
    }

    if (pid == 0) {
        // Hijo: Ejecuta el comando
        execv(command, args);
        // Si execv retorna, ha fallado.
        perror("zombie_safe_spawn execv");
        exit(EXIT_FAILURE); 
    }
    
    // Padre: Simplemente retorna. El reaprocesamiento es manejado por el SIGCHLD handler.
    return 0;
}

void zombie_get_stats(zombie_stats_t *stats_out) {
    if (!stats_out) return;

    // Bloquear y copiar las estadísticas de forma segura
    pthread_mutex_lock(&stats_mutex);
    memcpy(stats_out, &stats, sizeof(zombie_stats_t));
    pthread_mutex_unlock(&stats_mutex);
}
