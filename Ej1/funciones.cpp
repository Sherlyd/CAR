#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "funciones.h"

void medir_tiempos(int rank, int p_origen, int p_destino, int tamanos[], double tiempos[], int N) {
    int REPETICIONES = 100; // Promediar elimina el ruido del SO

    for (int i = 0; i < N; i++) {
        int n = tamanos[i];
        char *buffer = (char *)malloc(n * sizeof(char));

        // 1. Envio de CALENTAMIENTO (sin medir) para despertar los buffers
        if (rank == p_origen) {
            MPI_Send(buffer, n, MPI_CHAR, p_destino, 0, MPI_COMM_WORLD);
            MPI_Recv(buffer, n, MPI_CHAR, p_destino, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (rank == p_destino) {
            MPI_Recv(buffer, n, MPI_CHAR, p_origen, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(buffer, n, MPI_CHAR, p_origen, 0, MPI_COMM_WORLD);
        }

        // 2. MEDICIÓN REAL promediada
        double t_inicio = MPI_Wtime();

        for (int r = 0; r < REPETICIONES; r++) {
            if (rank == p_origen) {
                MPI_Send(buffer, n, MPI_CHAR, p_destino, 0, MPI_COMM_WORLD);
                MPI_Recv(buffer, n, MPI_CHAR, p_destino, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (rank == p_destino) {
                MPI_Recv(buffer, n, MPI_CHAR, p_origen, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buffer, n, MPI_CHAR, p_origen, 0, MPI_COMM_WORLD);
            }
        }

        double t_fin = MPI_Wtime();

        if (rank == p_origen) {
            // Tiempo promedio por cada Ping-Pong en segundos (ida y vuelta / 2)
            tiempos[i] = ((t_fin - t_inicio) / (double)REPETICIONES) / 2.0;
        }

        free(buffer);
    }
}

void calcular_regresion_lineal(int tamanos[], double tiempos[], int N, double *latencia, double *ancho_banda) {
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for (int i = 0; i < N; i++) {
        double x = tamanos[i];
        double y = tiempos[i];

        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    double m = (N * sum_xy - sum_x * sum_y) / (N * sum_x2 - sum_x * sum_x);
    double c = (sum_y - m * sum_x) / N;

    *latencia = c;
    *ancho_banda = 1.0 / m;
}