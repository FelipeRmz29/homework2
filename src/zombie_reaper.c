#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// --- Strategy 1: Explicit Wait ---

/**
 * @brief Cosecha (reaps) todos los procesos hijo explícitamente usando waitpid().
 * Se utiliza WNOHANG para no bloquear si no hay hijos terminados.
 */
void reap_explicit(void) {
    printf("--- Strategy 1: Explicit Wait (waitpid) ---\n");
    int status;
    pid_t pid;
    int reaped_count = 0;

    // waitpid(-1, &status, WNOHANG) intenta cosechar cualquier hijo.
    // > 0 significa que un hijo fue cosechado.
    // Bucle para asegurar que se cosechan todos los hijos que terminaron.
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("  [Reaper - Explicit]: Reaped child PID %d with exit code %d.\n", 
               pid, WEXITSTATUS(status));
        reaped_count++;
    }

    if (pid == -1 && errno != ECHILD) {
        perror("waitpid");
    }

    printf("Explicit reaping finished. Total reaped: %d\n", reaped_count);
}

// --- Strategy 2: SIGCHLD Handler ---

/**
 * @brief Manejador de la señal SIGCHLD. 
 * Se ejecuta de forma asíncrona cuando un hijo termina.
 */
void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    // IMPORTANTE: Se utiliza un bucle while para cosechar *todos* los hijos 
    // que terminaron desde la última señal, previniendo race conditions 
    // donde múltiples hijos terminan antes de que el handler se ejecute.
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // En un daemon o aplicación de larga duración, usaríamos write() o syslog 
        // en lugar de printf() dentro del signal handler, pero para la demostración 
        // de la prueba se usa printf().
        printf("  [Reaper - Handler]: Reaped child PID %d (Async).\n", pid);
    }

    if (pid == -1 && errno != ECHILD) {
        // En un handler real, se debe tener cuidado con las llamadas a funciones.
        // Aquí se omite el manejo de errores complejos por simplicidad de la prueba.
    }
}

/**
 * @brief Configura el manejador de señal SIGCHLD para el reaprocesamiento automático.
 */
void setup_auto_reaper(void) {
    printf("--- Strategy 2: SIGCHLD Handler ---\n");
    struct sigaction sa;

    // Inicializa la estructura sigaction
    sa.sa_handler = sigchld_handler; // Asigna la función manejadora
    sigemptyset(&sa.sa_mask);        // No bloquea otras señales
    sa.sa_flags = SA_RESTART;        // Reinicia llamadas al sistema interrumpidas

    // Instala el manejador para la señal SIGCHLD
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    printf("SIGCHLD handler installed. Children will be reaped automatically.\n");
}

// --- Strategy 3: Ignore SIGCHLD ---

/**
 * @brief Usa signal(SIGCHLD, SIG_IGN) para el reaprocesamiento automático.
 * Cuando SIGCHLD se establece en SIG_IGN, los hijos que terminan no se convierten 
 * en zombies; el sistema los elimina automáticamente.
 */
void setup_ignore_reaper(void) {
    printf("--- Strategy 3: Ignore SIGCHLD (SIG_IGN) ---\n");
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal SIG_IGN");
        exit(EXIT_FAILURE);
    }
    printf("SIGCHLD set to SIG_IGN. Children will be automatically reaped by kernel.\n");
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <estrategia>\n", argv[0]);
        fprintf(stderr, "Estrategias: 1 (Explicit Wait), 2 (SIGCHLD Handler), 3 (SIG_IGN)\n");
        return 1;
    }

    int strategy = atoi(argv[1]);
    if (strategy < 1 || strategy > 3) {
        fprintf(stderr, "Estrategia inválida. Use 1, 2, o 3.\n");
        return 1;
    }
    
    // Crear 10 procesos hijo
    printf("Creating 10 child processes...\n");
    for (int i = 0; i < 10; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork");
            // No salir, intentar continuar si es posible
            break; 
        }

        if (pid == 0) {
            // Proceso Hijo
            // Simula algún trabajo y termina
            srand(getpid() * i); // Semilla única para sleep
            sleep(rand() % 3);
            exit(i);
        }
        // Proceso Padre continúa sin llamar a wait()
    }
    
    // Usar la estrategia elegida
    switch(strategy) {
        case 1: 
            // Esperar un poco para que los hijos terminen y se conviertan en zombies
            sleep(3); 
            reap_explicit(); 
            break;
        case 2: 
            setup_auto_reaper(); 
            break;
        case 3: 
            setup_ignore_reaper(); 
            break;
    }
    
    // Esperar un tiempo suficiente para que todos los hijos terminen 
    // y el reaprocesamiento automático (estrategias 2 y 3) surta efecto.
    // Estrategia 1 ya cosechó, pero esperamos para verificar.
    printf("\nParent waiting for 5 seconds to allow all children to finish and reaping to occur...\n");
    sleep(5);  
    
    printf("\nVerification check (searching for 'defunct' processes):\n");
    // Verificar que no queden zombies
    system("ps aux | grep defunct | grep -v grep");
    
    printf("\nZombie reaper test finished.\n");

    // Para la estrategia 2, el padre podría terminar después de esto.
    // Para la estrategia 1, todos los hijos ya fueron cosechados.
    // Para la estrategia 3, el kernel los cosechó.
    return 0;
}
