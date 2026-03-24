systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target 2>/dev/null
for j in {1..10}
do
for i in 1000 1500 2000 2500 
do
./threaths $i >> hilos82.doc
done
done

for j in {1..10}
do
for i in 3000 3500 4000 4500 5000
do
./threaths $i >> hilos82.doc
done
done
systemctl unmask sleep.target suspend.target hibernate.target hybrid-sleep.target 2>/dev/null
