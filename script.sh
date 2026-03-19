#! /bin/bash

# Deshabilitar suspensión mientras corre el script
systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target 2>/dev/null


for j in {1..10}
do
for i in 1000 1500 2000 2500 3000 3500 4000 4500 5000
do
./matrix_multiply $i >> standar.doc
./matrix_multiply_cache $i >> cache.doc
./matrix_multiply_ofast $i >> ofast.doc

./matrix_multiply_threads_2 $i >> 2hilos.doc
./matrix_multiply_threads_4 $i >> 4hilos.doc
./matrix_multiply_threads_6 $i >> 6hilos.doc
./matrix_multiply_threads_8 $i >> 8hilos.doc

./matrix_multiply_process $i 2 >> 2procesos.doc
./matrix_multiply_process $i 4 >> 4procesos.doc
./matrix_multiply_process $i 6 >> 6procesos.doc
./matrix_multiply_process $i 8 >> 8procesos.doc
done
done

# Re-habilitar suspensión después de terminar
systemctl unmask sleep.target suspend.target hibernate.target hybrid-sleep.target 2>/dev/null

sudo shutdown now
