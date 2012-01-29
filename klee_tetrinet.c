#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <curses.h>
#include "tetrinet.h"
#include "tetris.h"
#include "io.h"
#include "sockets.h"

#include "klee_tetrinet.h"
#include "KTest.h"
#include <assert.h>

#define KTEST_FILE "tetrinet.ktest"
static FILE *klee_logfile = NULL;
char *klee_logname = "./klee.log";

int g_round = 0;
int g_last_round = 0;
int g_new_piece = 0;

static int current_enumerate_piece = 0;

#define INPUTS_LENGTH 16
unsigned int inputs[INPUTS_LENGTH];
int input_index = 0;
char* input_strings[] = {"UP", "DN", "LF", "RT", "SP", "QT", "INVALID"};

void klee_increment_round() { 
#ifndef KLEE
	g_round++;
#endif
}

int klee_new_piece() { 
	g_new_piece = 1;

	if (input_generation_type == 2) {
		return current_enumerate_piece;
	}
	return -1;
}

int nuklear_rand() {
	static long a = 100001;
	a = (a * 125) % 2796203;
	return (a % RAND_MAX);
}

int nuklear_rand2() {
	random_seed = (random_seed * 1103515245) + 12345;
	return (unsigned int)(random_seed / 65536) % 32768;
}

int nuklear_random() {
	return nuklear_rand();
}

int klee_set_random_var(unsigned int *var, unsigned int max) {
	*var = rand();
}

char *klee_get_input_str(int val) {
	if (val == KLEE_UP)
		return input_strings[0];
	if (val == KLEE_DOWN)
		return input_strings[1];
	if (val == KLEE_LEFT)
		return input_strings[2];
	if (val == KLEE_RIGHT)
		return input_strings[3];
	if (val == ' ')
		return input_strings[4];
	if (val == KLEE_QUITKEY)
		return input_strings[5];
	return input_strings[5];
}

void print_inputs() {
	// Print generated inputs
	char buf[512];
	char* bufp = buf;
	int i=0;
	
	bufp = bufp + sprintf(bufp, "Round %d input sequence : ", g_round);

	while (inputs[i] != 0xDEADBEEF) {
		char *input_str = klee_get_input_str(inputs[i]);
		bufp = bufp + sprintf(bufp, "%s, ", input_str);
		i++;
	}
	KPRINTF(buf);
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

//    Piece Types: 
//    0    1    2    3    4    5    6
//    ##X# .... .... .... .... .... ....
//    .... .X#. #X#. #X#. #X.. .X#. #X#.
//    .... .##. ..#. #... .##. ##.. .#..
//    .... .... .... .... .... .... ....
//    Rotational Positions:
//    2    1    4    4    2    2    4
//   
//    Max Moves Left in each Rotation
// 1  4    6    5    5    5    5    5   
// 2  6    .    5    5    5    5    5  
// 3  .    .    5    5    .    .    5  
// 4  .    .    6    6    .    .    6    
//
//    Max Moves Right in each Rotation
// 1  4    4    4    4    4    4    4    
// 2  5    .    5    5    5    5    5   
// 3  .    .    4    4    .    .    4   
// 4  .    .    4    4    .    .    4     

typedef struct {
	int s; // shifts
	int s_type; // shift_type
	int rot;
	int max_rot;
	int max_mov[2][4];
	int c;
} input_data_t;

input_data_t input_data[7] = {
{ .s=0, .s_type=0, .rot=0, .max_rot=2, .max_mov[0]={4,6,0,0}, .max_mov[1]={4,5,0,0}, .c=21 },
{ .s=0, .s_type=0, .rot=0, .max_rot=1, .max_mov[0]={6,0,0,0}, .max_mov[1]={4,0,0,0}, .c=11 },
{ .s=0, .s_type=0, .rot=0, .max_rot=4, .max_mov[0]={5,5,5,6}, .max_mov[1]={4,5,4,4}, .c=42 },
{ .s=0, .s_type=0, .rot=0, .max_rot=4, .max_mov[0]={5,5,5,6}, .max_mov[1]={4,5,4,4}, .c=42 },
{ .s=0, .s_type=0, .rot=0, .max_rot=2, .max_mov[0]={5,5,0,0}, .max_mov[1]={4,5,0,0}, .c=21 },
{ .s=0, .s_type=0, .rot=0, .max_rot=2, .max_mov[0]={5,5,0,0}, .max_mov[1]={4,5,0,0}, .c=21 },
{ .s=0, .s_type=0, .rot=0, .max_rot=4, .max_mov[0]={5,5,5,6}, .max_mov[1]={4,5,4,4}, .c=42 } 
};

void klee_enumerate_single_inputs() {
	int i=0, input_index=0;
	unsigned int do_quit=1;

	do_quit ^= 1;
	if (do_quit) {
		inputs[input_index++] = KLEE_QUITKEY;
		inputs[input_index++] = 0xDEADBEEF;
		return;
	}

	if (input_generation_type == 1) {
		if (!(input_data[0].c) && !(input_data[1].c) && !(input_data[2].c) && 
				!(input_data[3].c) && !(input_data[4].c) && !(input_data[5].c) && 
				!(input_data[6].c)) {
			inputs[0] = ('0'|0x80);
			inputs[1] = 0xDEADBEEF;
			inputs[2] = 0xDEADBEEF;
			return;
		}
	}

	input_data_t *cdata = &(input_data[current_piece]);

	for (i=cdata->rot; i>0; i--) {
		inputs[input_index++] = KLEE_UP;
	}

	for (i=cdata->s; i>0; i--) {
		inputs[input_index++] = cdata->s_type ? KLEE_RIGHT : KLEE_LEFT;
	}

	inputs[input_index++] = ' ';
	inputs[input_index++] = KLEE_QUITKEY;
	inputs[input_index++] = 0xDEADBEEF;

	cdata->s++;

	if (cdata->s > cdata->max_mov[cdata->s_type][cdata->rot]) {
		if (cdata->s_type == 1) {
			cdata->rot++;
		} 
		cdata->s_type ^= 1;
		cdata->s = 0;
		if (cdata->s_type == 1) {
			cdata->s++;
		}
	}

	if (cdata->rot >= cdata->max_rot) {
		//advance next piece 
		if (input_generation_type == 2) {
			if (current_enumerate_piece >= 6) {
				inputs[0] = ('0'|0x80);
				inputs[1] = 0xDEADBEEF;
			}
			current_enumerate_piece = (current_enumerate_piece + 1 ) % 7;
		} else if (input_generation_type == 1) {
			cdata->c = 0;
		}
		// reset piece data
		cdata->s = cdata->s_type = cdata->rot = 0;
	}
}

void klee_random_inputs_table() {
	unsigned int i=0, input_index=0;
	unsigned int rotations, do_quit, shifts, shift_type, do_invalid;

	MAKE_SYMBOLIC(&do_quit, "do_quit", 0);
	MAKE_SYMBOLIC(&do_invalid, "do_invalid", 1);
	MAKE_SYMBOLIC(&shifts,  "shifts", 5);
	MAKE_SYMBOLIC(&shift_type, "shift_type", 1);
	MAKE_SYMBOLIC(&rotations,"rotations", 2);

	input_data_t *cdata = &(input_data[current_piece]);

	if (do_quit) { 

		inputs[input_index++] = KLEE_QUITKEY;
	} else {
		KLEE_MOD(shift_type, 1);
		KLEE_MOD(rotations, cdata->max_rot-1);

		for (i=rotations; i>0; i--) {
			inputs[input_index++] = KLEE_UP;
		}

		unsigned int max_shift = cdata->max_mov[shift_type][rotations];

		//// If we are shifting left, don't ever have a shift distance of 0
		//if (shift_type) {
		//	max_shift--;
		//	inputs[input_index++] = KLEE_RIGHT;
		//}

		//char buf[64];
		//sprintf(buf, "Max Shift: %d", max_shift);
		//KPRINTF(buf);

		KLEE_MOD(shifts, max_shift);

		for (i=shifts; i>0; i--) {
			inputs[input_index++] = shift_type ? KLEE_RIGHT : KLEE_LEFT;
		}

		inputs[input_index++] = KLEE_DOWN;
		inputs[input_index++] = ' ';
	}

	inputs[input_index++] = 0xDEADBEEF;
}

void klee_random_inputs() {
	unsigned int i=0, rotations, do_quit, shifts, shift_type,do_invalid;

	MAKE_SYMBOLIC(&do_quit, "do_quit", 0);
	MAKE_SYMBOLIC(&do_invalid, "do_invalid", 1);
	MAKE_SYMBOLIC(&shifts,  "shifts", 5);
	MAKE_SYMBOLIC(&shift_type, "shift_type", 1);
	MAKE_SYMBOLIC(&rotations,"rotations", 2);

#ifndef KLEE
	static unsigned quit_round = 0;
	quit_round++;
	if (quit_round % 2 == 0)
		do_quit = 1;
	else
		do_quit = 0;

	if (input_generation_type != 0 && do_quit == 0) {
		inputs[i++] = KLEE_QUITKEY;
	} else {

#else

	if (do_quit == 0) {
		inputs[i++] = K_F10;
	}	else if (do_quit == 1) {
		inputs[i++] = KLEE_QUITKEY;
	} else {

#endif

		KLEE_MOD(shift_type, 1);
		if (shift_type == 0)
			shift_type = KLEE_LEFT;
		else
			shift_type = KLEE_RIGHT;

		//    Piece Types: 
		//    0    1    2    3    4    5    6
		//    ##X# .... .... .... .... .... ....
		//    .... .X#. #X#. #X#. #X.. .X#. #X#.
		//    .... .##. ..#. #... .##. ##.. .#..
		//    .... .... .... .... .... .... ....
		//    Rotational Positions:
		//    2    1    4    4    2    2    4
		//   
		//    Max Moves Left in each Rotation
		// 1  4    6    5    5    5    5    5   
		// 2  6    .    5    5    5    5    5  
		// 3  .    .    5    5    .    .    5  
		// 4  .    .    6    6    .    .    6    
		//
		//    Max Moves Right in each Rotation
		// 1  4    4    4    4    4    4    4    
		// 2  5    .    5    5    5    5    5   
		// 3  .    .    4    4    .    .    4   
		// 4  .    .    4    4    .    .    4     

		if (current_piece == 0) {
			KLEE_MOD(rotations, 1);
			if (shift_type == KLEE_LEFT) {
				if (rotations == 0) {
					KLEE_MOD(shifts, 4);
				} else {
					KLEE_MOD(shifts, 6);
				}
			} else {
				if (rotations == 0) {
					KLEE_MOD(shifts, 4);
				} else {
					KLEE_MOD(shifts, 5);
				}
			}
		}
		
		if (current_piece == 1) {
			rotations = 0;
			if (shift_type == KLEE_LEFT) {
				KLEE_MOD(shifts, 6);
			} else {
				KLEE_MOD(shifts, 4);
			}
		}
	
		if (current_piece == 2 || current_piece == 3 || current_piece == 6) {
			KLEE_MOD(rotations, 3);
			if (shift_type == KLEE_LEFT) {
				if (rotations == 3) {
					KLEE_MOD(shifts, 6);
			  } else {
					KLEE_MOD(shifts, 5);
				}
			} else {
				if (rotations == 1) {
					KLEE_MOD(shifts, 5);
				} else {
					KLEE_MOD(shifts, 4);
				}
			}
		}

		if (current_piece == 4 || current_piece == 5) {
			KLEE_MOD(rotations, 1);
			if (shift_type == KLEE_LEFT) {
				KLEE_MOD(shifts, 5);
			} else {
				if (rotations == 0) {
					KLEE_MOD(shifts, 4);
				} else {
					KLEE_MOD(shifts, 5);
				}
			}
		}

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

		//if (do_invalid)
		//	inputs[i++] = K_INVALID;

		//inputs[input_index++] = KLEE_DOWN;
		inputs[i++] = ' ';
	}
	inputs[i++] = 0xDEADBEEF;
	//print_inputs();
}

void klee_create_inputs() {

	// initialize game board with random tiles
	if (starting_height > 0) {
		Field *f = &fields[my_playernum-1];
		int x,y;
		for (y=0; y<=starting_height; y++) {
			for (x=0; x<FIELD_WIDTH; ++x) (*f)[FIELD_HEIGHT-y][x] = 1;
			(*f)[FIELD_HEIGHT-y][y % FIELD_WIDTH] = 0;
		}
	}

	switch (input_generation_type) {
#ifndef KLEE
		case 1:
		case 2:
			klee_enumerate_single_inputs();
			return;
#endif
		case 0:
		default:
			klee_random_inputs();
			return;
	}
}

int klee_getch() {
	int retval;
	if (input_index == 0) {
		klee_create_inputs();
	}

	if (inputs[input_index] == 0xDEADBEEF) {
		KPRINTF("last user input event");
		g_last_round = g_round;
		g_new_piece = 0;
		input_index = 0;
		memset(inputs, 0, INPUTS_LENGTH * sizeof(unsigned int));
		retval = 0;
	} else {
		retval = inputs[input_index++];
	}

	return retval;
}

#ifdef KLEE

// Returns either a network socket event or a key press (symbolic).
int klee_wait_for_input(int msec)
{
	if (g_new_piece) {
 		KPRINTF("user input event");
		return klee_getch();
	}

	unsigned int ev;
	MAKE_SYMBOLIC(&ev, "ev", 0);

	switch(ev) {
		case 1:
 		if (msec == -1) {
 			KPRINTF("select timeout event (invalid, exiting)");
 			KEXIT;
 		}
 		KPRINTF("select timeout event");
 		return -2;	/* out of time */

		case 2:
		KPRINTF("server message event");
		return -1;
		
		default:
	  KEXIT;
	}

	KEXIT;
}

// Merge point function
int nuklear_merge() { klee_warning("nuklear_merge"); return 0; }

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
  ssize_t num_bytes = send(fd, buf, count, 0);
  return num_bytes;
}

ssize_t ktest_read(int fd, void *buf, size_t count) {
  //ssize_t num_bytes = read(fd, buf, count);
  ssize_t num_bytes = recv(fd, buf, count, 0);
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
  ssize_t num_bytes = send(fd, buf, count, 0);

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

  ssize_t num_bytes = recv(fd, buf, count, 0);

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
  KTest ktest;
  ktest.numArgs = argc;
  ktest.args = argv;
  ktest.symArgvs = 0;
  ktest.symArgvLen = 0;
  ktest.numObjects = num_ktest_objects;
  ktest.objects = ktest_objects;
  int i;
  //fprintf(stdout, "Writing KTest file.\n");
  //for (i = 0; i<num_ktest_objects; i++) {
  //  printf("ktest_object[%d].name = %s : %d : %s \n",
  //         i, ktest_objects[i].name, 
	//				 ktest_objects[i].numBytes,
	//				 ktest_objects[i].bytes);
  //}
  int result = kTest_toFile(&ktest, ktest_filename);
  if (!result) {
    fprintf(stderr, "ERROR in ktest_finish\n");
  }
  exit(EXIT_SUCCESS);
}
#endif

