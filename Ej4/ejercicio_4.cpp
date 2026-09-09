#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define FILENAME "homo_sapiens_chromosome_1.fasta"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char* full_sequence = NULL;
    long long total_bases = 0;

    // Solo el maestro (rank 0) lee y parsea el archivo FASTA
    if (rank == 0) {
        FILE* file = fopen(FILENAME, "r");
        if (!file) {
            fprintf(stderr, "Error: No se pudo abrir el archivo %s\n", FILENAME);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Obtener tamaño del archivo para reservar un buffer preliminar
        fseek(file, 0, SEEK_END);
        long long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        full_sequence = (char*)malloc(file_size);
        if (!full_sequence) {
            fprintf(stderr, "Error: Memoria insuficiente para cargar el archivo.\n");
            fclose(file);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        char line[2048];
        // Leer línea a línea filtrando encabezados y saltos de línea
        while (fgets(line, sizeof(line), file)) {
            if (line[0] == '>') continue; // Ignorar encabezado FASTA

            for (int i = 0; line[i] != '\0'; i++) {
                char c = toupper((unsigned char)line[i]);
                if (c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N') {
                    full_sequence[total_bases++] = c;
                }
            }
        }
        fclose(file);
        printf("[Master] Archivo cargado. Total de bases biologicas: %lld\n", total_bases);
    }

    // Difundir la longitud total a todos los procesos
    MPI_Bcast(&total_bases, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    // Configuración para el particionado (Scatterv)
    int* sendcounts = (int*)malloc(size * sizeof(int));
    int* displs = (int*)malloc(size * sizeof(int));

    long long base_chunk = total_bases / size;
    long long remainder = total_bases % size;
    long long offset = 0;

    for (int i = 0; i < size; i++) {
        long long count = base_chunk + (i < remainder ? 1 : 0);
        sendcounts[i] = (int)count;
        displs[i] = (int)offset;
        offset += count;
    }

    // Buffer local para cada proceso
    int local_count = sendcounts[rank];
    char* local_buffer = (char*)malloc(local_count);
    if (!local_buffer && local_count > 0) {
        fprintf(stderr, "[P%d] Error de memoria en buffer local.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Sincronizar y medir tiempo de distribución y cómputo
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    // Distribuir las porciones a los n procesos
    MPI_Scatterv(full_sequence, sendcounts, displs, MPI_CHAR,
                 local_buffer, local_count, MPI_CHAR,
                 0, MPI_COMM_WORLD);

    // Conteo local de la base Adenina (A)
    long long local_a_count = 0;
    for (int i = 0; i < local_count; i++) {
        if (local_buffer[i] == 'A') {
            local_a_count++;
        }
    }

    // Cada nodo reporta de forma individual su conteo hacia el maestro
    long long* all_counts = NULL;
    if (rank == 0) {
        all_counts = (long long*)malloc(size * sizeof(long long));
    }

    MPI_Gather(&local_a_count, 1, MPI_LONG_LONG,
               all_counts, 1, MPI_LONG_LONG,
               0, MPI_COMM_WORLD);

    // Reducción global para sumar el total de Adeninas
    long long global_a_count = 0;
    MPI_Reduce(&local_a_count, &global_a_count, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    double t_end = MPI_Wtime();

    // Reporte final por el nodo maestro
    if (rank == 0) {
        printf("\n=== Conteo por Proceso ===\n");
        for (int i = 0; i < size; i++) {
            printf("Proceso P%d: %lld bases A (de un fragmento de %d bases)\n",
                   i, all_counts[i], sendcounts[i]);
        }

        double porcentaje_a = ((double)global_a_count / (double)total_bases) * 100.0;

        printf("\n=== Resultados Finales ===\n");
        printf("Total de Adeninas (A): %lld\n", global_a_count);
        printf("Total de Bases:        %lld\n", total_bases);
        printf("Porcentaje de A:       %.4f %%\n", porcentaje_a);
        printf("Tiempo transcurrido:   %.4f segundos\n", t_end - t_start);

        free(full_sequence);
        free(all_counts);
    }

    free(local_buffer);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}
