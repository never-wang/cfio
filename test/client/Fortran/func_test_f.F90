program func_test_f

use cfio

include 'mpif.h'

integer my_rank, comm_size

integer ierr
integer proc_type

integer ncidp, var1, var2

integer::dimids(2)

integer :: lat = 6, lon = 6
integer :: mstart(2), mcount(2)

real(8) :: fp(16)
integer(4) :: fp_i(16)

call MPI_Init(ierr)
call MPI_Comm_rank(MPI_COMM_WORLD, my_rank, ierr)
call MPI_Comm_size(MPI_COMM_WORLD, comm_size, ierr)

!write(*, *) "rank = ", my_rank
!write(*, *) "size = ", comm_size
!
ierr = cfio_init(2, 2, 4)

proc_type =  cfio_proc_type(my_rank)

if(proc_type == CFIO_PROC_CLIENT) then
    !write(*, *) "we are client"
    ierr = cfio_create("test.nc", 0, ncidp)
    ierr = cfio_def_dim(ncidp, "lat", lat, dimids(1)) 
    ierr = cfio_def_dim(ncidp, "lon", lon, dimids(2)) 

    mstart(1) = mod(my_rank, 2) * (lat / 2) + 1
    mstart(2) = (my_rank / 2) * (lon /2) + 1
    mcount(1) = lat / 2
    mcount(2) = lon / 2

    ierr = cfio_put_att(ncidp, nf90_global, "global_str", "global")
    ierr = cfio_put_att(ncidp, nf90_global, "global_int", 3)

    !write(*, *) 'start : (', mstart(1), ',', mstart(2), ')'
    
    ierr = cfio_def_var(ncidp, "time_v", NF90_DOUBLE, 2, dimids, mstart, mcount, var1) 
    ierr = cfio_def_var(ncidp, "time_i", NF90_INT, 2, dimids, mstart, mcount, var2) 
    ierr = cfio_put_att(ncidp, var1, "str", "str")
    ierr = cfio_put_att(ncidp, var1, "int", 3)
    ierr = cfio_enddef(ncidp)


    do k = 1, mcount(1) * mcount(2)
    fp(k) =  2.2
    fp_i(k) = k + my_rank * mcount(1) * mcount(2)
    end do

    ierr = cfio_put_vara(ncidp, var2, 2, mstart, mcount, fp_i)
    ierr = cfio_put_vara(ncidp, var1, 2, mstart, mcount, fp)

    ierr = cfio_close(ncidp)
end if

err = cfio_finalize()

call MPI_Finalize(ierr)

end program func_test_f

