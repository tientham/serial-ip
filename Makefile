# Makefile - make file for serial-ip daemon
#
# Copyright (c) 02 September,2015, Tien To Minh <tien.tominh@gmail.com>
#

#OPTS = -DUSE_TCP_WRAPPERS -DUULOG_SYSLOG
OPTS = -DUULOG_SYSLOG
# Linux
CC = gcc
CFLAGS = -fPIC -g -Wall -Wno-unused $(OPTS)
LIBS = -lwrap -lnsl

# SCO OpenServer 5.x with the SCO development system
#CC = cc
#CFLAGS = -belf -Kpic -Xa -axpg4plus -w3 -g -I/usr/local/include -DOSR5 $(OPTS)
#LIBS = -lwrap -lsocket

# SCO OpenServer 5.x with gcc
#CC = gcc
#CFLAGS = -fPIC -g -Wall -Wno-unused -DOSR5 $(OPTS)
#LIBS = -lwrap -lsocket

# AIX
#CC = cc
#CFLAGS = -I/usr/local/include -DAIX $(OPTS)
#LIBS = -L/usr/local/lib -lwrap

TARGET = serial_ip
OBJ1 = serial_ip.o configuration.o raw.o network_handle.o signal_handle.o mydaemon.o 
OBJ2 = network_controller.o serial_handle.o buffer_handle.o escape_sequence_handle.o
OBJ3 = debug_handle.o systemlog_handle.o telnet.o utilities.o pidfile_handle.o
OBJS = $(OBJ1) $(OBJ2) $(OBJ3)
HDRS = Makefile serial_ip.h 

#all:	$(TARGET) $(TARGET).static

all:	$(TARGET)

$(TARGET):	$(OBJS) Makefile
	$(CC) -o $@ $(OBJS) $(LIBS)
	-chmod 755 $@

#$(TARGET).static:	$(OBJS) Makefile
#	$(CC) -o $@ $(OBJS) -Wl,-Bstatic $(LIBS) -lc
#	-chmod 755 $@

serial_ip.o:			serial_ip.c $(HDRS)
raw.o:				raw.c $(HDRS)
configuration.o:		configuration.c $(HDRS)
network_handle.o:		network_handle.c $(HDRS)
signal_handle.o:		signal_handle.c $(HDRS)
mydaemon.o:			mydaemon.c $(HDRS)
network_controller.o:		network_controller.c $(HDRS)
serial_handle.o:		serial_handle.c $(HDRS)
buffer_handle.o:		buffer_handle.c $(HDRS)
escape_sequence_handle.o:	escape_sequence_handle.c $(HDRS)
debug_handle.o:			debug_handle.c $(HDRS)
systemlog_handle.o:		systemlog_handle.c $(HDRS)
telnet.o:			telnet.c $(HDRS)
utilities.o:			utilities.c $(HDRS)
pidfile_handle.o:		pidfile_handle.c $(HDRS)

clean:
	-rm -f $(OBJS)

veryclean:
	-rm -f $(OBJS) $(TARGET) $(TARGET).static
