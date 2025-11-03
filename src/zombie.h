#ifndef ZOMBIE_H
#define ZOMBIE_H

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h> // Para EXIT_SUCCESS/FAILURE

// Estructura para las estadísticas de zombies
typedef struct {
    int zombies_created; // Contados al hacer fork
    int zombies_reaped;  // Contados al llamar a waitpid
    int zombies_active;  // zombies_created - zombies_reaped
} zombie_stats_t;

/**
 * @brief Inicializa la prevención de zombies (configura el SIGCHLD handler).
 * Debe llamarse una vez al inicio del programa.
 */
void zombie_init(void);

/**
 * @brief Realiza un fork con prevención de zombies (el padre confía en el handler).
 * @return PID del hijo en el padre, 0 en el hijo, -1 en caso de error.
 */
pid_t zombie_safe_fork(void);

/**
 * @brief Ejecuta un comando en un proceso hijo con prevención de zombies.
 * @param command Ruta al ejecutable.
 * @param args Argumentos para el comando.
 * @return 0 en éxito (del padre), -1 en error.
 */
int zombie_safe_spawn(const char *command, char *args[]);

/**
 * @brief Obtiene las estadísticas de reaprocesamiento de zombies.
 * @param stats Puntero a la estructura donde se almacenarán los datos.
 */
void zombie_get_stats(zombie_stats_t *stats);

#endif // ZOMBIE_H
