#!/bin/bash
#SBATCH --job-name=ej3_biseccion
#SBATCH --ntasks=8                  # DEBE ser par; se sobreescribe con --ntasks=N al enviar
#SBATCH --ntasks-per-node=1
#SBATCH --output=ej3-%j-salida.txt
#SBATCH --error=ej3-%j-error.txt
#SBATCH --partition=debug

mpirun ./ejercicio3_biseccion
