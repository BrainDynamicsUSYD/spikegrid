opencvcflags=$(shell pkg-config --cflags opencv)
opencvldflags=$(shell pkg-config --libs opencv)
${VIEWERBIN}: viewer.c
	${CC} -g  --std=gnu99 ${opencvcflags} viewer.c -o ${VIEWERBIN} ${opencvldflags}
