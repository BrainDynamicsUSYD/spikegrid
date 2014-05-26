ifeq ($(CC),cc)
	optflags= -Ofast -msse -msse2 -msse3 -funsafe-loop-optimizations -mtune=native -march=native  -floop-interchange -ftree-loop-optimize -floop-strip-mine -floop-block -flto  -fassociative-math -fno-signed-zeros -freciprocal-math -ffinite-math-only -fno-trapping-math 
	extrawarnings=-Wstrict-aliasing -fstrict-aliasing   -Wshadow  -Wconversion -Wdouble-promotion -Wformat=2 -Wunused -Wuninitialized -Wfloat-equal -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wwrite-strings -Wjump-misses-init -Wlogical-op  -Wvector-operation-performance -Wno-pragmas
	extraextrawarnings=-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wstrict-overflow=5 
	export CFLAGS=-g -Wall -Wextra -std=gnu99 ${optflags} ${extrawarnings} ${extraextrawarnings}
else #clang
	export CFLAGS= -g -Wno-padded -Wno-missing-prototypes -Wno-missing-variable-declarations -Weverything -pedantic --std=gnu99 -Ofast

endif
export DEBUGFLAGS= -g -std=gnu99
#export SPEEDFLAG=-DFAST #comment out this line for double instead of float (will make code slower)
export CLIBFLAGS= -fPIC -shared
export LDFLAGS=-lm -lpng
CFLAGS += ${SPEEDFLAG}
SOURCES= conductance.c coupling.c  STDP.c STD.c picture.c output.c evolve.c ringbuffer.c newparam.c yossarian.c init.c theta.c
BINARY=./a.out
VERSION_HASH = $(shell git rev-parse HEAD)
.PHONY: profile clean submit docs debug
${BINARY}: ${SOURCES} *.h
	${CC} ${CFLAGS}     ${SOURCES} -o ${BINARY} ${LDFLAGS}
debug: ${SOURCE}
	${CC} ${DEBUGFLAGS} ${SOURCES} -o ${BINARY} ${LDFLAGS}
docs: html/index.html
html/index.html: ${SOURCES} *.h Doxyfile
	echo "Suphys computers don't have dot installed, so graphs will be missing if this was run on silliac"
	doxygen Doxyfile
profile:
	${CC} ${CFLAGS} -pg ${SOURCES} -o ${BINARY} ${LDFLAGS}
time: ${BINARY}
	echo ${VERSION_HASH} $$( (/usr/bin/time  -f '%e' 'sh' '-c' './${BINARY} > /dev/null') 2>&1) >> times
	echo ${VERSION_HASH} $$( (/usr/bin/time  -f '%e' 'sh' '-c' './${BINARY} > /dev/null') 2>&1) >> times
	echo ${VERSION_HASH} $$( (/usr/bin/time  -f '%e' 'sh' '-c' './${BINARY} > /dev/null') 2>&1) >> times
	echo ${VERSION_HASH} $$( (/usr/bin/time  -f '%e' 'sh' '-c' './${BINARY} > /dev/null') 2>&1) >> times
	echo ${VERSION_HASH} $$( (/usr/bin/time  -f '%e' 'sh' '-c' './${BINARY} > /dev/null') 2>&1) >> times
	echo ${VERSION_HASH} $$( (/usr/bin/time  -f '%e' 'sh' '-c' './${BINARY} > /dev/null') 2>&1) >> times
yossarian.csh: ${BINARY}
	${BINARY} -g
submit: yossarian.csh
	qsub yossarian.csh
clean:
	-rm -f ${BINARY}
	-rm -rf html
compile.m: makefile
	echo "mex CFLAGS=\"-fPIC -shared ${CFLAGS}  -DMATLAB  -U__STDC_UTF_16__ \" LDFLAGS=\"${LDFLAGS} -shared\" ${SOURCES}" > compile.m
