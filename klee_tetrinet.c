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


#ifdef KLEE

int g_round = 0;
int g_last_round = 0;
int g_new_piece = 0;
int g_recv = 0;
//int wait_for_input_count = 0;

static int nuklear_merge() { klee_warning("nuklear_merge"); return 0; }

void klee_increment_round() {
	g_round++;
	KPRINTF("NEW ROUND");
	klee_print_expr("round = ", g_round);
}

void klee_new_piece() {
	g_new_piece = 1;
	KPRINTF("new piece");
}

int inputs[32];
int input_index = 0;
char* input_strings[6] = {"UP", "LF", "RT", "SP", "QT", "INVALID"};

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

void klee_create_inputs() {
	int rotations; // from 0 to 1
	int do_quit;
	int shifts;
	int shift_type;
	int direction;

	int i = 0;

	// TODO change to nuklear_symbolic
	klee_nuklear_make_symbolic(&do_quit, "do_quit");
	klee_nuklear_make_symbolic(&rotations,"rotations");
	klee_nuklear_make_symbolic(&shifts,  "shifts");
	klee_nuklear_make_symbolic(&shift_type, "shift_type");

	if (do_quit == 0) {
		inputs[i++] = K_F10;
	} else {

		switch (rotations) {
			case 3:
				inputs[i++] = K_UP;
			case 2:
				inputs[i++] = K_UP;
			case 1:
				inputs[i++] = K_UP;
			default:
				break;
		}

		if (shift_type == 0)
			shift_type = K_LEFT;
		else
			shift_type = K_RIGHT;

		switch (shifts) {
			case 6:
				inputs[i++] = shift_type;
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
			default:
				break;
		}

		inputs[i++] = ' ';
	}
	inputs[i++] = K_INVALID;
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

	char buf[64];
	sprintf(buf, "Round: %d Input[%d] = %s(%x)", 
		g_round, input_index-1, klee_get_input_str(retval), retval);
	KPRINTF(buf);

	if (inputs[input_index] == K_INVALID) {
		KPRINTF("last user input event");
		g_last_round = g_round;
		g_new_piece = 0;
		input_index = 0;
		memset(inputs, 0, 32);
	}
	return retval;
}

// Returns either a network socket event or a key press (symbolic).
// TODO: declare klee_fds symbolic using klee API and implement getch in klee.
int klee_wait_for_input(int msec)
{
	int c;

	//wait_for_input_count++;
	//klee_print_expr("wait_for_input count = ", wait_for_input_count);

	if (g_round > g_last_round && g_new_piece) {
	//if (g_new_piece) {
		return klee_getch();
	}

	KPRINTF("KLEE_NUKLEAR_MAKE_SYMBOLIC");
	//klee_make_symbolic(&klee_fds, sizeof(klee_fds), "klee_fds");
	
	unsigned int *ev = malloc(sizeof (unsigned int));
	klee_nuklear_make_symbolic(ev, "ev");

	//if (*ev > 2)
	//  KEXIT;

	if (*ev == 1) {
 		if (msec == -1) {
			//klee_print_expr("ev: ", *ev);
			free(ev);
 			KPRINTF("select timeout event (invalid, exiting)");
 			//klee_print_expr("WFI: ", wait_for_input_count);
 			KEXIT;
 		}
		//klee_print_expr("ev: ", *ev);
 		free(ev);
 		KPRINTF("select timeout event");
 		//klee_print_expr("WFI: ", wait_for_input_count);
 		return -2;	/* out of time */
	} 
	if (*ev == 2) {
		//klee_print_expr("ev: ", *ev);
 		free(ev);
		KPRINTF("server message event");
		//klee_print_expr("WFI: ", wait_for_input_count);
		return -1;
	}
	//switch (*klee_fds) {
	//	case 1: {
	//		free(klee_fds);
	//		if (msec == -1) {
	//			KPRINTF("select timeout event (invalid, exiting)");
	//			klee_print_expr("WFI: ", wait_for_input_count);
	//			KEXIT;
	//		}
	//		KPRINTF("select timeout event");
	//		klee_print_expr("WFI: ", wait_for_input_count);
	//		return -2;	/* out of time */
	//	} break;
	//	case 2: {
	//		free(klee_fds);
	//		KPRINTF("server message event");
	//		klee_print_expr("WFI: ", wait_for_input_count);
	//		return -1;
	//	} break;
	//	default: {
	//		free(klee_fds);
	//	} break;
	//}
	free(ev);
	//KPRINTF("klee exit");
	//klee_print_expr("WFI: ", wait_for_input_count);
	KEXIT;
}

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

int nuklear_rand() {
	static long a = 100001;
	a = (a * 125) % 2796203;
	return (a % RAND_MAX);
}

int nuklear_random() {
	return nuklear_rand();
}


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

