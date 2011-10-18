#ifndef KLEE_TETRINET_H
#define KLEE_TETRINET_H
#include "io.h"

extern Interface klee_interface;

void klee_init();

extern int g_round;
extern int g_last_round;
extern int g_new_piece;
enum { CLIENT_TO_SERVER=0, SERVER_TO_CLIENT=1 };

#ifdef KLEE

#define IFKLEE(x) x
#define KPRINTF(x) //klee_warning(x)
#define KEXIT klee_silent_exit(1);
//#define MAKE_SYMBOLIC(x,y,z) klee_nuklear_make_symbolic(x,y)
#define MAKE_SYMBOLIC(x,y,z) klee_make_symbolic(x, sizeof(unsigned), y)
#define KLEE_MOD(x, y)  if (x > y)  klee_silent_exit(1);

#define KLEE_UP K_UP
#define KLEE_DOWN K_DOWN
#define KLEE_LEFT K_LEFT
#define KLEE_RIGHT K_RIGHT
#define KLEE_QUITKEY ('0'|0x80)

#else

#define IFKLEE(x)
#define KEXIT 

#define MAKE_SYMBOLIC(x,y,z)  klee_set_random_var(x,z)
//#define MAKE_SYMBOLIC(x,y,z)

#define KLEE_MOD(x, y) x = x % (y + 1);

#define KPRINTF(x) 
//#define KPRINTF(x) klee_write_log(x)

#define KLEE_UP KEY_UP
#define KLEE_DOWN KEY_DOWN
#define KLEE_LEFT KEY_LEFT
#define KLEE_RIGHT KEY_RIGHT
#define KLEE_QUITKEY ('0'|0x80)




#endif



#endif //KLEE_TETRINET_H

