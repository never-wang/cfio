CC := mpiicc
INCLUDEDIR := -I/home/never/usr/include
LIBDIR := -L/home/never/usr/lib 
LIB := -lpthread -lnetcdf -lpnetcdf -lhdf5_hl -lhdf5 -lcurl -lsz 
ALLOBJECTS := $(patsubst %.c, %.o, $(wildcard *.c)) 
GREPCMD := grep -l 'int main.*(.*)' *.c
#Find the files containing main fun
EXESOURCES := $(shell $(GREPCMD))
EXEOBJECTS :=  $(patsubst %.c, %.o, $(EXESOURCES)) 

#Generate Exe file names
EXEFILES :=  $(patsubst %.c, %, $(EXESOURCES)) 

#.so files
SOOBJECTS := libiofw.so

GREPCMD := grep -L 'int main\s*(.*)' *.c
#Find the files doesn't contain main fun
NONEXESOURCES := $(shell $(GREPCMD))
NONEXEOBJECTS := $(patsubst %.c, %.o, $(NONEXESOURCES))  
HEADERS := $(shell ls | grep '\.h$$' )
CFLAGS += $(INCLUDEDIR)
CFLAGS += $(LIBDIR) 
CFLAGS += -Wall
DEBUG =
define GenExeCmd
$(CC) $(CFLAGS) -o $@ $^ $@.o $(LIB)
endef


#Must add $(EXEFILES)
all:$(ALLOBJECTS) $(EXEFILES) 
	mkdir output

#TODO just ignore #$(SOOBJECTS)
#Generate execuable file
rmcmd := rm $(EXEFILES)2>/dev/null
%.o:%.c $(HEADERS)
	$(CC) -O2 $(INCLUDEDIR) -c -Wall $(DEBUG) $< -fPIC
	$(shell $(rmcmd))
$(EXEFILES):$(NONEXEOBJECTS)
	$(GenExeCmd)
$(SOOBJECTS):$(ALLOBJECTS)
	$(CC) -O2 -shared -fPIC $(INCLUDEDIR) $(LIBDIR) $(NONEXEOBJECTS) -o $@ $(LIB)
.PHONY:clean run install
clean:
	-rm $(ALLOBJECTS) $(EXEFILES) $(SOOBJECTS)
	rm -r output
run:
	#$(shell ./startserver.sh)
#install:
#	mkdir -p $(AXIS2C_HOME)/services/zteoss
#	mkdir -p $(AXIS2C_HOME)/etc
#	cp ./libzteoss.so $(AXIS2C_HOME)/services/zteoss/libzteoss.so
#	cp ./services.xml $(AXIS2C_HOME)/services/zteoss/services.xml
#	cp ./zte_oss.config $(AXIS2C_HOME)/etc/zte_oss.config
#	ls -l $(AXIS2C_HOME)/services/zteoss/libzteoss.so
#	@echo "+-----------------------------------------------+"
#	@echo "|             Install Success                   |"
#	@echo "+-----------------------------------------------+"
#
