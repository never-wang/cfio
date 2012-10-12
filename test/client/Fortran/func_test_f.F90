program func_test_f

use iofw

include 'mpif.h'

integer my_rank, comm_size

integer ierr

integer ncidp, var1

integer::dimids(2)

integer :: lat = 8, lon = 8
integer :: mstart(2), mcount(2)

real(8) :: fp(16)

call MPI_Init(ierr)
call MPI_Comm_rank(MPI_COMM_WORLD, my_rank, ierr)
call MPI_Comm_size(MPI_COMM_WORLD, comm_size, ierr)

write(*, *) "rank = ", my_rank
write(*, *) "size = ", comm_size

call iofw_init(comm_size)

call iofw_create(my_rank, "output/test.nc", 0, ncidp)
call iofw_def_dim(my_rank, ncidp, "lat", lat, dimids(1)) 
call iofw_def_dim(my_rank, ncidp, "lon", lon, dimids(2)) 

call iofw_def_var(my_rank, ncidp, "time_v", NF90_DOUBLE, 2, dimids, var1) 

call iofw_enddef(my_rank, ncidp)

mstart(1) = mod(my_rank, 2) * (lat / 2) + 1
mstart(2) = (my_rank / 2) * (lon /2) + 1
mcount(1) = lat / 2
mcount(2) = lon / 2

write(*, *) 'start : (', mstart(1), ',', mstart(2), ')'

do k = 1, mcount(1) * mcount(2)
fp(k) = k + my_rank * mcount(1) * mcount(2)
end do

call iofw_put_vara_double(my_rank, ncidp, var1, 2, mstart, mcount, fp)

call iofw_close(my_rank, ncidp)

call iofw_finalize()

call MPI_Finalize(ierr)

end program func_test_f

