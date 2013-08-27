export CFLAGS=-g -O3 -Wall -Wextra -std=gnu99 -lm 
export CLIBFLAGS= -fPIC -shared
SOURCES=conductance.c
a.out: ${SOURCES}
	gcc ${CFLAGS} ${SOURCES}
