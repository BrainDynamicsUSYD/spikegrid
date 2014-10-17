opencvcflags=$(shell pkg-config --cflags opencv)
opencvldflags=$(shell pkg-config --libs opencv)
${CVClib}: api.cpp
	${CXX} -g -fpic -Wall -Wextra  ${opencvcflags} api.cpp -o ${CVClib} -shared ${opencvldflags}
