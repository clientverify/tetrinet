######## Configuration area

# Your C compiler
CC = cc
LD = cc

# The passed compilation flags
CFLAGS = -O2 -I/usr/include/ncurses -g -Wall -fno-builtin-log
CFLAGS = -O2 -I/usr/include/ncurses -g -fno-builtin-log
SERVER_CFLAGS = -lncurses

# Whether to enable IPv6 support
#IPV6 = 1

# Whether to have builtin server in the tetrinet client (available through
# -server argument) (tetrinet-server will be built always regardless this)
# BUILTIN_SERVER = 1

# If you experience random delays and server freezes when accepting new
# clients, enable this.
# NO_BRUTE_FORCE_DECRYPTION = 1

LDFLAGS = -lncurses

######## End of configuration area


OBJS = sockets.o tetrinet.o tetris.o tty.o klee_tetrinet.o

ifdef IPV6
	CFLAGS += -DHAVE_IPV6
endif
ifdef BUILTIN_SERVER
	CFLAGS += -DBUILTIN_SERVER
	OBJS += server.o
endif
ifdef NO_BRUTE_FORCE_DECRYPTION
	CFLAGS += -DNO_BRUTE_FORCE_DECRYPTION
endif
ifdef KLEE 
	CC = llvm-gcc
	#LD = /home/rac/research/gsec/local/llvm-2.7/bin/llvm-ld -disable-opt
	#LD = /home/rac/research/gsec/local/llvm-2.7/bin/llvm-ld
	LD = /playpen2/rac/gsec/local/llvm-2.7/bin/llvm-ld
	CFLAGS = -I/usr/include/ncurses -DKLEE -emit-llvm -g
	LDFLAGS = 
	SERVER_CFLAGS =
endif
ifdef KTEST
	CFLAGS += -DKTEST
	OBJS += KTest.o
endif


########

#all: tetrinet tetrinet-server
all: tetrinet 

install: all
	cp -p tetrinet tetrinet-server /usr/games

clean:
	rm -f tetrinet tetrinet-server tetrinet-ktest tetrinet.bc tetrinet-llvm tetrinet-native *.o

spotless: clean

binonly:
	rm -f *.[cho] Makefile
	rm -rf CVS/

########


tetrinet: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

tetrinet-server: server.c sockets.c tetrinet.c tetris.c klee_tetrinet.c KTest.c server.h sockets.h tetrinet.h tetris.h klee_tetrinet.h KTest.h
	$(CC) $(CFLAGS) $(SERVER_CFLAGS) -o $@ -DSERVER_ONLY server.c sockets.c tetrinet.c tetris.c klee_tetrinet.c KTest.c

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

server.o:	server.c tetrinet.h tetris.h server.h sockets.h
sockets.o:	sockets.c sockets.h tetrinet.h
tetrinet.o:	tetrinet.c tetrinet.h io.h server.h sockets.h tetris.h
tetris.o:	tetris.c tetris.h tetrinet.h io.h sockets.h
tty.o:		tty.c tetrinet.h tetris.h io.h
klee_tetrinet.o:klee_tetrinet.c klee_tetrinet.h io.h KTest.h
KTest.o:	KTest.c KTest.h

tetrinet.h:	io.h
