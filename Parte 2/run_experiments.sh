#!/bin/bash
# run_experiments.sh
# Ejecuta experimentos oficiales y genera resultados en CSV
# Además genera un CSV verbose para small_trace.txt incluyendo totales

SIM=./sim
if [ ! -x "$SIM" ]; then
  echo "Ejecutable 'sim' no encontrado. Compila con: gcc -O2 -std=c11 sim.c -o sim"
  exit 1
fi

# Archivo principal (sin verbose)
OUT=results.csv
echo "trace,nframes,page_size,referencias,fallos,tasa" > $OUT

# Archivo verbose para small_trace
SMALL_OUT=small_trace_verbose.csv
echo "dv_hex,nvp,offset,hit_or_fallo,marco,df_hex" > $SMALL_OUT


run_and_parse() {
  trace=$1
  nframes=$2
  psize=$3

  out=$($SIM $nframes $psize $trace 2>&1)

  refs=$(echo "$out" | awk -F': ' '/Referencias:/ {print $2; exit}')
  faults=$(echo "$out" | awk -F': ' '/Fallos de página:/ {print $2; exit}')
  rate=$(echo "$out" | awk -F': ' '/Tasa de fallos:/ {print $2; exit}')

  if [ -z "$refs" ] || [ -z "$faults" ] || [ -z "$rate" ]; then
    echo "Error parseando salida en $trace"
    echo "$out"
    return
  fi

  echo "${trace},${nframes},${psize},${refs},${faults},${rate}" >> $OUT
  echo "Hecho: $trace nframes=$nframes psize=$psize -> refs=$refs faults=$faults rate=$rate"
}


# -------------------- Experimentos oficiales --------------------

if [ -f "trace1.txt" ]; then
  for n in 8 16 32; do
    run_and_parse trace1.txt $n 8
  done
fi

if [ -f "trace2.txt" ]; then
  for n in 8 16 32; do
    run_and_parse trace2.txt $n 4096
  done
fi


# -------------------- small_trace: verbose + totales --------------------

if [ -f "small_trace.txt" ]; then
  echo "Ejecutando small_trace.txt con --verbose..."

  # Ejecutar verbose
  verbose_out=$($SIM 8 8 --verbose small_trace.txt 2>&1)

  # Guardar línea por línea del verbose en CSV
  echo "$verbose_out" | grep -E "DV=" | while read -r line; do

    dv=$(echo "$line" | awk '{print $1}' | cut -d'=' -f2)
    nvp=$(echo "$line" | awk '{print $2}' | cut -d'=' -f2)
    offset=$(echo "$line" | awk '{print $3}' | cut -d'=' -f2)
    hof=$(echo "$line" | awk '{print $4}')
    marco=$(echo "$line" | awk '{print $5}' | cut -d'=' -f2)
    df=$(echo "$line" | awk '{print $6}' | cut -d'=' -f2)

    echo "${dv},${nvp},${offset},${hof},${marco},${df}" >> $SMALL_OUT
  done

  echo "" >> $SMALL_OUT
  echo "# Totales" >> $SMALL_OUT
  echo "trace,nframes,page_size,referencias,fallos,tasa" >> $SMALL_OUT

  # Extraer totales igual que en results.csv
  refs=$(echo "$verbose_out" | awk -F': ' '/Referencias:/ {print $2; exit}')
  faults=$(echo "$verbose_out" | awk -F': ' '/Fallos de página:/ {print $2; exit}')
  rate=$(echo "$verbose_out" | awk -F': ' '/Tasa de fallos:/ {print $2; exit}')

  echo "small_trace.txt,8,8,${refs},${faults},${rate}" >> $SMALL_OUT

  echo "small_trace_verbose.csv generado correctamente."
fi

echo "Resultados escritos en $OUT"
