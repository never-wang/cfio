PKGDIR = ../../../src/client
INCLUDEDIR = -I$(PKGDIR)/Fortran -I/home/never/esm-soft//include
LIBDIR = -L$(PKGDIR)/C -L/home/never/esm-soft/lib
LIB = -liofw -lpthread -lnetcdf -lpnetcdf -lhdf5_hl -lhdf5 -lcurl -lsz

func_test_f : func_test_f.F90
	mpiifort $(INCLUDEDIR) $(LIBDIR) -o func_test_f func_test_f.F90 $(LIB)
