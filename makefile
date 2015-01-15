##########
#Makefile options:
#########
#export SPEEDFLAG=-DFAST #comment out this line for double instead of float (will make code slower)
export DEFINES=-DOPENCV
##########
#set up some variables for compiling
##########
ifeq ($(CC),clang)
	export CFLAGS= -g -Wno-padded -Wno-missing-prototypes -Wno-missing-variable-declarations -Weverything -pedantic  -Ofast -Wno-documentation-unknown-command -Wno-covered-switch-default
else #gcc
	optflags=  -Ofast -msse -msse2 -msse3 -funsafe-loop-optimizations -mtune=native -march=native  -floop-interchange -ftree-loop-optimize -floop-strip-mine -floop-block -flto  -fassociative-math -fno-signed-zeros -freciprocal-math -ffinite-math-only -fno-trapping-math -ftree-vectorize
	extrawarnings=-Wstrict-aliasing -fstrict-aliasing   -Wshadow  -Wconversion -Wdouble-promotion -Wformat=2 -Wunused -Wuninitialized -Wfloat-equal -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wwrite-strings  -Wlogical-op  -Wvector-operation-performance -Wno-pragmas
	extraextrawarnings=-Wsuggest-attribute=pure  -Wsuggest-attribute=noreturn -Wstrict-overflow=4
	export CFLAGS=-g -ggdb -Wall -Wextra  ${optflags} ${extrawarnings} ${extraextrawarnings}
	cspecificwarnings= -Wjump-misses-init
endif
#now when we are using matlab, everything is way too hard
#this is mainly because matlab ships its own opencv libraries.
#There are several problems with this:
# 1. The matlab version is old and doesn't have all libraries
# 2. Using any of my installed opencv libs will make the code not run.
#So there is a rather ridiculous amount of hackery here

#First find where matlab is - we need to use readlink and then dirname as matlab is almost always symlinked
export matlabdir=$(shell dirname $$(readlink -f $$(which matlab)))
#these C and ld flags are stolen from mex-v
export MATLABCFLAGS= -DMX_COMPAT_32 -D_GNU_SOURCE -DMATLAB_MEX_FILE -DMATLAB -I"${matlabdir}/../extern/include" -I"${matlabdir}/../simulink/include"
export ADAMMATLABCFLAGS= -DMX_COMPAT_32 -D_GNU_SOURCE -DMATLAB_MEX_FILE -I"${matlabdir}/../extern/include" -I"${matlabdir}/../simulink/include"
export MATLABLDFLAGS=  -L"${matlabdir}/glnxa64" -lmx -lmex -lmat -lm -lstdc++
#here some hackery - use pkg-config to learn the names of the libs but then find to only get the ones matlab has a copy of
export matlabopencvldflags=$(shell for x in $$(pkg-config --libs opencv); do  find ${matlabdir}/glnxa64/ -name $$(basename -- $$x)\*  ; done)
#have to set rpath in the linker to get the right libs to link.  Also add in the extra flags
export matlabopencvldflags:= -Wl,-rpath -Wl,${matlabdir} ${matlabopencvldflags} $(shell pkg-config --libs-only-l opencv)
#set up some non-matlab variables
export DEBUGFLAGS= -g -std=gnu11 ${DEFINES}
export CXXDEBUGFLAGS= -g --std=c++11 ${DEFINES}
export CLIBFLAGS= -fPIC -shared
export LDFLAGS= -lm -g
export opencvcflags=$(shell  pkg-config --cflags opencv)
export opencvldflags=$(shell  pkg-config --libs opencv)
CFLAGS += ${SPEEDFLAG} ${DEFINES}
export CXXFLAGS:=${CFLAGS} #use := to create an actual copy
CFLAGS += --std=gnu11 ${cspecificwarnings}
CXXFLAGS += --std=c++11
#conductance.c always needs to be first - this ensures that the mexfile gets the right name
SOURCES= conductance.c coupling.c  STDP.c STD.c output.c evolve.c newparam.c yossarian.c init.c theta.c printstruct.c cleanup.c evolvegen.c lagstorage.c gui.c tagged_array.c localstim.c utils.c animal.c randconns.c
BINARY=./a.out
VERSION_HASH = $(shell git rev-parse HEAD)
export CONFIG=whichparam.h config/*
export CONFIG-SUBD=$(shell pwd)/whichparam.h $(shell pwd)/config/*
export VIEWERBIN=$(shell pwd)/watch
export CVClib=$(shell pwd)/cv.o
export outlib=$(shell pwd)/out.o
export imreadlib=$(shell pwd)/imread.o
export maskgen=$(shell pwd)/mask

OFILES=${imreadlib} ${outlib} ${CVClib}
.PHONY: profile clean submit docs debug params matlabparams viewer ${VIEWERBIN}  force_look TEST
###########
#Actually compile
###########
${BINARY}: ${SOURCES} *.h ${OFILES} ${CONFIG}
	${CC} ${CFLAGS} ${opencvcflags}     ${SOURCES} ${OFILES} -o ${BINARY} -L. ${LDFLAGS}   ${opencvldflags}
conductance.mexa64: CFLAGS +=   ${MATLABCFLAGS}
conductance.mexa64: CXXFLAGS += ${MATLABCFLAGS}
conductance.mexa64: LDFLAGS +=  ${MATLABLDFLAGS}
conductance.mexa64: opencvldflags =  ${matlabopencvldflags}
conductance.mexa64:  ${SOURCES} *.h whichparam.h ${OFILES}
	${CC} -fpic ${CFLAGS} ${MATLABCFLAGS} ${opencvcflags}     ${SOURCES} ${OFILES} -o conductance.mexa64 -L. ${CLIBFLAGS} ${LDFLAGS} ${opencvldflags}
evolvegen.c: ${maskgen} whichparam.h config/*
	${maskgen} > evolvegen.c
ADAM: CFLAGS +=   ${ADAMMATLABCFLAGS}
ADAM: CXXFLAGS += ${MATLABCFLAGS}
ADAM: LDFLAGS +=  ${MATLABLDFLAGS}
ADAM: opencvldflags =  ${matlabopencvldflags}
ADAM: ${SOURCES} *.h ${OFILES} ${CONFIG}
	${CC} ${CFLAGS} ${opencvcflags} ${SOURCES} ${OFILES} -o ${BINARY} -L. ${LDFLAGS} ${opencvldflags}
whichparam.h:
	./setupparam.sh
debug: CFLAGS = ${DEBUGFLAGS}
debug: CXXFLAGS = ${CXXDEBUGFLAGS}
debug: ${BINARY}
mediumopt:
	${CC} -g -std=gnu99 ${SOURCES} -o ${BINARY} ${LDFLAGS}
TEST: 
	rm -rf jobtest/*
	mv whichparam.h whichparambackup.h #backup config choice
	echo -e '#include "config/parametersCANONICAL.h"' > whichparam.h
	$(MAKE) evolvegen.c
	$(MAKE) ${CVClib}
	$(MAKE) ${outlib}
	$(MAKE) ${imreadlib}
	${CC}  ${CFLAGS} ${opencvcflags} -fno-omit-frame-pointer ${SOURCES} ${OFILES} -o ${BINARY} ${LDFLAGS}  ${opencvldflags}
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
	-rm -f ${BINARY}  ${maskgen}  evolvegen.c ${OFILES}
	-rm -rf html
#.m files
compile.m: makefile
	echo "mex CFLAGS=\"-fPIC -shared ${CFLAGS} -DMATLAB \" LDFLAGS=\"${LDFLAGS} ${opencvldflags} -shared\" ${SOURCES} ${OFILES}"  > compile.m
compileslow.m: makefile
	echo "mex CFLAGS=\"-fPIC -shared ${DEBUGFLAGS}  -DMATLAB \" LDFLAGS=\"${LDFLAGS} ${opencvldflags} -shared\" ${SOURCES} ${OFILES}" > compileslow.m
#movie viewer
viewer: ${VIEWERBIN}
${VIEWERBIN} :
	$(MAKE) -C viewer ${VIEWERBIN}
#libs / o files / generated source
${CVClib} : force_look
	$(MAKE) -C openCVAPI ${CVClib}
${maskgen} : force_look ${CONFIG}
	$(MAKE) -C maskgen ${maskgen}
${outlib}: force_look ${CONFIG}
	$(MAKE) -C out ${outlib}
${imreadlib}: force_look ${CONFIG}
	$(MAKE) -C imread ${imreadlib}
