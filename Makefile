CXX=g++
CXXFLAGS=-g -Wall -MMD -std=c++11
BUILDDIR="build"
EXPDIR="examples"
BUILD=${BUILDDIR}/xlang
OBJFILES=src/analyze.o src/convert.o src/error.o src/insn.o src/lex.o src/main.o\
	src/murmurhash2.o src/parser.o src/print.o src/regs.o src/symtab.o\
	src/tree.o src/x86_gen.o src/optimize.o

${BUILD} : ${OBJFILES}
	mkdir ${BUILDDIR}
	cp -r ${EXPDIR} ${BUILDDIR}
	${CXX} ${CXXFLAGS} ${OBJFILES} -o $@ 
	
src/analyze.o : src/analyze.cpp
	${CXX} -c ${CXXFLAGS} src/analyze.cpp -o $@

src/convert.o : src/convert.cpp
	${CXX} -c ${CXXFLAGS} src/convert.cpp -o $@

src/error.o : src/error.cpp
	${CXX} -c ${CXXFLAGS} src/error.cpp -o $@

src/insn.o : src/insn.cpp
	${CXX} -c ${CXXFLAGS} src/insn.cpp -o $@

src/lex.o : src/lex.cpp
	${CXX} -c ${CXXFLAGS} src/lex.cpp -o $@

src/main.o : src/main.cpp
	${CXX} -c ${CXXFLAGS} src/main.cpp -o $@

src/murmurhash.o : src/murmurhash2.cpp
	${CXX} -c ${CXXFLAGS} src/murmurhash.cpp -o $@

src/parser.o : src/parser.cpp
	${CXX} -c ${CXXFLAGS} src/parser.cpp -o $@

src/print.o : src/print.cpp
	${CXX} -c ${CXXFLAGS} src/print.cpp -o $@

src/regs.o : src/regs.cpp
	${CXX} -c ${CXXFLAGS} src/regs.cpp -o $@

src/symtab.o : src/symtab.cpp
	${CXX} -c ${CXXFLAGS} src/symtab.cpp -o $@

src/tree.o : src/tree.cpp
	${CXX} -c ${CXXFLAGS} src/tree.cpp -o $@

src/x86_gen.o : src/x86_gen.cpp
	${CXX} -c ${CXXFLAGS} src/x86_gen.cpp -o $@

src/optimize.o : src/optimize.cpp
	${CXX} -c ${CXXFLAGS} src/optimize.cpp -o $@

install:
	cp build/xlang /usr/bin/xlang
	cp man/xlang.1 /usr/share/man/man1/xlang.1

remove:
	rm /usr/bin/xlang
	rm /usr/share/man/man1/xlang.1

cleanobj:
	rm src/*.o src/*.d

clean:
	rm -r ${BUILD}
	rm -r ${BUILDDIR}/${EXPDIR}
	rmdir ${BUILDDIR}
	
