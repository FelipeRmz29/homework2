#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Nota: Usa la ruta relativa correcta para el header
#include "../src/zombie.h" 

#define NUM_PROCESSES 5

int main() {
    zombie_stats_t current_stats;
    int i;
    
    printf("--- Test de la Librería Zombie Prevention ---\n");
    
    // 1. Inicializar el handler de cosecha (vital para la librería)
    zombie_init();
    printf("Zombie handler inicializado. Los procesos hijos serán cosechados automáticamente.\n");

    // 2. Crear procesos usando zombie_safe_fork
    printf("\nCreando %d hijos usando zombie_safe_fork...\n", NUM_PROCESSES);
    for (i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = zombie_safe_fork();
        
        if (pid == -1) {
            perror("zombie_safe_fork falló");
            break;
        }

        if (pid == 0) {
            // Proceso Hijo
            sleep(1); // Simula trabajo
            exit(i + 1); // El hijo termina y se convierte en zombie (temporalmente)
        }
    }
    
    // 3. Esperar un tiempo para que todos los hijos terminen y sean cosechados
    printf("\nEsperando 3 segundos para permitir el reaprocesamiento automático por el SIGCHLD handler...\n");
    sleep(3); 

    // 4. Obtener y mostrar estadísticas
    zombie_get_stats(&current_stats);

    printf("\n--- Estadísticas Finales de Zombies ---\n");
    printf("Procesos Creados (fork/spawn): %d\n", current_stats.zombies_created);
    printf("Procesos Cosechados (Reaped): %d\n", current_stats.zombies_reaped);
    printf("Zombies Activos (Debería ser 0): %d\n", current_stats.zombies_active);

    // 5. Verificación final de zombies
    printf("\nVerificación de Ausencia de Zombies ('defunct'):\n");
    system("ps aux | grep defunct | grep -v grep");
    
    if (current_stats.zombies_active == 0 && current_stats.zombies_created == current_stats.zombies_reaped) {
        printf("\n[ÉXITO] La librería previno la creación de zombies activos y las estadísticas son correctas.\n");
        return 0;
    } else {
        printf("\n[FALLO] La librería no cosechó a todos los procesos o el conteo de estadísticas es incorrecto.\n");
        return 1;
    }
}
