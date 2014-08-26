opencvcflags=$(shell pkg-config --cflags opencv)
opencvldflags=$(shell pkg-config --libs opencv)
${CVClib}: api.c
	g++ -g -fpic  ${opencvcflags} api.cpp -o ${CVClib} -shared ${opencvldflags}
