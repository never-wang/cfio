bin_PROGRAMS = func_test func_test_f perform_test perform_test_pnetcdf perform_test_netcdf
func_test_SOURCES = func_test.c
func_test_LDADD = ../src/client/C/libiofw.a
func_test_CFLAGS = -I../src/client/C -I../src/common

func_test_f_SOURCES = func_test_f.F90
func_test_f_FCFLAGS = -I../src/client/Fortran
func_test_f_LDADD = ../src/client/C/libiofw.a
func_test_f_LIBS = -lc

perform_test_SOURCES = perform_test.c
perform_test_LDADD = ../src/client/C/libiofw.a
perform_test_CFLAGS = -I../src/client/C -I../src/common

perform_test_pnetcdf_SOURCES = perform_test_pnetcdf.c
perform_test_pnetcdf_LDADD = ../src/client/C/libiofw.a
perform_test_pnetcdf_CFLAGS = -I../src/client/C -I../src/common

perform_test_netcdf_SOURCES = perform_test_netcdf.c
perform_test_netcdf_LDADD = ../src/client/C/libiofw.a
perform_test_netcdf_CFLAGS = -I../src/client/C -I../src/common
