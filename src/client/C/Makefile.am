common_dir = ../../common
common = $(common_dir)/buffer.c	$(common_dir)/buffer.h 	    $(common_dir)/debug.c 	\
	 $(common_dir)/debug.h 	$(common_dir)/define.h 	    $(common_dir)/id.c  	\
	 $(common_dir)/id.h  	$(common_dir)/iofw_error.h  $(common_dir)/iofw_types.h  \
	 $(common_dir)/map.c  	$(common_dir)/map.h  	    $(common_dir)/msg.c  	\
	 $(common_dir)/msg.h  	$(common_dir)/quickhash.h   $(common_dir)/quicklist.h  	\
	 $(common_dir)/times.c  $(common_dir)/times.h

server_dir = ../../server
server = $(server_dir)/io.c $(server_dir)/io.h  \
	 $(server_dir)/server.c  $(server_dir)/server.h

lib_LIBRARIES = libiofw.a
libiofw_a_SOURCES = iofw.h iofw.c \
		    $(common) $(server)
libiofw_a_CFLAGS = -I$(common_dir)

include_HEADERS = iofw.h


