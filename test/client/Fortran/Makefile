PKGDIR = /home/never/cfio
INCLUDEDIR = -I$(PKGDIR)/include -I/home/never/esm-soft//include
LIBDIR = -L$(PKGDIR)/lib -L/home/never/esm-soft/lib
LIB = -lcfio -lpthread -lnetcdf -lpnetcdf -lhdf5_hl -lhdf5 -lcurl -lsz

func_test_f : func_test_f.F90 cfio.F90
	mpiifort $(INCLUDEDIR) $(LIBDIR) -o func_test_f func_test_f.F90 cfio.F90 $(LIB)

clean :
	rm func_test_f
