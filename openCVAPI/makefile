opencvcflags=$(shell pkg-config --cflags opencv)
opencvldflags=$(shell pkg-config --libs opencv)
${CVClib}: api.cpp
	g++ -g -fpic  ${opencvcflags} api.cpp -o ${CVClib} -shared ${opencvldflags}
