export CFLAGS=-g -O3 -Wall -Wextra -std=gnu99
export CLIBFLAGS= -fPIC -shared
export LDFLAGS=-lm -lpng
SOURCES=parameters.c coupling.c  STDP.c conductance.c STD.c movie.c output.c
BINARY=a.out
a.out: ${SOURCES}
	${CC}  ${CFLAGS} ${SOURCES} -o ${BINARY} ${LDFLAGS}
clean:
	rm ${BINARY}
