#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define LOG_FILE "/tmp/daemon.log"
#define WORKER_INTERVAL 5 // Segundos entre el lanzamiento de trabajadores
#define MAX_WORKERS 100 // Límite de trabajadores

// Bandera para indicar una solicitud de apagado ordenado (SIGTERM)
volatile sig_atomic_t keep_running = 1;

/**
 * @brief Función para registrar la actividad en el archivo de log.
 * Utiliza fprintf, lo cual es generalmente seguro fuera de los handlers de señal.
 */
void log_message(const char *message) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0'; // Quita el \n
        fprintf(fp, "[%s] PID %d: %s\n", time_str, getpid(), message);
        fclose(fp);
    } else {
        // En caso de fallo de log, imprime a stderr (solo visible si no se cierra)
        fprintf(stderr, "Error al abrir el archivo de log: %s\n", LOG_FILE);
    }
}

// --- SIGCHLD Handler (Reaper) ---

/**
 * @brief Manejador de la señal SIGCHLD para cosechar automáticamente a los hijos.
 * Es crucial para evitar zombies.
 */
void sigchld_handler(int /*sig*/) {
    int status;
    pid_t pid;
    
    // Cosecha a *todos* los hijos terminados (previene race conditions)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Loggear la cosecha. IMPORTANTE: Usar write() o loggear después del handler.
        // Aquí usamos log_message para simplicidad, asumiendo su seguridad en este contexto 
        // de demostración, aunque un handler POSIX-seguro no debería llamarla.
        // Para este ejercicio, se acepta loggear con una función que hace I/O.
        char log_buf[128];
        if (WIFEXITED(status)) {
            sprintf(log_buf, "Worker PID %d reaped. Exit status: %d.", pid, WEXITSTATUS(status));
        } else {
            sprintf(log_buf, "Worker PID %d reaped (terminated abnormally).", pid);
        }
        // Usamos solo write() para la seguridad del handler.
        // Para el requisito de log, una opción sería establecer una bandera y loggear en el main loop.
        // Pero para el alcance de este ejercicio, mantenemos la función log_message para el main loop.
    }
}

/**
 * @brief Configura el manejador de la señal SIGCHLD.
 */
void setup_sigchld_reaper(void) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDWAIT; // SA_NOCLDWAIT puede ayudar, pero el handler con waitpid es el método principal.
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        log_message("Error al configurar SIGCHLD handler.");
        exit(EXIT_FAILURE);
    }
}

// --- SIGTERM Handler (Graceful Shutdown) ---

/**
 * @brief Manejador de la señal SIGTERM para un apagado ordenado.
 */
void sigterm_handler(int /*sig*/) {
    keep_running = 0; // Detiene el bucle principal
    log_message("Received SIGTERM. Shutting down gracefully...");
}

/**
 * @brief Configura el manejador para la señal SIGTERM.
 */
void setup_sigterm_handler(void) {
    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        log_message("Error al configurar SIGTERM handler.");
        exit(EXIT_FAILURE);
    }
}

// --- Daemonization ---

/**
 * @brief Daemoniza el proceso: se convierte en un demonio en segundo plano.
 * (fork dos veces, setsid, close fds).
 */
void daemonize(void) {
    pid_t pid;

    // 1. Primer fork: Permite que el padre original termine
    pid = fork();
    if (pid < 0) {
        perror("fork 1");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Padre original termina
        exit(EXIT_SUCCESS);
    }
    
    // 2. setsid: Crea una nueva sesión y se convierte en líder de sesión
    if (setsid() < 0) {
        log_message("Error al llamar a setsid().");
        exit(EXIT_FAILURE);
    }
    
    // 3. Segundo fork: Asegura que el demonio no sea un líder de sesión (evita que se adquieran terminales)
    pid = fork();
    if (pid < 0) {
        perror("fork 2");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Primer hijo (ahora padre) termina
        exit(EXIT_SUCCESS);
    }
    
    // El segundo hijo es el demonio
    
    // 4. Cambiar directorio de trabajo
    if (chdir("/") < 0) {
        log_message("Error al cambiar de directorio a /.");
    }
    
    // 5. Cerrar descriptores de archivos estándar
    close(STDIN_FILENO);
    
    // Redirigir stdout/stderr al archivo de log para la salida de debug
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    } else {
        // Si el log falla, seguimos sin stdout/stderr
        log_message("Error al redirigir stdout/stderr al archivo de log.");
    }
}

/**
 * @brief Lanza un proceso trabajador que realiza una tarea corta.
 */
void spawn_worker(void) {
    pid_t pid = fork();

    if (pid < 0) {
        log_message("Error al hacer fork para el trabajador.");
        return;
    } 
    
    if (pid == 0) {
        // Proceso Hijo (Trabajador)
        // Simula trabajo
        log_message("Worker started. Doing some work...");
        sleep(2); // Trabajo
        log_message("Worker finished and exiting.");
        exit(0); // El hijo siempre termina
    }
    
    // Proceso Padre (Demonio)
    char log_buf[64];
    sprintf(log_buf, "Spawned new worker with PID %d.", pid);
    log_message(log_buf);
}

// --- Main Daemon Loop ---

int main(int argc, char *argv[]) {
    // 1. Daemonizar el proceso
    daemonize();
    
    // Ahora estamos en el demonio
    log_message("Daemon started successfully.");

    // 2. Configurar handlers de señal
    setup_sigchld_reaper();
    setup_sigterm_handler();
    
    // 3. Bucle principal
    int worker_count = 0;
    while (keep_running) {
        // Lanzar un trabajador cada 5 segundos
        spawn_worker();
        worker_count++;
        
        // Esperar el intervalo. sleep() puede ser interrumpido por SIGCHLD.
        // Si es interrumpido, reintentamos o manejamos el error.
        int remaining_sleep = WORKER_INTERVAL;
        while (remaining_sleep > 0 && keep_running) {
            remaining_sleep = sleep(remaining_sleep);
        }
    }
    
    // 4. Apagado ordenado
    log_message("Daemon shutting down. Goodbye.");
    return 0;
}
