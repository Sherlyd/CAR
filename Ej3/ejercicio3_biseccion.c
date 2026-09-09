/*
 * Ejercicio 3 - Ancho de banda de bisección
 *
 * Divide los n procesos en dos mitades: [0, n/2) y [n/2, n). El proceso i de
 * la primera mitad se empareja con el proceso i+n/2 de la segunda mitad, y
 * ambos se envían datos SIMULTÁNEAMENTE (Isend/Irecv), repitiendo NREPS
 * veces. El ancho de banda de bisección es:
 *
 *     BW_bisec = (n/2 * MSG_SIZE) / tiempo_maximo
 *
 * (la cantidad total de datos movidos en paralelo, dividido el tiempo que
 * tardó el par más lento). Requiere un número PAR de procesos.
 *
 */
#include <mpi.h>      /* funciones y tipos de MPI */
#include <stdio.h>    /* printf, fprintf */
#include <stdlib.h>   /* malloc, free */

#define MSG_SIZE (1 << 24)   /* 2^24 = 16777216 bytes ≈ 16 MB: tamaño del mensaje a transferir */
#define NREPS 5              /* cantidad de repeticiones para promediar la medición */

int main(int argc, char **argv) {

    /* ARRANQUE DE MPI */
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);    /* "quién soy yo": mi número de proceso (0..nprocs-1) */
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);  /* cuántos procesos hay en total (el "n") */

    /*Validación: el algoritmo necesita poder partir los procesos en dos mitades iguales*/
    if (nprocs % 2 != 0) {
        if (rank == 0) {  /* que el mensaje de error se imprima una sola vez, no una por proceso */
            fprintf(stderr, "Error: se requiere un numero PAR de procesos (--ntasks par)\n");
        }
        MPI_Finalize();   /* cerrar MPI de forma ordenada antes de terminar */
        return 1;         /* código de salida distinto de 0 = "hubo un error" */
    }

    /*Armar el emparejamiento entre las dos mitades*/
    int half = nprocs / 2;
    /* Si mi rank está en la primera mitad (0..half-1), mi pareja es rank+half.
       Si estoy en la segunda mitad (half..nprocs-1), mi pareja es rank-half.
       Ej: con nprocs=8, half=4 -> parejas: (0,4) (1,5) (2,6) (3,7) */
    int partner = (rank < half) ? rank + half : rank - half;

    /*Buffers de envío y recepción: cada proceso reserva su propia memoria*/
    char *sendbuf = malloc(MSG_SIZE);  /* lo que este proceso va a mandar */
    char *recvbuf = malloc(MSG_SIZE);  /* dónde este proceso va a guardar lo que reciba */

    /*Medición del tiempo*/
    MPI_Barrier(MPI_COMM_WORLD);  /* todos arrancan el cronómetro al mismo tiempo */
    double t0 = MPI_Wtime();      /* instante inicial */

    for (int r = 0; r < NREPS; r++) {
        MPI_Request reqs[2];  /* "identificadores" para poder esperar después estas dos operaciones */

        /* Isend/Irecv = versiones NO bloqueantes: el proceso las lanza y sigue de largo
           sin esperar a que terminen. Esto es clave acá: TODOS los pares (0-4, 1-5, etc.)
           tienen que enviar y recibir a la vez, no de a uno, para medir el ancho de banda
           agregado real de la red cuando se satura con tráfico simultáneo. */
        MPI_Isend(sendbuf, MSG_SIZE, MPI_BYTE, partner, 0, MPI_COMM_WORLD, &reqs[0]);
        MPI_Irecv(recvbuf, MSG_SIZE, MPI_BYTE, partner, 0, MPI_COMM_WORLD, &reqs[1]);

        /* Waitall: recién acá el proceso se queda esperando a que las 2 operaciones lanzadas
           arriba (el reqs[0] y el reqs[1]) hayan terminado de verdad, antes de pasar a la
           siguiente repetición. MPI_STATUSES_IGNORE = no me interesa guardar info extra. */
        MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);  /* esperar a que TODOS los procesos hayan terminado sus NREPS
                                      repeticiones antes de parar el cronómetro */
    double t1 = MPI_Wtime();      /* instante final */

    /* Tiempo promedio (de las NREPS repeticiones) que le llevó a ESTE proceso en particular
       hacer su intercambio con su pareja */
    double local_time = (t1 - t0) / NREPS;

    /*Combinar los tiempos de todos los procesos en uno solo*/
    double max_time;
    /* MPI_Reduce junta un valor de cada proceso y lo combina en el proceso 0 (el último "0").
       MPI_MAX = nos quedamos con el máximo de todos los local_time: el par más lento manda,
       porque el ancho de banda agregado real está limitado por el que tardó más. */
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    /*Solo el proceso 0 calcula el resultado final e imprime*/
    if (rank == 0) {
        /* "half" pares transfieren en simultáneo, cada uno moviendo MSG_SIZE bytes:
           esa es la cantidad TOTAL de datos que se movieron en paralelo por la red */
        double total_bytes = (double)half * MSG_SIZE;

        /* ancho de banda = datos totales / tiempo que tardó el más lento, pasado a MB/s (1e6) */
        double bisection_bw_mbs = total_bytes / max_time / 1e6;

        /* Formato: n_procesos, ancho_de_banda_biseccion_MBs
           (así run_ejercicio3_barrido.sh puede juntar estas líneas en un CSV) */
        printf("%d,%.3f\n", nprocs, bisection_bw_mbs);
    }

    /*Liberar memoria y cerrar MPI*/
    free(sendbuf);
    free(recvbuf);
    MPI_Finalize();   /* última llamada de MPI, cierra todo de forma ordenada */
    return 0;
}
