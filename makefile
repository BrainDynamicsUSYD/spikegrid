export CFLAGS=-g -O3 -Wall -Wextra -std=gnu99 -lm 
export CLIBFLAGS= -fPIC -shared
SOURCES=parameters.c coupling.c conductance.c
a.out: ${SOURCES}
	gcc ${CFLAGS} ${SOURCES}
