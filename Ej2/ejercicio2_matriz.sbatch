#!/bin/bash
#SBATCH --job-name=ej2_matriz
#SBATCH --ntasks=10                 # cantidad de procesos a comparar (ajustar)
#SBATCH --ntasks-per-node=1         # 1 proceso por nodo, para medir la red real
#SBATCH --output=ej2-%j-salida.txt
#SBATCH --error=ej2-%j-error.txt
#SBATCH --partition=debug           # cambiar a "internos" o "externos" según corresponda

mpirun ./ejercicio2_matriz
