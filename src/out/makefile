${outlib}: out.cpp api.cpp outputtable.cpp
	@echo compiling output library
	@${CXX} ${CXXFLAGS} -g -fpic -Wall -Wextra  ${opencvcflags} $^ -o ${outlib} -shared
