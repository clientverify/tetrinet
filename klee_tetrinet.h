#ifndef KLEE_TETRINET_H
#define KLEE_TETRINET_H
#include "io.h"

extern Interface klee_interface;

void klee_init();

#ifdef KLEE

#define IFKLEE(x) x
#define KPRINTF(x) klee_warning(x)
#define KEXIT klee_silent_exit(1);

#else

#define IFKLEE(x)
#define KPRINTF(x) 
#define KEXIT 

#endif



#endif //KLEE_TETRINET_H

