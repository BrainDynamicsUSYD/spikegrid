include config.mk
#define some variables
export BINARY=$(shell pwd)/a.out
export whichparam=$(shell pwd)/whichparam.h
export CONFIG=${whichparam} $(shell pwd)/config/*
export VIEWERBIN=$(shell pwd)/watch
export outlib=$(shell pwd)/lib/out.o
export imreadlib=$(shell pwd)/lib/imread.o
export maskgen=$(shell pwd)/lib/mask
export mexfile=$(shell pwd)/conductance.mexa64
export OFILES=${imreadlib} ${outlib}
.PHONY: profile clean submit docs debug viewer ${VIEWERBIN}  force_look TEST watch matlab run
###########
# The different ways to create the binary
###########
${BINARY}: force_look
	$(MAKE) -C src ${BINARY}
matlab: MATLAB=yes
matlab: ${mexfile}
${mexfile}: force_look
	$(MAKE) -C src ${mexfile}
debug: force_look
	$(MAKE) -C src debug
run: ${BINARY}
	./a.out

TEST:
	rm -rf jobtest/*
	mv whichparam.h whichparambackup.h #backup config choice
	echo -e '#include "'$(shell pwd)/'config/parametersCANONICAL.h"' > whichparam.h
	$(MAKE) evolvegen.c
	$(MAKE) -C src ${outlib}
	$(MAKE) -C src ${imreadlib}
	$(MAKE) -C src ${BINARY}
#	${CC}  ${CFLAGS} ${opencvcflags} -fno-omit-frame-pointer ${SOURCES} ${OFILES} -o ${BINARY} ${LDFLAGS}  ${opencvldflags}
	mv whichparambackup.h whichparam.h #restore config choice
	time ./a.out -n
	mv job-{0..5} jobtest
	diff -r Test_known_good jobtest
	echo "Tests passed"


#documentation
docs: html/index.html
html/index.html: ${SOURCES}  Doxyfile
		echo "Suphys computers don't have dot installed, so graphs will be missing if this was run on silliac"
	doxygen Doxyfile
#yossarian
yossarian.csh: ${BINARY}
	${BINARY} -g
	chmod +x yossarian.csh
submit: yossarian.csh
	qsub yossarian.csh
clean:
	-rm -f ${BINARY}  ${maskgen}  src/evolvegen.c ${OFILES}
	-rm -rf html
#movie viewer
viewer: ${VIEWERBIN}
${VIEWERBIN} :
	$(MAKE) -C viewer ${VIEWERBIN}

watch:
	while ! inotifywait -e modify *.c; do make;done
