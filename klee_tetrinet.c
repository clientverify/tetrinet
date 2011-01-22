#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <curses.h>
#include "tetrinet.h"
#include "tetris.h"
#include "io.h"
#include "sockets.h"

#include "klee_tetrinet.h"
#include "KTest.h"

#define KTEST_FILE "tetrinet.ktest"
static FILE *klee_logfile = NULL;
char *klee_logname = "./klee.log";

int g_round = 0;
int g_last_round = 0;
int g_new_piece = 0;

#define INPUTS_LENGTH 32
unsigned int inputs[INPUTS_LENGTH];
int input_index = 0;
char* input_strings[6] = {"UP", "LF", "RT", "SP", "QT", "INVALID"};

void klee_increment_round() { g_round++; }
void klee_new_piece() { g_new_piece = 1; }

int nuklear_rand() {
	static long a = 100001;
	a = (a * 125) % 2796203;
	return (a % RAND_MAX);
}

int nuklear_random() {
	return nuklear_rand();
}

int klee_set_random_var(unsigned int *var, unsigned int max) {
	if (max == 0) return 0;
	*var = rand() % (max + 1);
}

char *klee_get_input_str(int val) {
	if (val == K_UP)
		return input_strings[0];
	if (val == K_LEFT)
		return input_strings[1];
	if (val == K_RIGHT)
		return input_strings[2];
	if (val == ' ')
		return input_strings[3];
	if (val == K_F10)
		return input_strings[4];
	return input_strings[5];
}

void klee_write_log(char* buf) {
	if (!klee_logfile)
		klee_logfile = fopen(klee_logname, "a");
	if (klee_logfile) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		fprintf(klee_logfile, "[%d.%03d] KLEE %s\n",
				(int) tv.tv_sec, (int) tv.tv_usec/1000, buf);
		fflush(klee_logfile);
	}
}

void klee_create_inputs() {
	unsigned int i=0, rotations, do_quit, shifts, shift_type;

	MAKE_SYMBOLIC(&do_quit, "do_quit", 0);
	MAKE_SYMBOLIC(&shifts,  "shifts", 5);
	MAKE_SYMBOLIC(&shift_type, "shift_type", 1);
	MAKE_SYMBOLIC(&rotations,"rotations", 2);

	if (do_quit == 1) {
		inputs[i++] = K_F10;
	} else {

		switch (rotations) {
			case 3:
				inputs[i++] = KLEE_UP;
			case 2:
				inputs[i++] = KLEE_UP;
			case 1:
				inputs[i++] = KLEE_UP;
			default:
				break;
		}

		if (shift_type == 0)
			shift_type = KLEE_LEFT;
		else
			shift_type = KLEE_RIGHT;

		switch (shifts) {
			case 5:
				inputs[i++] = shift_type;
			case 4:
				inputs[i++] = shift_type;
			case 3:
				inputs[i++] = shift_type;
			case 2:
				inputs[i++] = shift_type;
			case 1:
				inputs[i++] = shift_type;
			case 0:
				inputs[i++] = shift_type;
			default:
				break;
		}

		inputs[i++] = ' ';
	}
	inputs[i++] = K_INVALID;
	
	// Print generated inputs
	char buf[512];
	char* bufp = buf;
	int j = 0;
	
	bufp = bufp + sprintf(bufp, "Round %d input sequence : ", g_round);
	for (j = 0; j<i; j++) {
		char *input_str = klee_get_input_str(inputs[j]);
		bufp = bufp + sprintf(bufp, "%s, ", input_str);
	}
	KPRINTF(buf);
}

int klee_getch() {
	if (input_index == 0) {
		klee_create_inputs();
	}

	int retval = inputs[input_index++];

	// Print input
	char buf[64];
	sprintf(buf, "Round: %d Input[%d] = %s(%x)", 
		g_round, input_index-1, klee_get_input_str(retval), retval);
	KPRINTF(buf);

	if (inputs[input_index] == K_INVALID) {
		KPRINTF("last user input event");
		g_last_round = g_round;
		g_new_piece = 0;
		input_index = 0;
		memset(inputs, 0, INPUTS_LENGTH * sizeof(unsigned int));
	}
	return retval;
}

#ifdef KLEE

// Returns either a network socket event or a key press (symbolic).
int klee_wait_for_input(int msec)
{
	int c;

	if (g_round > g_last_round && g_new_piece) {
		return klee_getch();
	}

	KPRINTF("KLEE_NUKLEAR_MAKE_SYMBOLIC");
	
	unsigned int ev;
	klee_nuklear_make_symbolic(&ev, "ev");

	if (ev > 2)
	  KEXIT;

	if (ev == 1) {
 		if (msec == -1) {
 			KPRINTF("select timeout event (invalid, exiting)");
 			KEXIT;
 		}
 		KPRINTF("select timeout event");
 		return -2;	/* out of time */
	} 

	if (ev == 2) {
		KPRINTF("server message event");
		return -1;
	}

	KEXIT;
}

// Merge point function
static int nuklear_merge() { klee_warning("nuklear_merge"); return 0; }

// Stub functions used by stub interface.
void klee_void() { return; }
void klee_int(int i) { return; }
void klee_int_pchar(int i, const char *s) { return; }
void klee_pchar_int_int(const char *s, int i, int j) { return; }
void klee_pchar_int(const char *s, int i) { return; }

// Replaces ncurses interface with stub functions
Interface klee_interface;

void klee_init() {

	// Set symbolic input function
	klee_interface.wait_for_input = klee_wait_for_input;

	// Set stub functions
	klee_interface.screen_setup = klee_void;
	klee_interface.screen_refresh = klee_void;
	klee_interface.screen_redraw = klee_void;
	klee_interface.draw_text = klee_int_pchar;
	klee_interface.clear_text = klee_int;
	klee_interface.setup_fields = klee_void;
	klee_interface.draw_own_field = klee_void;
	klee_interface.draw_other_field = klee_int;
	klee_interface.draw_status = klee_void;
	klee_interface.draw_specials = klee_void;
	klee_interface.draw_attdef = klee_pchar_int_int;
	klee_interface.draw_gmsg_input = klee_pchar_int;
	klee_interface.clear_gmsg_input = klee_void;
	klee_interface.setup_partyline = klee_void;
	klee_interface.draw_partyline_input = klee_pchar_int;
	klee_interface.setup_winlist = klee_void;
}

#endif


#if !defined(KTEST) 

ssize_t ktest_write(int fd, const void *buf, size_t count) {
  //ssize_t num_bytes = write(fd, buf, count);
  ssize_t num_bytes = send(fd, buf, count, NULL);
  return num_bytes;
}

ssize_t ktest_read(int fd, void *buf, size_t count) {
  //ssize_t num_bytes = read(fd, buf, count);
  ssize_t num_bytes = recv(fd, buf, count, NULL);
  return num_bytes;
}

void ktest_copy(void *buf, size_t num_bytes, int name_type) {
	return;
}

void ktest_finish(int argc, char** argv) {
	return;
}

#else

KTestObject* ktest_objects = NULL;
int num_ktest_objects = -1;
int max_ktest_objects = 0;
enum { CLIENT_TO_SERVER=0, SERVER_TO_CLIENT=1 };
//char* ktest_object_names[] = { "c2s", "s2c" }; // if recording at server
char* ktest_object_names[] = { "s2c", "c2s" }; // if recording at client

static inline void ktest_check_mem() {
  if (num_ktest_objects >= max_ktest_objects) {
    max_ktest_objects = (max_ktest_objects+1)*2;
    size_t size = max_ktest_objects * sizeof(KTestObject);
    ktest_objects = (KTestObject*) realloc(ktest_objects, size);
    if (!ktest_objects) {
      fprintf(stderr, "ERROR in ktest_check_mem\n");
      exit(EXIT_FAILURE);
    }
  }
}

ssize_t ktest_write(int fd, const void *buf, size_t count) {
  int i = ++num_ktest_objects;

  //ssize_t num_bytes = write(fd, buf, count);
  ssize_t num_bytes = send(fd, buf, count, NULL);

  if (num_bytes >= 0) {
    ktest_check_mem();
    ktest_objects[i].name = ktest_object_names[SERVER_TO_CLIENT];
    ktest_objects[i].numBytes = num_bytes;
    ktest_objects[i].bytes = (unsigned char*) malloc(sizeof (unsigned char) * num_bytes);
    memcpy(ktest_objects[i].bytes, buf, num_bytes);
  } else {
    fprintf(stderr, "ERROR in ktest_write\n");
    exit(EXIT_FAILURE);
  }
  return num_bytes;
}

ssize_t ktest_read(int fd, void *buf, size_t count) {
  int i = ++num_ktest_objects;

  //ssize_t num_bytes = read(fd, buf, count);
  ssize_t num_bytes = recv(fd, buf, count, NULL);

  if (num_bytes >= 0) {
    ktest_check_mem();
    ktest_objects[i].name = ktest_object_names[CLIENT_TO_SERVER];
    ktest_objects[i].numBytes = num_bytes;
    ktest_objects[i].bytes = (unsigned char*) malloc(sizeof (unsigned char) * num_bytes);
    memcpy(ktest_objects[i].bytes, buf, num_bytes);
  } else {
    fprintf(stderr, "ERROR in ktest_read\n");
    exit(EXIT_FAILURE);
  }
  return num_bytes;
}

void ktest_copy(void *buf, size_t num_bytes, int name_type) {
  int i = ++num_ktest_objects;

  if (num_bytes >= 0) {
    ktest_check_mem();
    ktest_objects[i].name = ktest_object_names[name_type];
    ktest_objects[i].numBytes = num_bytes;
    ktest_objects[i].bytes = (unsigned char*) malloc(sizeof (unsigned char) * num_bytes);
    memcpy(ktest_objects[i].bytes, buf, num_bytes);
  } else {
    fprintf(stderr, "ERROR in ktest_read\n");
    exit(EXIT_FAILURE);
  }
}


void ktest_finish(int argc, char** argv) {
  ++num_ktest_objects;
  fprintf(stdout, "Writing KTest file.\n");
  KTest ktest;
  ktest.numArgs = argc;
  ktest.args = argv;
  ktest.symArgvs = 0;
  ktest.symArgvLen = 0;
  ktest.numObjects = num_ktest_objects;
  ktest.objects = ktest_objects;
  int i;
  for (i = 0; i<num_ktest_objects; i++) {
    printf("ktest_object[%d].name = %s : %d : %s \n",
           i, ktest_objects[i].name, 
					 ktest_objects[i].numBytes,
					 ktest_objects[i].bytes);
  }
  int result = kTest_toFile(&ktest, KTEST_FILE);
  if (!result) {
    fprintf(stderr, "ERROR in ktest_finish\n");
  }
  exit(EXIT_SUCCESS);
}
#endif

