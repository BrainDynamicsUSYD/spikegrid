include config.mk
#conductance.c always needs to be first - this ensures that the mexfile gets the right name
SOURCES= conductance.c coupling.c  STDP.c STD.c output.c evolve.c newparam.c yossarian.c init.c theta.c printstruct.c cleanup.c evolvegen.c lagstorage.c  tagged_array.c localstim.c utils.c animal.c randconns.c phi.c
BINARY=./a.out
VERSION_HASH = $(shell git rev-parse HEAD)
export CONFIG=whichparam.h config/*
export CONFIG-SUBD=$(shell pwd)/whichparam.h $(shell pwd)/config/*
export VIEWERBIN=$(shell pwd)/watch
export outlib=$(shell pwd)/out.o
export imreadlib=$(shell pwd)/imread.o
export maskgen=$(shell pwd)/mask
export tracklib=$(shell pwd)/track.o
OFILES=${imreadlib} ${outlib} ${tracklib}
.PHONY: profile clean submit docs debug params matlabparams viewer ${VIEWERBIN}  force_look TEST watch
###########
#Actually compile
###########
${BINARY}: ${SOURCES} *.h ${OFILES} ${CONFIG}
	${CC} ${CFLAGS} ${opencvcflags}     ${SOURCES} ${OFILES} -o ${BINARY} -L. ${LDFLAGS}   ${opencvldflags}
watch:
	while ! inotifywait -e modify *.c; do make;done
#manually compile the mex file.  This is actually similar to what matlab does but we get more control this way
conductance.mexa64: MATLAB = YES #sets variable for future makefiles
conductance.mexa64: CFLAGS +=   ${MATLABCFLAGS}
conductance.mexa64: CXXFLAGS += ${MATLABCFLAGS}
conductance.mexa64: LDFLAGS +=  ${MATLABLDFLAGS}
conductance.mexa64: opencvldflags =  ${matlabopencvldflags}
conductance.mexa64:  ${SOURCES} *.h whichparam.h ${OFILES}
	${CC} -fpic ${CFLAGS} ${MATLABCFLAGS} ${opencvcflags}     ${SOURCES} ${OFILES} -o conductance.mexa64 -L. ${CLIBFLAGS} ${LDFLAGS} ${opencvldflags}
#generated .c file - for speed
evolvegen.c: ${maskgen} whichparam.h config/*
	${maskgen} > evolvegen.c
#select a parameters file
whichparam.h:
	./setupparam.sh
#debug build
debug: CFLAGS = ${DEBUGFLAGS}
debug: CXXFLAGS = ${CXXDEBUGFLAGS}
debug: ${BINARY}
TEST:
	rm -rf jobtest/*
	mv whichparam.h whichparambackup.h #backup config choice
	echo -e '#include "config/parametersCANONICAL.h"' > whichparam.h
	$(MAKE) clean
	$(MAKE)
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
#yossarian
yossarian.csh: ${BINARY}
	${BINARY} -g
	chmod +x yossarian.csh
submit: yossarian.csh
	qsub yossarian.csh
clean:
	-rm -f ${BINARY}  ${maskgen}  evolvegen.c ${OFILES}
	-rm -rf html
#movie viewer
viewer: ${VIEWERBIN}
${VIEWERBIN} :
	$(MAKE) -C viewer ${VIEWERBIN}
#libs / o files / generated source
${maskgen} : force_look ${CONFIG}
	$(MAKE) -C maskgen ${maskgen}
${outlib}: force_look ${CONFIG}
	$(MAKE) -C out ${outlib}
${imreadlib}: force_look ${CONFIG}
	$(MAKE) -C imread ${imreadlib}
${tracklib}: force_look ${CONFIG}
	$(MAKE) -C CPP-tracking ${tracklib}
#cson is currently unused - but clear the variables so that CSON's makefile works
cson/libcson.a: CFLAGS=
cson/libcson.a: CXXFLAGS =
cson/libcson.a: LDFLAGS =
cson/libcson.a: force_look
	cd cson && make && cd ..
