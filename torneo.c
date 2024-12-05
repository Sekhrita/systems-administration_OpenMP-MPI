#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_PAISES 8
#define NUM_EQUIPOS 20
#define JUGADORES_POR_EQUIPO 25
#define JUGADORES_EN_PARTIDO 11

// Estructura para almacenar información de un equipo
typedef struct {
    int id;
    int rendimiento[JUGADORES_POR_EQUIPO];
    int puntaje;
} Equipo;

void calcular_rendimiento_jugadores(Equipo* equipo) {
    #pragma omp parallel for
    for (int i = 0; i < JUGADORES_POR_EQUIPO; i++) {
        // Usar una semilla única basada en el tiempo, el número de hilo y el índice del jugador
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned int seed = ts.tv_nsec ^ omp_get_thread_num() ^ i;
        equipo->rendimiento[i] = rand_r(&seed) % 99 + 1; // Valor aleatorio entre 1 y 99
    }
}

int calcular_puntaje_equipo(Equipo* equipo) {
    // Seleccionar a los 11 jugadores con mayor rendimiento
    int puntajes[JUGADORES_EN_PARTIDO];
    
    // Copiar los rendimientos para seleccionar los mejores
    int rendimientos[JUGADORES_POR_EQUIPO];
    for (int i = 0; i < JUGADORES_POR_EQUIPO; i++) {
        rendimientos[i] = equipo->rendimiento[i];
    }

    // Seleccionar los 11 mejores jugadores
    for (int i = 0; i < JUGADORES_EN_PARTIDO; i++) {
        int max_idx = i;
        for (int j = i + 1; j < JUGADORES_POR_EQUIPO; j++) {
            if (rendimientos[j] > rendimientos[max_idx]) {
                max_idx = j;
            }
        }
        // Intercambiar valores
        int temp = rendimientos[i];
        rendimientos[i] = rendimientos[max_idx];
        rendimientos[max_idx] = temp;
        puntajes[i] = rendimientos[i];
    }

    // Calcular el puntaje total del equipo (suma de los 11 mejores rendimientos)
    int puntaje_total = 0;
    for (int i = 0; i < JUGADORES_EN_PARTIDO; i++) {
        puntaje_total += puntajes[i];
    }
    return puntaje_total;
}

int comparar_puntajes(const void* a, const void* b) {
    Equipo* equipoA = (Equipo*)a;
    Equipo* equipoB = (Equipo*)b;
    return equipoB->puntaje - equipoA->puntaje;
}

void resolver_empates(Equipo equipos[], int num_equipos) {
    // Buscar el puntaje más alto y cuántos equipos tienen ese puntaje
    int max_puntaje = equipos[0].puntaje;
    int empatados[NUM_EQUIPOS];
    int num_empatados = 0;

    for (int i = 0; i < num_equipos; i++) {
        if (equipos[i].puntaje == max_puntaje) {
            empatados[num_empatados++] = i;
        }
    }

    // Si hay más de dos equipos empatados
    if (num_empatados > 2) {
        // Generar un índice aleatorio para seleccionar un equipo al cual sumar 2 puntos
        int idx_ganador = rand() % num_empatados;
        equipos[empatados[idx_ganador]].puntaje += 2;

        // Generar un índice diferente para sumar 1 punto a otro equipo
        int idx_segundo;
        do {
            idx_segundo = rand() % num_empatados;
        } while (idx_segundo == idx_ganador);

        equipos[empatados[idx_segundo]].puntaje += 1;
    }
}

void simular_liga_local(Equipo equipos[], FILE* log_file) {
    // Simular la liga local (todos contra todos)
    int partido_id = 0;
    for (int i = 0; i < NUM_EQUIPOS; i++) {
        for (int j = i + 1; j < NUM_EQUIPOS; j++) {
            // Calcular el rendimiento de los equipos antes de cada partido usando OpenMP
            calcular_rendimiento_jugadores(&equipos[i]);
            calcular_rendimiento_jugadores(&equipos[j]);

            int puntaje1 = calcular_puntaje_equipo(&equipos[i]);
            int puntaje2 = calcular_puntaje_equipo(&equipos[j]);

            if (puntaje1 > puntaje2) {
                equipos[i].puntaje += 3;
                fprintf(log_file, "Equipo %d\tEquipo %d\tVictoria\tEquipo %d\n", i, j, i);
            } else if (puntaje1 < puntaje2) {
                equipos[j].puntaje += 3;
                fprintf(log_file, "Equipo %d\tEquipo %d\tVictoria\tEquipo %d\n", i, j, j);
            } else {
                equipos[i].puntaje += 1;
                equipos[j].puntaje += 1;
                fprintf(log_file, "Equipo %d\tEquipo %d\tEmpate\t---\n", i, j);
            }
            partido_id++;
        }
    }
}

int main(int argc, char** argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != NUM_PAISES) {
        if (rank == 0) {
            printf("Este programa requiere exactamente %d nodos MPI.\n", NUM_PAISES);
        }
        MPI_Finalize();
        return -1;
    }

    // Inicializar la semilla aleatoria para cada proceso MPI
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand(ts.tv_nsec ^ rank);

    // Crear carpetas para almacenar los logs y las tablas de puntuación
    mkdir("logs_partidos_pais", 0777);
    mkdir("tabla_puntuacion", 0777);

    // Inicializar los equipos del país correspondiente a este nodo
    Equipo equipos[NUM_EQUIPOS];
    for (int i = 0; i < NUM_EQUIPOS; i++) {
        equipos[i].id = i;
        equipos[i].puntaje = 0;
    }

    // Abrir archivo de log para registrar los resultados de los partidos
    char log_filename[100];
    sprintf(log_filename, "logs_partidos_pais/log_pais_%d.txt", rank);
    FILE* log_file = fopen(log_filename, "w");
    if (log_file == NULL) {
        printf("Error al abrir el archivo de log.\n");
        MPI_Finalize();
        return -1;
    }

    // Escribir la fecha de creación en el archivo de log
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log_file, "# Fecha de creación: %02d-%02d-%d %02d:%02d:%02d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(log_file, "Equipo_local\tEquipo_visita\tDesenlace\tGanador\n");

    // Simular la liga local para el país correspondiente
    simular_liga_local(equipos, log_file);
    

    fclose(log_file);

    // Llamar a la función para resolver empates antes de ordenar los equipos
    resolver_empates(equipos, NUM_EQUIPOS);

    // Ordenar los equipos por puntaje de mayor a menor
    qsort(equipos, NUM_EQUIPOS, sizeof(Equipo), comparar_puntajes);

    // Crear archivo de puntuación en formato TSV
    char tsv_filename[100];
    sprintf(tsv_filename, "tabla_puntuacion/tabla_puntuacion_pais_%d.tsv", rank);
    FILE* tsv_file = fopen(tsv_filename, "w");
    if (tsv_file == NULL) {
        printf("Error al abrir el archivo de puntuación.\n");
        MPI_Finalize();
        return -1;
    }

    // Escribir la fecha de creación en el archivo de puntuación
    fprintf(tsv_file, "# Fecha de creación: %02d-%02d-%d %02d:%02d:%02d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(tsv_file, "Equipo\tPuntaje\n");
    for (int i = 0; i < NUM_EQUIPOS; i++) {
        fprintf(tsv_file, "%d\t%d\n", equipos[i].id, equipos[i].puntaje);
    }

    fclose(tsv_file);

    // Encontrar los dos mejores equipos
    int primero = equipos[0].id;
    int segundo = equipos[1].id;

    // Enviar los dos mejores equipos al nodo principal (rank 0)
    int mejores_equipos[2] = {primero, segundo};
    int recv_buffer[NUM_PAISES * 2];
    MPI_Gather(mejores_equipos, 2, MPI_INT, recv_buffer, 2, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Procesar los equipos recibidos para la fase de eliminatoria
        printf("Fase de liga completada. Los mejores equipos de cada país avanzan a la eliminatoria.\n");
        for (int i = 0; i < NUM_PAISES; i++) {
            printf("País %d: Equipo %d y Equipo %d avanzan\n", i, recv_buffer[i * 2], recv_buffer[i * 2 + 1]);
        }
        // Aquí se implementaría la lógica de la eliminatoria
    }

    MPI_Finalize();
    return 0;
}
