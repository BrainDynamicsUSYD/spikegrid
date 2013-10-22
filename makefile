optflags= -O3
export CFLAGS=-g -Wall -Wextra -std=gnu99 ${optflags} 
#-Wstrict-aliasing -fstrict-aliasing -Wstrict-overflow=5 -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wshadow -Wconversion
export CLIBFLAGS= -fPIC -shared
export LDFLAGS=-lm -lpng
SOURCES= coupling.c  STDP.c conductance.c STD.c movie.c output.c
BINARY=a.out
a.out: ${SOURCES}
	${CC}  ${CFLAGS} ${SOURCES} -o ${BINARY} ${LDFLAGS}
clean:
	rm ${BINARY}
