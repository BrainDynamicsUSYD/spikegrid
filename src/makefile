.PHONY: force_look

SOURCES= conductance.c coupling.c  STDP.c STD.c output.c evolve.c newparam.c yossarian.c init.c theta.c printstruct.c cleanup.c evolvegen.c lagstorage.c  tagged_array.c localstim.c utils.c animal.c randconns.c phi.c timing.c simplestorage.c

${BINARY}: ${SOURCES} *.h ${OFILES} ${CONFIG}
	@echo compiling main binary
	@${CC} ${CFLAGS} ${opencvcflags}     ${SOURCES} ${OFILES} -o ${BINARY} -L. ${LDFLAGS}   ${opencvldflags}

#manually compile the mex file.  This is actually similar to what matlab does but we get more control this way
#these C and ld flags are stolen from mex-v
#here some hackery - use pkg-config to learn the names of the libs but then find to only get the ones matlab has a copy of
#have to set rpath in the linker to get the right libs to link.  Also add in the extra flags
${mexfile}:matlabdir=$(shell dirname $$(readlink -f $$(which matlab)))
${mexfile}: MATLABCFLAGS= -DMX_COMPAT_32 -D_GNU_SOURCE -DMATLAB_MEX_FILE -DMATLAB -I"${matlabdir}/../extern/include" -I"${matlabdir}/../simulink/include"
${mexfile}: MATLABLDFLAGS=  -L"${matlabdir}/glnxa64" -lmx -lmex -lmat -lm -lstdc++
${mexfile}: matlabopencvldflags=$(shell for x in $$(pkg-config --libs opencv); do  find ${matlabdir}/glnxa64/ -name $$(basename -- $$x)\*  ; done)
${mexfile}: matlabopencvldflags:= -Wl,-rpath -Wl,${matlabdir} ${matlabopencvldflags} $(shell pkg-config --libs-only-l opencv)
${mexfile}: CFLAGS +=   ${MATLABCFLAGS}
${mexfile}: CXXFLAGS += ${MATLABCFLAGS}
${mexfile}: LDFLAGS +=  ${MATLABLDFLAGS}
${mexfile}: opencvldflags =  ${matlabopencvldflags}
${mexfile}:  ${SOURCES} *.h ${CONFIG}  ${OFILES}
	echo ${MATLABCFLAGS}
	echo ${MATLABLDFLAGS}
	${CC} -fpic ${CFLAGS} ${opencvcflags} ${SOURCES} ${OFILES} -o ${mexfile} -L. ${CLIBFLAGS} ${LDFLAGS} ${opencvldflags}
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
