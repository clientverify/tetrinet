######## Configuration area

# Your C compiler
CC = cc
LD = cc

ifndef LLVMGCC_BIN_DIR
	LLVMGCC_BIN_DIR = ../../local/bin
endif
ifndef LLVM_BIN_DIR
	LLVM_BIN_DIR = ../../local/bin
endif
ifndef NCURSES_DIR
	NCURSES_DIR = ../../local
endif

LLVMGCC = $(LLVMGCC_BIN_DIR)/llvm-gcc
LLVM_LD = $(LLVM_BIN_DIR)/llvm-ld
NCURSES_INCLUDE = $(NCURSES_DIR)/include/ncurses
NCURSES_LIB = $(NCURSES_DIR)/lib

# The passed compilation flags
CFLAGS = -O2 -I$(NCURSES_INCLUDE) -g -fno-builtin-log

# Whether to enable IPv6 support
#IPV6 = 1

# Whether to have builtin server in the tetrinet client (available through
# -server argument) (tetrinet-server will be built always regardless this)
# BUILTIN_SERVER = 1

# If you experience random delays and server freezes when accepting new
# clients, enable this.
# NO_BRUTE_FORCE_DECRYPTION = 1

LDFLAGS = -lncurses -L$(NCURSES_LIB) 

######## End of configuration area

BASE_SRCS = sockets.c tetrinet.c tetris.c klee_tetrinet.c
SRCS = $(BASE_SRCS) tty.c 
KTEST_SRCS = $(BASE_SRCS) tty.c KTest.c
SERVER_SRCS = $(BASE_SRCS) KTest.c server.c
KLEE_SRCS = $(BASE_SRCS) tty.c

BIN_DIR = ./bin

ifndef PREFIX
	PREFIX = "/usr/local"
endif
ifdef IPV6
	CFLAGS += -DHAVE_IPV6
endif
ifdef BUILTIN_SERVER
	CFLAGS += -DBUILTIN_SERVER
	SRCS += server.c
endif
ifdef NO_BRUTE_FORCE_DECRYPTION
	CFLAGS += -DNO_BRUTE_FORCE_DECRYPTION
endif

########

TARGETS = tetrinet tetrinet-server tetrinet-ktest tetrinet-klee

all: $(TARGETS) tags

install: all
	cp -p $(BIN_DIR)/* $(PREFIX)/bin/

$(BIN_DIR):
	@mkdir $(BIN_DIR)

tags:
	ctags *.c *.h

.PHONY: all

########

OBJS_DIR = .objs
OBJS = $(addprefix $(OBJS_DIR)/,$(SRCS:.c=.o))

$(OBJS): $(OBJS_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -o $@ -c $<

$(OBJS_DIR):
	@mkdir $(OBJS_DIR)

-include $(OBJS:.o=.d)

tetrinet: $(BIN_DIR) $(OBJS_DIR) $(OBJS)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@ $(OBJS)

########

KTEST_OBJS_DIR = .ktest_objs
KTEST_OBJS = $(addprefix $(KTEST_OBJS_DIR)/,$(KTEST_SRCS:.c=.o))

$(KTEST_OBJS): $(KTEST_OBJS_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -DKTEST -o $@ -c $<

$(KTEST_OBJS_DIR):
	@mkdir $(KTEST_OBJS_DIR)

-include $(KTEST_OBJS:.o=.d)

tetrinet-ktest: $(BIN_DIR) $(KTEST_OBJS_DIR) $(KTEST_OBJS)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@ $(KTEST_OBJS)

########

SERVER_OBJS_DIR = .server_objs
SERVER_OBJS = $(addprefix $(SERVER_OBJS_DIR)/,$(SERVER_SRCS:.c=.o))

$(SERVER_OBJS): $(SERVER_OBJS_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -lncurses -DSERVER_ONLY -o $@ -c $<

$(SERVER_OBJS_DIR):
	@mkdir $(SERVER_OBJS_DIR)

-include $(SERVER_OBJS:.o=.d)

tetrinet-server: $(BIN_DIR) $(SERVER_OBJS_DIR) $(SERVER_OBJS)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@ $(SERVER_OBJS)

########

KLEE_OBJS_DIR = .klee_objs
KLEE_OBJS = $(addprefix $(KLEE_OBJS_DIR)/,$(KLEE_SRCS:.c=.o))

$(KLEE_OBJS): $(KLEE_OBJS_DIR)/%.o: %.c
	$(LLVMGCC) $(CFLAGS) -MMD -I/usr/include/ncurses -DKLEE -emit-llvm -o $@ -c $<

$(KLEE_OBJS_DIR):
	@mkdir $(KLEE_OBJS_DIR)

-include $(KLEE_OBJS:.o=.d)

tetrinet-klee: $(BIN_DIR) $(KLEE_OBJS_DIR) $(KLEE_OBJS)
	$(LLVM_LD) $(LDFLAGS) -o $(BIN_DIR)/$@ $(KLEE_OBJS)

########

clean:
	@rm -rf $(BIN_DIR) $(OBJS_DIR) $(KTEST_OBJS_DIR) $(SERVER_OBJS_DIR) $(KLEE_OBJS_DIR) tags


