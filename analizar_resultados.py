import csv
import numpy as np
import matplotlib.pyplot as plt

# ---------- Ejercicio 2: matriz de transferencia ----------
# try:
#     matriz = np.loadtxt("matriz_transferencia.csv", delimiter=",")
#     fig, ax = plt.subplots(figsize=(6, 5))
#     im = ax.imshow(matriz, cmap="viridis")
#     ax.set_title("Matriz de transferencia (MB/s)")
#     ax.set_xlabel("Proceso destino")
#     ax.set_ylabel("Proceso origen")
#     fig.colorbar(im, label="MB/s")
#     fig.tight_layout()
#     fig.savefig("matriz_transferencia.png", dpi=150)
#     print("Guardado matriz_transferencia.png")
#     print("Media:", matriz[matriz > 0].mean(), "Desvío std:", matriz[matriz > 0].std())
# except OSError:
#     print("No se encontró matriz_transferencia.csv (correr primero el ejercicio 2)")

# ---------- Ejercicio 3: ancho de banda de bisección ----------
try:
    ns, bws = [], []
    with open("resultados_biseccion.csv") as f:
        reader = csv.DictReader(f)
        for row in reader:
            ns.append(int(row["n_procesos"]))
            bws.append(float(row["ancho_banda_biseccion_MBs"]))

    fig, ax = plt.subplots(figsize=(6, 5))
    ax.plot(ns, bws, "o-", label="Medido")
    # referencia lineal ideal, escalada al primer punto
    if ns[0] > 0:
        ideal = [bws[0] * (n / ns[0]) for n in ns]
        ax.plot(ns, ideal, "--", label="Lineal ideal (referencia)")
    ax.set_xlabel("n (cantidad de procesos)")
    ax.set_ylabel("Ancho de banda de bisección (MB/s)")
    ax.set_title("Ancho de banda de bisección vs. n")
    ax.legend()
    fig.tight_layout()
    fig.savefig("biseccion_vs_n.png", dpi=150)
    print("Guardado biseccion_vs_n.png")
except OSError:
    print("No se encontró resultados_biseccion.csv (correr primero el barrido del ejercicio 3)")
