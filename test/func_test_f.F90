program func_test_f
include 'mpif.h'
!include 'iofw.h'

integer my_rank, comm_size

integer ierr

integer ncidp, var1

integer::dimids(2)

integer lat, lon
lat = 180
lon = 180

call MPI_Init(ierr)
call MPI_Comm_rank(MPI_COMM_WORLD, my_rank, ierr)
call MPI_Comm_size(MPI_COMM_WORLD, comm_size, ierr)

write(*, *) "rank = ", my_rank
write(*, *) "size = ", comm_size

call iofw_init(comm_size)

call iofw_nc_create(my_rank, "output/test.nc", 0, ncidp)
call iofw_nc_def_dim(my_rank, ncidp, "lat", lat, dimids(1)) 
call iofw_nc_def_dim(my_rank, ncidp, "lon", lon, dimids(2)) 

call iofw_nc_def_var(my_rank, ncidp, "time_v", NF90_FLOAT, 2, dimids, var1) 

call iofw_nc_enddef(my_rank, ncidp)
call iofw_nc_close(my_rank, ncidp)

call iofw_finalize()

call MPI_Finalize(ierr)

end program func_test_f

