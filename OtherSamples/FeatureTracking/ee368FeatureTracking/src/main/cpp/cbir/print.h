#ifndef PRINT_H
#define PRINT_H

#include <stdio.h>

#ifdef SILENT
	#define DPRINT(a)
	#define DPRINTF(a)
	#define WAIT_FOR_KEY
	#define FFLUSH
#else
	#define DPRINT(a) printf(a)
	#define DPRINTF(a) printf a
	#define WAIT_FOR_KEY DPRINTF("Press enter...\n"); getchar();
	#define FFLUSH fflush(0)
#endif

#endif
