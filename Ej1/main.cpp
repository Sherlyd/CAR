#include <stdio.h>
#include <mpi.h>
#include "funciones.h"

int main(int argc, char **argv) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) printf("Error: Se necesitan al menos 2 procesos en MPI.\n");
        MPI_Finalize();
        return 0;
    }

    int p_origen = 0;
    int p_destino = 1;

    // Desde 2^3 (8 B) hasta 2^25 (33554432 B) -> 23 elementos
    int N = 23;
    int tamanos[23];
    double tiempos[23];

    for (int i = 0; i < N; i++) {
        tamanos[i] = 1 << (i + 3); // 2^3 hasta 2^25
    }

    // 1. Medición de tiempos
    medir_tiempos(rank, p_origen, p_destino, tamanos, tiempos, N);

    // 2. Exportación de datos y regresión
    if (rank == p_origen) {
        double latencia_chicos, ab_chicos;
        double latencia_global, ancho_banda_global;

        // A. Exportar la tabla de datos medidos (necesario para graficar en Python)
        FILE *archivo = fopen("datos.dat", "w");
        if (archivo != NULL) {
            for (int i = 0; i < N; i++) {
                fprintf(archivo, "%d %.10f\n", tamanos[i], tiempos[i]); // Bytes y Segundos
            }
            fclose(archivo);
        }

        // B. Latencia real: Se calcula con paquetes pequeños (primeros 12 datos: 8B a 16KB)
        int N_latencia = 12;
        calcular_regresion_lineal(tamanos, tiempos, N_latencia, &latencia_chicos, &ab_chicos);

        // C. Ancho de banda global: Se calcula con todos los datos (N = 23)
        calcular_regresion_lineal(tamanos, tiempos, N, &latencia_global, &ancho_banda_global);

        // D. Corrección de latencia si la regresión global se volvió negativa por los mensajes MB
        double latencia_final = (latencia_global > 0) ? latencia_global : latencia_chicos;

        // E. Guardar parámetros de la recta para el script de Python
        FILE *arch_params = fopen("params.dat", "w");
        if (arch_params != NULL) {
            fprintf(arch_params, "%.10f %.15f\n", latencia_final, 1.0 / ancho_banda_global);
            fclose(arch_params);
        }

        // F. Salida por consola
        printf("\n=========================================\n");
        printf(" MEDICION MPI ENTRE PROCESOS P%d Y P%d\n", p_origen, p_destino);
        printf("=========================================\n");
        printf("Latencia estimada (l):       %.2f us (%.6f s)\n", latencia_final * 1e6, latencia_final);
        printf("Ancho de banda estimado (b): %.2f GB/s\n", ancho_banda_global / 1e9);
        printf("=========================================\n\n");
    }

    MPI_Finalize();
    return 0;
}