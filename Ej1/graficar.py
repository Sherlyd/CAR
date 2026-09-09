import matplotlib.pyplot as plt
import numpy as np

# 1. Cargar los datos guardados por C++
datos = np.loadtxt('datos.dat')
bytes_msg = datos[:, 0]
tiempos_s = datos[:, 1]

params = np.loadtxt('params.dat')
latencia_s = params[0]
pendiente_m = params[1]

# Configuración de la figura
plt.figure(figsize=(9, 5.5))

# Puntos medidos
plt.plot(bytes_msg, tiempos_s, 'o', color='#1f77b4', label='Datos medidos', markersize=6)

# Recta de regresión
x_recta = np.linspace(0, max(bytes_msg), 100)
y_recta = latencia_s + pendiente_m * x_recta

# Etiqueta con la ecuación en notación científica como la de la imagen
label_recta = f'y = {pendiente_m:.1E}x + {latencia_s:.4f}'
plt.plot(x_recta, y_recta, 'k-', label=label_recta, linewidth=1.5)

# Formato visual igual al Excel del ejemplo
plt.title('Tiempo vs Bytes', fontsize=12)
plt.xlabel('Bytes', fontsize=11)
plt.ylabel('Tiempo(s)', fontsize=11)

# QUITAR NOTACIÓN CIENTÍFICA EN EL EJE X (Igual al gráfico de referencia)
plt.ticklabel_format(style='plain', axis='x')

# Límites y grilla limpia
plt.xlim(-1000000, 36000000)
plt.grid(True, linestyle='-', alpha=0.3)
plt.legend(loc='upper left', fontsize=10)

plt.savefig('grafico_red.png', dpi=300, bbox_inches='tight')
print("Gráfico generado como 'grafico_red.png'")