for((i = 1; i <= 32; i = i * 2)); do
    bsub -a intelmpi -o origin-$i.output -n $i mpirun.lsf /home/huangxm/WORK/io-forward/perform_test_origin
done

for((i = 2; i <= 64; i = i * 2)); do
    bsub -a intelmpi -o $i.output -n $i mpirun.lsf /home/huangxm/WORK/io-forward/perform_test
done
