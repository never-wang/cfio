bin_PROGRAMS = func_test func_test_f
func_test_SOURCES = func_test.c
func_test_LDADD = ../src/client/C/libiofw.a
func_test_CFLAGS = -I../src/client/C -I../src/common

func_test_f_SOURCES = func_test_f.F90
func_test_f_FCFLAGS = -I../src/client/Fortran
func_test_f_LDADD = ../src/client/C/libiofw.a
func_test_f_LIBS = -lc
