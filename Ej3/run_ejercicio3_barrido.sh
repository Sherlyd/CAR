#!/bin/bash
# Corre el ejercicio 3 con una cantidad creciente de procesos (todos pares)
# y junta los resultados en resultados_biseccion.csv
#
# Uso en el cluster (con SLURM):
#   ./run_ejercicio3_barrido.sh cluster
#
# Uso local en Ubuntu (con mpirun directo, sin SLURM):
#   ./run_ejercicio3_barrido.sh local

MODO=${1:-local}
SALIDA=resultados_biseccion.csv
echo "n_procesos,ancho_banda_biseccion_MBs" > "$SALIDA"

for N in 2 4 6 8 10 12; do
    if [ "$MODO" = "cluster" ]; then
        # Envía el job y espera a que termine antes de seguir con el próximo N
        JOBOUT=$(sbatch --wait --ntasks=$N --output=ej3-n${N}-%j.txt ejercicio3_biseccion.sbatch)
        JOBID=$(echo "$JOBOUT" | awk '{print $4}')
        RESULTLINE=$(grep -h "^$N," ej3-n${N}-${JOBID}.txt)
    else
        RESULTLINE=$(mpirun -np $N --oversubscribe ./ejercicio3_biseccion)
    fi
    echo "$RESULTLINE" >> "$SALIDA"
    echo "n=$N -> $RESULTLINE"
done

echo "Resultados guardados en $SALIDA"
