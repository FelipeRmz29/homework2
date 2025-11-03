#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_ZOMBIES 1024

// Estructura para almacenar información básica de los zombies
typedef struct {
    int pid;
    int ppid;
    char command[256];
} zombie_info_t;

/**
 * @brief Obtiene el tiempo de CPU (user + system) en segundos de un proceso.
 * @param pid ID del proceso.
 * @return Tiempo total de CPU en segundos, o -1 en caso de error.
 */
long get_cputime_seconds(int pid) {
    char stat_path[256];
    FILE *fp;
    long utime, stime;
    long Hertz = sysconf(_SC_CLK_TCK);

    sprintf(stat_path, "/proc/%d/stat", pid);
    fp = fopen(stat_path, "r");
    if (!fp) {
        return -1;
    }

    // El archivo /proc/[pid]/stat contiene 52 campos.
    // utime (14to campo) y stime (15to campo) son los tiempos que nos interesan.
    // Usamos el formato para leer hasta el campo 15.
    if (fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld", 
               &utime, &stime) != 2) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    // El tiempo total es la suma, dividido por el valor de HZ (ticks por segundo)
    return (utime + stime) / Hertz;
}

/**
 * @brief Imprime la información sobre un proceso zombie.
 * @param info Estructura con la información del zombie (PID, PPID, Comando).
 * @param cputime_sec Tiempo total de CPU del zombie en segundos.
 */
void print_zombie_info(const zombie_info_t *info, long cputime_sec) {
    long hours = cputime_sec / 3600;
    long minutes = (cputime_sec % 3600) / 60;
    long seconds = cputime_sec % 60;
    
    // Imprime la fila del reporte
    printf("%-8d%-8d%-16s%-8c%02ld:%02ld:%02ld\n", 
           info->pid, info->ppid, info->command, 'Z', hours, minutes, seconds);
}

/**
 * @brief Escanea el sistema de archivos /proc en busca de procesos zombie.
 * @param zombie_list Arreglo para almacenar la información de los zombies encontrados.
 * @param max_zombies Capacidad máxima del arreglo.
 * @return Número de zombies encontrados.
 */
int find_zombies(zombie_info_t *zombie_list, int max_zombies) {
    DIR *dir;
    struct dirent *entry;
    int zombies_found = 0;

    // 1. Abrir el directorio /proc
    dir = opendir("/proc");
    if (dir == NULL) {
        perror("opendir /proc");
        return 0;
    }

    // 2. Iterar sobre las entradas del directorio /proc
    while ((entry = readdir(dir)) != NULL && zombies_found < max_zombies) {
        // Verificar si la entrada es un PID (solo contiene dígitos)
        int is_pid = 1;
        for (char *p = entry->d_name; *p; p++) {
            if (!isdigit(*p)) {
                is_pid = 0;
                break;
            }
        }

        if (is_pid) {
            char stat_path[256];
            FILE *fp;
            int pid, ppid;
            char comm[256];
            char state;

            // 3. Construir la ruta a /proc/[pid]/stat
            sprintf(stat_path, "/proc/%s/stat", entry->d_name);
            fp = fopen(stat_path, "r");

            if (fp) {
                // 4. Leer el archivo /proc/[pid]/stat para obtener PID, COMM, Estado y PPID
                // Formato de lectura: %d (PID) %s (COMM) %c (STATE) %d (PPID)
                if (fscanf(fp, "%d %s %c %d", &pid, comm, &state, &ppid) == 4) {
                    // 5. Detectar el estado 'Z' (Zombie)
                    if (state == 'Z') {
                        // Limpiar el nombre del comando: quitar paréntesis
                        comm[strlen(comm) - 1] = '\0'; // Quita el ')' final
                        
                        // Almacenar la información del zombie
                        zombie_list[zombies_found].pid = pid;
                        zombie_list[zombies_found].ppid = ppid;
                        strcpy(zombie_list[zombies_found].command, comm + 1); // Quita el '(' inicial

                        zombies_found++;
                    }
                }
                fclose(fp);
            }
        }
    }

    closedir(dir);
    return zombies_found;
}

/**
 * @brief Implementa la lógica de análisis del proceso padre.
 * @param zombie_list Lista de zombies encontrados.
 * @param count Número de zombies encontrados.
 */
void analyze_parents(const zombie_info_t *zombie_list, int count) {
    if (count == 0) return;

    // Encontrar PPIDs únicos y contar sus zombies
    // En un sistema real se usaría un hash map, aquí usaremos un array simple.
    int parent_pids[count];
    int zombie_counts[count];
    int unique_parents = 0;

    for (int i = 0; i < count; i++) {
        int ppid = zombie_list[i].ppid;
        int found = 0;
        for (int j = 0; j < unique_parents; j++) {
            if (parent_pids[j] == ppid) {
                zombie_counts[j]++;
                found = 1;
                break;
            }
        }
        if (!found) {
            parent_pids[unique_parents] = ppid;
            zombie_counts[unique_parents] = 1;
            unique_parents++;
        }
    }

    printf("\nParent Process Analysis:\n");
    for (int i = 0; i < unique_parents; i++) {
        char parent_cmd[256] = "unknown";
        char stat_path[256];
        FILE *fp;
        int pid, ppid_dummy;
        char state;
        
        // Intentar obtener el nombre del comando del padre
        sprintf(stat_path, "/proc/%d/stat", parent_pids[i]);
        fp = fopen(stat_path, "r");
        if (fp) {
            // Leer los primeros 4 campos: PID, COMM, STATE, PPID
            if (fscanf(fp, "%d %s %c %d", &pid, parent_cmd, &state, &ppid_dummy) == 4) {
                // Limpiar el nombre del comando
                parent_cmd[strlen(parent_cmd) - 1] = '\0';
                // Si el padre también es zombie o no existe, su nombre aparecerá como "defunct"
                printf("  PID %d (%s) has %d zombie children\n", 
                       parent_pids[i], parent_cmd + 1, zombie_counts[i]);
            } else {
                 printf("  PID %d (command unknown) has %d zombie children\n", 
                       parent_pids[i], zombie_counts[i]);
            }
            fclose(fp);
        } else {
            // Si /proc/[ppid] no existe, el padre ya terminó y fue adoptado por init (pero este reporte lo ignora)
            printf("  PID %d (process terminated) has %d zombie children\n", 
                   parent_pids[i], zombie_counts[i]);
        }
    }
}

int main() {
    zombie_info_t zombie_list[MAX_ZOMBIES];
    int total_zombies;

    // 1. Escanear y encontrar zombies
    total_zombies = find_zombies(zombie_list, MAX_ZOMBIES);

    // 2. Imprimir el encabezado del reporte
    printf("=== Zombie Process Report ===\n");
    printf("Total Zombies: %d\n\n", total_zombies);

    if (total_zombies > 0) {
        // 3. Imprimir la tabla de detalles
        printf("%-8s%-8s%-16s%-8s%-8s\n", "PID", "PPID", "Command", "State", "Time");
        printf("------- ------- ---------------- ----- --------\n");

        for (int i = 0; i < total_zombies; i++) {
            long cputime = get_cputime_seconds(zombie_list[i].pid);
            print_zombie_info(&zombie_list[i], cputime);
        }

        // 4. Imprimir el análisis de padres
        analyze_parents(zombie_list, total_zombies);
    } else {
        printf("¡No se encontraron procesos zombie en el sistema!\n");
    }

    return 0;
}
