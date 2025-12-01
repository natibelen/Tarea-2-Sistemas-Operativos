# Tarea-2-Sistemas-Operativos
Para compilar la implementación de la parte 1 se deben tener los archivos:

 -barrier.h
 -barrier.c
 -main_barrier.c

 Todos ubicados dentro de la misma carpeta.

Situado dentro del directorio donde están los archivos, ejecute:

 gcc -pthread -o barrier main_barrier.c barrier.c

Para ejecutar el programa:

 ./barrier

Para la parte 2 se incluyen:
- sim.c — Código del simulador
- run_experiments.sh — Script automático para ejecutar las pruebas
- trace1.txt, trace2.txt — Tracas de la tarea
- small_trace.txt — Trace pequeño para verificación (verbose)
 Requisitos

- Linux o WSL  
- Compilador GCC  
- Bash para ejecutar el script

1. Compilar el simulador:

gcc -O2 -std=c11 sim.c -o sim

2. Ejecutar el simulador manualmente
./sim <marcos> <page_size> <trace>

Ejemplo:
 ./sim 16 8 trace1.txt

Para ver el detalle de cada referencia:

 ./sim 8 8 --verbose small_trace.txt

3. Ejecutar los experimentos (script)
Primero dar permisos al script:

 chmod +x run_experiments.sh

Luego ejecutarlo:

 ./run_experiments.sh



