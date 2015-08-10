##########
#Makefile options:
#########
#export SPEEDFLAG=-DFAST #comment out this line for double instead of float (will make code slower)
export DEFINES=-DOPENCV
##########
#set up some variables for compiling
##########

#graphite optimizations are hard on some systems - so disable them
ifneq ($(shell gcc -v 2>&1 | grep with-isl ) ,)
	optflags=-floop-interchange -floop-strip-mine -floop-block
else
	#hack to get a warning
	IGNORE := $(shell >&2 echo no ISL - some gcc optimizations have been disabled)
endif
ifeq ($(CC),clang)
	export CFLAGS= -g -Wno-padded -Wno-missing-prototypes -Wno-missing-variable-declarations -Weverything -pedantic  -Ofast -Wno-documentation-unknown-command -Wno-covered-switch-default -Wno-old-style-cast -Wno-extended-offsetof
else #gcc
	optflags:=${optflags} -Ofast -msse -msse2 -msse3 -funsafe-loop-optimizations -mtune=native -march=native  -ftree-loop-optimize   -flto  -fassociative-math -fno-signed-zeros -freciprocal-math -ffinite-math-only -fno-trapping-math -ftree-vectorize

	extrawarnings=-Wstrict-aliasing -fstrict-aliasing   -Wshadow  -Wconversion -Wdouble-promotion -Wformat=2 -Wunused -Wuninitialized -Wfloat-equal -Wunsafe-loop-optimizations -Wcast-qual -Wcast-align -Wwrite-strings  -Wlogical-op  -Wvector-operation-performance -Wno-pragmas
	extraextrawarnings=-Wsuggest-attribute=pure  -Wsuggest-attribute=noreturn -Wstrict-overflow=4
	export CFLAGS=-g -ggdb -Wall -Wextra  ${optflags} ${extrawarnings} ${extraextrawarnings} -Werror-implicit-function-declaration -fno-builtin-malloc
	cspecificwarnings= -Wjump-misses-init
endif

ifeq ($(MATLAB),yes) #this has to be an if due to errors
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
endif
#set up some non-matlab variables
export DEBUGFLAGS= -g -std=gnu11 ${DEFINES}
export CXXDEBUGFLAGS= -g -std=c++11 ${DEFINES}
export CLIBFLAGS= -fPIC -shared
export LDFLAGS= -lm -g
export opencvcflags=$(shell  pkg-config --cflags opencv)
export opencvldflags=$(shell  pkg-config --libs opencv)
CFLAGS += ${SPEEDFLAG} ${DEFINES} -fno-omit-frame-pointer
export CXXFLAGS:=${CFLAGS} #use := to create an actual copy
CFLAGS += -std=gnu11 ${cspecificwarnings}
CXXFLAGS += -std=c++11

