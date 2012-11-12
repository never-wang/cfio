LDADD = ../../../src/client/C/libiofw.la
AM_CFLAGS = -I../../../src/client/C -I../../../src/common

bin_PROGRAMS = func_test perform_test_pnetcdf perform_test_netcdf perform_test
func_test_SOURCES = func_test.c test_def.h
perform_test_SOURCES = perform_test.c

#func_test_f_SOURCES = func_test_f.F90
#func_test_f_FCFLAGS = -I../src/client/Fortran
#func_test_f_LDADD = ../src/client/C/libiofw.a
#func_test_f_LIBS = -lc


perform_test_pnetcdf_SOURCES = perform_test_pnetcdf.c test_def.h

perform_test_netcdf_SOURCES = perform_test_netcdf.c test_def.h
