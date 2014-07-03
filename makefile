ifeq ($(CC),cc)
	optflags= -Ofast -msse -msse2 -msse3 -funsafe-loop-optimizations -mtune=native -march=native  -floop-interchange -ftree-loop-optimize -floop-strip-mine -floop-block -flto  -fassociative-math -fno-signed-zeros -freciprocal-math -ffinite-math-only -fno-trapping-math 
	extrawarnings=-Wstrict-aliasing -fstrict-aliasing   -Wshadow  -Wconversion -Wdouble-promotion -Wformat=2 -Wunused -Wuninitialized -Wfloat-equal -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wwrite-strings -Wjump-misses-init -Wlogical-op  -Wvector-operation-performance -Wno-pragmas
	extraextrawarnings=-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wstrict-overflow=4 
	export CFLAGS=-g -Wall -Wextra -std=gnu99 ${optflags} ${extrawarnings} ${extraextrawarnings}
else #clang
	export CFLAGS= -g -Wno-padded -Wno-missing-prototypes -Wno-missing-variable-declarations -Weverything -pedantic --std=gnu99 -Ofast

endif
export DEBUGFLAGS= -g -std=gnu99
#export SPEEDFLAG=-DFAST #comment out this line for double instead of float (will make code slower)
export CLIBFLAGS= -fPIC -shared
export LDFLAGS=-lm -lpng -g
CFLAGS += ${SPEEDFLAG}
#conductance.c always needs to be first - this ensures that the mexfile gets the right name
SOURCES= conductance.c coupling.c  STDP.c STD.c picture.c output.c evolve.c ringbuffer.c newparam.c yossarian.c init.c theta.c printstruct.c matlab_output.c cleanup.c
BINARY=./a.out
VERSION_HASH = $(shell git rev-parse HEAD)
export VIEWERBIN=$(shell pwd)/watch
.PHONY: profile clean submit docs debug params matlabparams viewer ${VIEWERBIN}
#binary
${BINARY}: ${SOURCES} *.h
	${CC} ${CFLAGS}     ${SOURCES} -o ${BINARY} ${LDFLAGS}
debug: ${SOURCE}
	${CC} ${DEBUGFLAGS} ${SOURCES} -o ${BINARY} ${LDFLAGS}
TEST:
	rm -rf jobtest/*
	mv whichparam.h whichparambackup.h #backup config choice
	echo -e '#warning "using canonical parameters"\n#include "parametersCANONICAL.h"' > whichparam.h
	${CC} ${CFLAGS} -fno-omit-frame-pointer ${SOURCES} -o ${BINARY} ${LDFLAGS}
	mv whichparambackup.h whichparam.h #restore config choice
	time ./a.out
	mv job-{0..5} jobtest
	diff -r Test_known_good jobtest
	echo "Tests passed"
#documentation
docs: html/index.html
html/index.html: ${SOURCES} *.h Doxyfile
		echo "Suphys computers don't have dot installed, so graphs will be missing if this was run on silliac"
	doxygen Doxyfile
profile:
	${CC} ${CFLAGS} -pg ${SOURCES} -o ${BINARY} ${LDFLAGS}
#yossarian
yossarian.csh: ${BINARY}
	${BINARY} -g
	chmod +x yossarian.csh
submit: yossarian.csh
	qsub yossarian.csh
clean:
	-rm -f ${BINARY}
	-rm -rf html
#.m files
compile.m: makefile
	echo "mex CFLAGS=\"-fPIC -shared ${CFLAGS}  -DMATLAB \" LDFLAGS=\"${LDFLAGS} -shared\" ${SOURCES}" > compile.m
compileslow.m: makefile
	echo "mex CFLAGS=\"-fPIC -shared ${DEBUGFLAGS}  -DMATLAB \" LDFLAGS=\"${LDFLAGS} -shared\" ${SOURCES}" > compileslow.m
#movie viewer
viewer: ${VIEWERBIN}
${VIEWERBIN} :
	cd viewer && $(MAKE) ${VIEWERBIN}
