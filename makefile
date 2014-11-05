##########
#set up some variables for compiling
##########
ifeq ($(CC),clang)
	export CFLAGS= -g -Wno-padded -Wno-missing-prototypes -Wno-missing-variable-declarations -Weverything -pedantic  -Ofast -Wno-documentation-unknown-command -Wno-covered-switch-default
else #gcc
	optflags=  -Ofast -msse -msse2 -msse3 -funsafe-loop-optimizations -mtune=native -march=native  -floop-interchange -ftree-loop-optimize -floop-strip-mine -floop-block -flto  -fassociative-math -fno-signed-zeros -freciprocal-math -ffinite-math-only -fno-trapping-math -ftree-vectorize
	extrawarnings=-Wstrict-aliasing -fstrict-aliasing   -Wshadow  -Wconversion -Wdouble-promotion -Wformat=2 -Wunused -Wuninitialized -Wfloat-equal -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wwrite-strings  -Wlogical-op  -Wvector-operation-performance -Wno-pragmas 
	extraextrawarnings=-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wstrict-overflow=4
	export CFLAGS=-g -ggdb -Wall -Wextra  ${optflags} ${extrawarnings} ${extraextrawarnings}
	cspecificwarnings= -Wjump-misses-init
endif
export DEFINES=-DOPENCV
export opencvcflags=$(shell pkg-config --cflags opencv)
export opencvldflags=$(shell pkg-config --libs opencv)
export DEBUGFLAGS= -g -std=gnu11
#export SPEEDFLAG=-DFAST #comment out this line for double instead of float (will make code slower)
export CLIBFLAGS= -fPIC -shared
export LDFLAGS= -lm -g
CFLAGS += ${SPEEDFLAG} ${DEFINES}
export CXXFLAGS:=${CFLAGS} #use := to create an actual copy
CFLAGS += --std=gnu11 ${cspecificwarnings}
CXXFLAGS += --std=c++11
#conductance.c always needs to be first - this ensures that the mexfile gets the right name
SOURCES= conductance.c coupling.c  STDP.c STD.c output.c evolve.c  newparam.c yossarian.c init.c theta.c printstruct.c matlab_output.c cleanup.c evolvegen.c lagstorage.c gui.c tagged_array.c
BINARY=./a.out
VERSION_HASH = $(shell git rev-parse HEAD)
export VIEWERBIN=$(shell pwd)/watch
export CVClib=$(shell pwd)/libcv
export outlib=$(shell pwd)/out.o
export imreadlib=$(shell pwd)/imread.o
export maskgen=$(shell pwd)/mask
.PHONY: profile clean submit docs debug params matlabparams viewer ${VIEWERBIN}  force_look
###########
#Actually compile
###########
${BINARY}: ${SOURCES} *.h whichparam.h ${CVClib} ${outlib} ${imreadlib}
	${CC} ${CFLAGS} ${opencvcflags}     ${SOURCES} ${outlib} ${imreadlib} -o ${BINARY} -L. ${LDFLAGS}  -l:${CVClib}   ${opencvldflags}
evolvegen.c: ${maskgen} whichparam.h config/*
	${maskgen} > evolvegen.c
whichparam.h:
	./setupparam.sh
debug: ${SOURCE}
	${CC} ${DEBUGFLAGS} ${SOURCES} -o ${BINARY} ${LDFLAGS}
mediumopt:
	${CC} -g -std=gnu99 ${SOURCES} -o ${BINARY} ${LDFLAGS}
TEST: ${outlib} ${CVClib}
	rm -rf jobtest/*
	mv whichparam.h whichparambackup.h #backup config choice
	echo -e '#include "config/parametersCANONICAL.h"' > whichparam.h
	$(MAKE) evolvegen.c
	$(MAKE) ${CVClib}
	${CC}  ${CFLAGS} ${opencvcflags} -fno-omit-frame-pointer ${SOURCES} out.o -o ${BINARY} ${LDFLAGS} -l:${CVClib} ${opencvldflags}
	mv whichparambackup.h whichparam.h #restore config choice
	time ./a.out -n
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
	-rm -f ${BINARY} ${CVClib} ${maskgen} ${outlib} evolvegen.c
	-rm -rf html
#.m files
compile.m: makefile
	echo "mex CFLAGS=\"-fPIC -shared ${CFLAGS} -DMATLAB \" LDFLAGS=\"${LDFLAGS} -shared\" ${SOURCES}"  > compile.m
compileslow.m: makefile
	echo "mex CFLAGS=\"-fPIC -shared ${DEBUGFLAGS}  -DMATLAB \" LDFLAGS=\"${LDFLAGS} -shared\" ${SOURCES}" > compileslow.m
#movie viewer
viewer: ${VIEWERBIN}
${VIEWERBIN} :
	$(MAKE) -C viewer ${VIEWERBIN}
	#libs
${CVClib} : force_look
	$(MAKE) -C openCVAPI ${CVClib}
${maskgen} : force_look
	$(MAKE) -C maskgen ${maskgen}
${outlib}: force_look
	$(MAKE) -C out ${outlib}
${imreadlib}: force_look
	$(MAKE) -C imread ${imreadlib}
