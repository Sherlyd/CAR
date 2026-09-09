/*
 * Ejercicio 2 - Matriz de transferencia
 *
 * Mide el ancho de banda entre CADA PAR de procesos (src, destino) usando un ping-pong con un mensaje de tamaño fijo         (MSG_SIZE), repetido NREPS veces para promediar. Los pares se prueban de a uno por vez (el resto de los procesos espera en una barrera), y el proceso 0 arma la matriz n x n con el ancho de banda (en MB/s) de cada par y la guarda en un CSV.
 Las inhomogeneidades de velocidades se refieren a las variaciones espaciales en la rapidez con la que se propaga una onda o un flujo a través de un medio determinado.
 */
 
/* traigo todas las funciones y tipos de MPI:
- MPI_Init: para arrancar el entorno MPI.
- MPI_Send: manda el mensaje desde este hacia otro proceso.
- MPI_COMM_WORLD: en este caso es la constante que represanta la cantidad de grupos con todos los procesos que se lanzaron, es más para saber en que grupo de proceso esta obteniendose cada respuesta.

stdio.h para:
- printf: salida por pantalla.
- fopen: abre o crea un archivo para leer y/o escribir.
- fprintf: escribe dentro de un archivo ya abierto con fopen

stdlib:
- malloc: sirve para reservar un bloque de memoria en bytes, pero arranca con basura.
- calloc: tambien reserva memoria pero ademas la inicializa en cero y busco que no arranque con basura, en este caso lo uso para ir sumando o completando valores en la matriz.
- free: libera la memoria que reserve anteriormente, es importante que no olvide que siempre debo liberar la memoria pedida para que no se quede con basura y desperdiciar espacio de memoria.
*/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/*Creo una cte (constante) y defino el tamaño del mensaje que voy a enviar en esta prueba uso '<<' ya que es el operador de desplazamiento a la izquierda, toma los bites de 'a' y los corre 'n' posiciones hacia la izq y rellena con ceros a la derecha, y cada que corro un byte a la derecha ese mismo numero se multiplica por 2 que es lo mismo que pasa en base 10, y hago esto ya que estoy trabajando en memoria, por lo que la cantidades se trabajan en potencia de 2 y no de 10

Cada medición se debe repetir 5 veces para poder promediar y que las variaciones aleatorias de la red no afecten tanto.*/
#define MSG_SIZE (1 << 20)   /* 2^20 ~= 1 MB */
#define NREPS 5

/*ARRANQUE DE MPI*/
int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);// Para saber en que grupo esta y en cada proceso que identificador tiene.
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);// para saber cuantos procesos hay en total en el grupo

    char *buf = malloc(MSG_SIZE);//reservo memoria para enviar y recibir el mensaje, solo mide cuanto tarda en ir y volver.
    double *matrix = NULL;
    if (rank == 0) {
        matrix = calloc((size_t)nprocs * nprocs, sizeof(double));
    }//aqui hacemos la matriz pero inicializada en cero

    for (int origen = 0; origen < nprocs; origen++) {//aqui genero todos los pares ordenados de proceso por ejemplo (0,1),(0,2)..etc, pero es (origen,destino) 
        for (int destino = 0; destino < nprocs; destino++) {
            if (origen == destino) continue;// no hace nada solo continua si un proceso se manda mensaje a sí mismo.

            MPI_Barrier(MPI_COMM_WORLD);// aseguro que ningun proceso siga sin que todos lleguen hasta aqui. sincronizo, para evitar errores.

            if (rank == origen) {//si el rank del proceso que se esta realizando coincide con el origen de la iteraccion actual entonces este proceso es el que va a mandar el mensake.
                double t0 = MPI_Wtime();//devuelve la hora y se guarda el instante de arrange en la variable t0
                
                for (int r = 0; r < NREPS; r++) {
                    MPI_Send(buf, MSG_SIZE, MPI_BYTE, destino, 0, MPI_COMM_WORLD);
                    MPI_Recv(buf, MSG_SIZE, MPI_BYTE, destino, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }//aqui se repite 5 veces el ida y vuelta del mensaje para poder medir el tiempo.
                double t1 = MPI_Wtime();
                double t_prom = (t1 - t0) / NREPS;//calculo el tiempo final con t1 y el tiempo que tardan las 5 idas y vueltas y calculo el promedio de una sola ida y vuelta y lo guardo en t_prom

                if (rank == 0) {
                    matrix[origen * nprocs + destino] = t_prom;
                } else {
                    MPI_Send(&t_prom, 1, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
                }/* Acá hay un detalle, como la matriz solo existe en la memoria del proceso 0, hay que llevarle el resultado.
Si el origen es el proceso 0, directamente escribe el resultado en su propia matriz. matrix[src * nprocs + dst] es la forma de acceder a la posición (origen, destino) de una matriz 2D guardada como un arreglo 1D (fila por fila): la fila origen empieza en la posición origen * nprocs, y le sumamos destino para llegar a la columna correspondiente.
Si el que midió no es el proceso 0, le manda el valor t_prom (un solo double) al proceso 0, con tag 2 (para no confundirlo con los mensajes de datos que usan tag 0 y 1)*/
            } else if (rank == destino) {
                for (int r = 0; r < NREPS; r++) {
                    MPI_Recv(buf, MSG_SIZE, MPI_BYTE, origen, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(buf, MSG_SIZE, MPI_BYTE, origen, 1, MPI_COMM_WORLD);
                }/*Si este proceso es el destino de la medición actual, hace exactamente el papel complementario: recibe el mensaje grande (tag 0) que le mandó origen, y le devuelve una confirmación (tag 1).*/
            } else if (rank == 0) {
                double t_prom;
                MPI_Recv(&t_prom, 1, MPI_DOUBLE, origen, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                matrix[origen * nprocs + destino] = t_prom;
            }/*Si el proceso 0 no es ni origen ni destino en esta iteración (por ejemplo, se está midiendo el par (2,3) y este es el proceso 0), igual tiene que recibir el resultado que el proceso origen le mandó, entonces lo recibe acá y lo guarda en la posición correspondiente de la matriz*/
        }
    }

    if (rank == 0) {
        FILE *f = fopen("matriz_transferencia.csv", "w");//escribimos el resultado en .csv que creamos, abrimos y escribimos
        for (int i = 0; i < nprocs; i++) {
            for (int j = 0; j < nprocs; j++) {
                double bw_mbs = 0.0;
                if (i != j) {
                    bw_mbs = (MSG_SIZE / matrix[i * nprocs + j]) / 1e6; /* MB/s */
                }
                fprintf(f, "%.3f%s", bw_mbs, (j == nprocs - 1) ? "\n" : ",");
            }
        }/* esta parte es medio trambolica pero intento que sea no tan dificil, recorro toda la matriz para convertir tiempo en ancho de banda: si tardó t segundos en mandar MSG_SIZE bytes, el ancho de banda es bytes / tiempo. Se divide por 1e6 para pasar de bytes/segundo a MB/s (1 MB = 10⁶ bytes en esta convención).
En la diagonal (i == j, un proceso "consigo mismo") deja 0.0 porque nunca se midió.
fprintf(f, "%.3f%s", bw_mbs, (j == nprocs - 1) ? "\n" : ","): escribe el número con 3 decimales, y después una coma (si no es la última columna de la fila) o un salto de línea (si es la última). Así arma el formato CSV, fila por fila.*/

        fclose(f);
        printf("Listo. Matriz de %dx%d guardada en matriz_transferencia.csv (MB/s)\n",
               nprocs, nprocs);
    }// termina y cierra, avisa que esta listo

    free(buf);
    if (rank == 0) free(matrix);//libero la memoria que pedí antes

    MPI_Finalize();
    return 0;// finalizo todo el entorno
}
