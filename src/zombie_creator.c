#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/**
 * @brief Crea N procesos zombie para pruebas.
 * El padre NO llama a wait() - los hijos se convierten en zombies.
 * * @param count Número de procesos zombie a crear.
 * @return 0 en caso de éxito, -1 en caso de fallo.
 */
int create_zombies(int count) {
    if (count <= 0) {
        fprintf(stderr, "Error: El número de zombies debe ser positivo.\n");
        return -1;
    }

    printf("Iniciando la creación de %d procesos zombie...\n", count);

    for (int i = 0; i < count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            // Fallo al hacer fork
            perror("fork");
            // Intentamos salir del bucle si falla el fork
            return -1;
        } else if (pid == 0) {
            // Bloque del Proceso Hijo
            // Los hijos salen inmediatamente con un código de salida diferente.
            exit(i); 
        } else {
            // Bloque del Proceso Padre
            // Imprime el PID y el código de salida del hijo que se convertirá en zombie.
            printf("Created zombie: PID %d (exit code %d)\n", pid, i);
            // El padre intencionalmente NO llama a wait()
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int num_zombies = 5; // Valor por defecto

    if (argc == 2) {
        // Lee el número de zombies desde el argumento de línea de comandos
        num_zombies = atoi(argv[1]);
        if (num_zombies <= 0) {
            fprintf(stderr, "Uso: %s <Número_de_zombies_a_crear>\n", argv[0]);
            return 1;
        }
    } else if (argc > 2) {
        fprintf(stderr, "Uso: %s <Número_de_zombies_a_crear>\n", argv[0]);
        return 1;
    }

    if (create_zombies(num_zombies) == 0) {
        // El proceso padre se queda vivo para mantener a los zombies
        printf("\nProcesos zombie creados. El padre (PID %d) se mantiene vivo.\n", getpid());
        printf("Verifique los zombies en otra terminal: ps aux | grep 'Z'\n");
        
        // Espera una entrada para que el padre termine y limpie a los zombies
        printf("Presione ENTER para salir y permitir que los zombies sean limpiados (por init o el SO)...\n");
        getchar(); 
    } else {
        fprintf(stderr, "\nFallo en la creación de zombies.\n");
        return 1;
    }

    printf("Padre saliendo. Los zombies serán limpiados.\n");
    return 0;
}
