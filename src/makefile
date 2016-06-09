.PHONY: force_look
SOURCES= conductance.c coupling.c  STDP.c STD.c output.c evolve.c newparam.c yossarian.c init.c theta.c printstruct.c cleanup.c evolvegen.c lagstorage.c  tagged_array.c localstim.c utils.c animal.c randconns.c phi.c timing.c simplestorage.c
${BINARY}: ${SOURCES} *.h ${OFILES} ${CONFIG}
	@echo compiling main binary
	@${CC} ${CFLAGS} ${opencvcflags}     ${SOURCES} ${OFILES} -o ${BINARY} -L. ${LDFLAGS}   ${opencvldflags}

#manually compile the mex file.  This is actually similar to what matlab does but we get more control this way
${mexfile}: MATLAB = YES #sets variable for future makefiles
${mexfile}: CFLAGS +=   ${MATLABCFLAGS}
${mexfile}: CXXFLAGS += ${MATLABCFLAGS}
${mexfile}: LDFLAGS +=  ${MATLABLDFLAGS}
${mexfile}: opencvldflags =  ${matlabopencvldflags}
${mexfile}:  ${SOURCES} *.h ${CONFIG}  ${OFILES}
	${CC} -fpic ${CFLAGS} ${MATLABCFLAGS} ${opencvcflags} ${SOURCES} ${OFILES} -o ${mexfile} -L. ${CLIBFLAGS} ${LDFLAGS} ${opencvldflags}
#generated .c file - for speed
#libs / o files / generated source
${maskgen} : force_look ${CONFIG}
	$(MAKE) -C maskgen ${maskgen}
evolvegen.c: ${maskgen} ${CONFIG}
	${maskgen} > evolvegen.c
#debug build
debug: CFLAGS = ${DEBUGFLAGS}
debug: CXXFLAGS = ${CXXDEBUGFLAGS}
debug: ${BINARY}

${whichparam}:
	cd .. && ./setupparam.sh

${imreadlib}: force_look ${CONFIG}
	$(MAKE) -C imread ${imreadlib}
${outlib}: force_look ${CONFIG}
	$(MAKE) -C out ${outlib}
