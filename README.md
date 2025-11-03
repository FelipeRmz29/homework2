````markdown

| **Nombre Completo** | **Luis Felipe Ramírez Torres** |
| **Matrícula** | 5125519 |

---

## 🎯 Objetivo del Proyecto

Este proyecto es una implementación completa para entender y gestionar el ciclo de vida de los procesos en Linux, enfocándose en la **creación, detección y cosecha (reaping)** de **procesos zombie**. El objetivo es demostrar diversas estrategias de prevención y limpieza de zombies en aplicaciones de larga duración.

---

## ⚙️ Compilación y Ejecución

El proyecto utiliza un **`Makefile`** para automatizar la compilación de todos los ejecutables (`zombie_creator`, `zombie_detector`, `zombie_reaper`, `process_daemon`, `test_lib`) y la creación de la librería estática (`libzombie.a`).

### 1. Compilar todo

```bash
make all
````

### 2\. Ejecutar Pruebas (Recomendado)

El `Makefile` incluye una regla para ejecutar todos los scripts de prueba (`.sh`) secuencialmente.

```bash
make test_all
```

### 3\. Limpieza

```bash
make clean
```

-----

## 📂 Descripción de los Módulos

| Archivo | Parte | Función Principal |
| :--- | :--- | :--- |
| `zombie_creator.c` | **Parte 1** | Genera intencionalmente **N procesos zombie** al omitir `wait()`. |
| `zombie_detector.c` | **Parte 2** | Escanea `/proc` y genera un reporte detallado de los procesos en estado **'Z'** (Zombie). |
| `zombie_reaper.c` | **Parte 3** | Demuestra y prueba 3 estrategias distintas para **cosechar** zombies: `waitpid` explícito, `SIGCHLD` handler e `IGNORE SIGCHLD`. |
| `process_daemon.c` | **Parte 4** | Implementa un demonio de larga duración que **nunca crea zombies** al usar el `SIGCHLD Handler` para la cosecha automática. |
| `zombie.c` / `zombie.h` | **Parte 5** | Crea la librería estática `libzombie.a` con funciones seguras (`zombie_safe_fork`) y estadísticas protegidas por **mutex**. |

-----

## 🧪 Pruebas Automatizadas

El directorio `tests/` contiene scripts de *shell* para verificar la funcionalidad de cada componente.

| Script | Ejecutable Probado | Objetivo de la Prueba |
| :--- | :--- | :--- |
| `test_creator.sh` | `zombie_creator` | Verifica la creación de zombies y su correcta limpieza. |
| `test_detector.sh` | `zombie_detector` | Verifica la precisión del reporte y la identificación del proceso padre (PPID). |
| `test_reaper.sh` | `zombie_reaper` | Ejecuta y verifica que las **3 estrategias de cosecha** limpian por completo a los zombies. |
| `test_daemon.sh` | `process_daemon` | Monitorea el demonio para garantizar que **cero** procesos zombie sean creados por los trabajadores. |

```

Si necesitas alguna modificación en el formato o el contenido de este `README.md`, solo dímelo.
```
