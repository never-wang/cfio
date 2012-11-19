program func_test_f

use iofw

include 'mpif.h'

integer my_rank, comm_size

integer ierr
integer proc_type

integer ncidp, var1

integer::dimids(2)

integer :: lat = 6, lon = 6
integer :: mstart(2), mcount(2)

real(8) :: fp(16)

call MPI_Init(ierr)
call MPI_Comm_rank(MPI_COMM_WORLD, my_rank, ierr)
call MPI_Comm_size(MPI_COMM_WORLD, comm_size, ierr)

write(*, *) "rank = ", my_rank
write(*, *) "size = ", comm_size

call iofw_init(3, 3, 8)

proc_type = iofw_proc_type(my_rank)

if(proc_type == IOFW_PROC_CLIENT) then
    write(*, *) "we are client"
    call iofw_create("test.nc", 0, ncidp)
    call iofw_def_dim(ncidp, "lat", lat, dimids(1)) 
    call iofw_def_dim(ncidp, "lon", lon, dimids(2)) 

    mstart(1) = mod(my_rank, 3) * (lat / 3) + 1
    mstart(2) = (my_rank / 3) * (lon /3) + 1
    mcount(1) = lat / 3
    mcount(2) = lon / 3

    write(*, *) 'start : (', mstart(1), ',', mstart(2), ')'
    
    call iofw_def_var(ncidp, "time_v", NF90_DOUBLE, 2, dimids, mstart, mcount, var1) 
    call iofw_enddef(ncidp)


    do k = 1, mcount(1) * mcount(2)
    fp(k) = k + my_rank * mcount(1) * mcount(2)
    end do

    call iofw_put_vara_double(ncidp, var1, 2, mstart, mcount, fp)

    call iofw_close(ncidp)
end if

call iofw_finalize()

call MPI_Finalize(ierr)

end program func_test_f

