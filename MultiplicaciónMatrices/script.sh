#! /bin/bash

# Deshabilitar suspensión mientras corre el script
systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target 2>/dev/null


for j in {1..10}
do
for i in 1000 1500 2000 2500 3000 3500 4000 4500 5000
do
./matrix_multiply_openmp $i >> openmp.doc

done
done

# Re-habilitar suspensión después de terminar
systemctl unmask sleep.target suspend.target hibernate.target hybrid-sleep.target 2>/dev/null

sudo shutdown now
