export CFLAGS=-g -O0 -Wall -Wextra -std=gnu99 -lm 
export CLIBFLAGS= -fPIC -shared
SOURCES=conductance.c
a.out: ${SOURCES}
	gcc ${CFLAGS} ${SOURCES}
