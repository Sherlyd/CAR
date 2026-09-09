#ifndef FUNCIONES_H
#define FUNCIONES_H

// Mide el tiempo de comunicación entre cualquier par de procesadores
void medir_tiempos(int rank, int p_origen, int p_destino, int tamanos[], double tiempos[], int N);

// Calcula la regresión lineal para obtener latencia (l) y ancho de banda (b)
void calcular_regresion_lineal(int tamanos[], double tiempos[], int N, double *latencia, double *ancho_banda);

#endif