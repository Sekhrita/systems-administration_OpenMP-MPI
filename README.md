# systems-administration_OpenMP-MPI

En este torneo de fútbol, los equipos compiten en una fase de clasificación: una liga local de formato "todos contra todos" por país. El proyecto utiliza paralelismo con OpenMP y MPI para simular una competencia realista en la que se clasifican los mejores equipos a nivel nacional e internacional de manera simultanea.

## Objetivos del Proyecto

- Implementar un torneo de fútbol en el que los equipos compiten en una liga local por país.

- Utilizar el paralelismo con OpenMP y MPI para optimizar el cálculo de rendimientos y la gestión de partidos; donde cada slot o núcleo procesa el torneo de un país (8 por default, por con escalabilidad).

- Clasificar a los dos mejores equipos de cada país (8 países) para la eliminatoria internacional.

## Objetivos del Proyecto

El proyecto está diseñado para simular un torneo de fútbol con 8 países, cada uno con 20 equipos. Cada equipo tiene 25 jugadores, y el rendimiento de cada jugador se calcula de manera aleatoria antes de cada partido. La implementación utiliza OpenMP para calcular en paralelo el rendimiento de los jugadores y MPI para distribuir la simulación de cada país en nodos separados, permitiendo que las ligas locales se ejecuten en paralelo.


## Consideraciones Técnicas

Para los usuarios de WSL (Windows Subsystem for Linux), es necesario crear un archivo `hostfile` que contenga el número de nodos que se van a utilizar. Este archivo debe incluir la dirección `localhost` si se ejecuta en una sola máquina, o las direcciones IP de las máquinas involucradas en el caso de un entorno distribuido. Un ejemplo de contenido para `hostfile` podría ser:

```bash
localhost slots=8
```

## Instalación de Dependencias (Debian OS)

Para instalar las dependencias necesarias en un sistema operativo Debian, ejecute los siguientes comandos:

```bash
sudo apt update -y && sudo apt upgrate -y
sudo apt install -y build-essential libopenmpi-dev openmpi-bin
```
Esto instalará el compilador gcc, las bibliotecas de MPI y el binario de OpenMPI necesario para ejecutar el proyecto.

## Cómo Correr el Proyecto

1. Compile el código fuente utilizando el compilador `mpicc`:
   ```bash
   mpicc -fopenmp -o torneo torneo.c
   ```

2. Ejecute el programa con `mpirun`, asegurándose de especificar un archivo de hosts si está ejecutando en más de una máquina. El programa requiere 8 procesos MPI (uno para cada país)::
   ```bash
   mpirun --hostfile hostfile -np 8 ./torneo
   ```
   Asegúrese de tener un archivo hostfile con las direcciones de las máquinas involucradas si ejecuta en un entorno distribuido.


## Cosas a Tener en Cuenta

- Sistema de Desempate: Si al finalizar la liga local más de dos equipos están empatados en puntaje, se aplica un mecanismo de desempate aleatorio. A uno de los equipos empatados se le suman 2 puntos y a otro se le suma 1 punto para evitar empates en la clasificación final.

- OpenMP y MPI: OpenMP se usa para calcular el rendimiento de cada jugador en paralelo antes de cada partido, y MPI se utiliza para asignar cada país a un nodo separado, permitiendo una ejecución asíncrona y en paralelo de las ligas locales.

## Archivos `.log` Generados

- `logs_partidos_pais/log_pais_X.txt`: Estos archivos registran los resultados de cada partido para el país `X` correspondiente. Incluyen información sobre el equipo local, el equipo visitante, el desenlace del partido (victoria o empate) y el ganador.

- `tabla_puntuacion/tabla_puntuacion_pais_X.tsv`: Estos archivos contienen la tabla de puntuación final de cada país `X` en formato TSV (Tab-Separated Values), mostrando el puntaje acumulado por cada equipo después de la liga local. Estos archivos permiten identificar los dos equipos que avanzan a la siguiente fase.

