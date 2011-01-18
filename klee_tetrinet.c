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

void klee_increment_round() {
	g_round++;
	klee_print_expr("round = ", g_round);
}

void klee_new_piece() {
	g_new_piece = 1;
	KPRINTF("new piece");
}

// Returns either a network socket event or a key press (symbolic).
// TODO: declare klee_fds symbolic using klee API and implement getch in klee.
static int klee_wait_for_input(int msec)
{
	int c;
	int klee_fds; //symbolic
	static int wait_for_input_count = 0;

	wait_for_input_count++;
	klee_print_expr("wait_for_input count = ", wait_for_input_count);

	if (g_round > g_last_round && g_new_piece) {
		g_last_round = g_round;
		g_new_piece = 0;

		c = nuklear_getch();
		int retval = -1;

		//if (c == KEY_UP)
		//	retval = K_UP;
		//else if (c == KEY_DOWN)
		//	retval = K_DOWN;
		//else if (c == KEY_LEFT)
		//	retval = K_LEFT;
		//else if (c == KEY_RIGHT)
		//	retval = K_RIGHT;
		//else if (c == ' ')
		//	retval = ' ';

		if (c == ' ')
			retval = ' ';
		else if (c == '0'|0x80)
			retval = K_F10;
		else
			KEXIT;
		
		KPRINTF("user input event");
		return retval;
	}

	klee_make_symbolic(&klee_fds, sizeof(klee_fds), "klee_fds");

	if (klee_fds == -1) {
		if (msec == -1) {
			KPRINTF("select timeout event (invalid, exiting)");
			KEXIT;
		}
		KPRINTF("select timeout event");
		return -2;	/* out of time */
	}

	if (klee_fds == 1)  { // (FD_ISSET(server_sock, &fds))
		KPRINTF("server message event");
		return -1;
	}

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

