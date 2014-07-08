datarootdir = ${prefix}/share

srcdir = .
top_srcdir = .

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${datarootdir}
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${datarootdir}/info
mandir = ${datarootdir}/man
includedir = ${prefix}/include

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}

CC = gcc
CPP = gcc -E
CXX = g++
CXXCPP = @CXXCPP@
RANLIB = @RANLIB@


DEFS += -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS += 
LDFLAGS += 
LIBS += -laspparser 
CXXFLAGS += $(CPPFLAGS) -g -O0 $(DEFS) #was -O2
CFLAGS += $(CPPFLAGS) -g -O0 $(DEFS) #was -O2
RM=rm -f

LBIN = bin
LDIST = dist

CXXFLAGS += -Wall -g

LIBS += -laspparser

EXES=$(LBIN)/hr $(LBIN)/cr2 $(LBIN)/crmodels2 $(LBIN)/clasp-fe

CR2OBJS=cr2.o debug.o stringutils.o fileutils.o crlist.o hash.o extended_data.o program.o loader.o model.o atom_store.o filter_crrules.o generator.o tester.o timed_computation.o

.PHONY: all install clean release-bin


all: $(EXES)

-include Makefile-local


$(LBIN)/hr: hr.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(LBIN)/hr hr.o $(LIBS)

hr.o: hr.cc
	$(CXX) $(CXXFLAGS) -c -o hr.o hr.cc

$(LBIN)/clasp-fe: clasp-fe.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(LBIN)/clasp-fe clasp-fe.cc $(LIBS)

$(LBIN)/crmodels2: crmodels2.o timed_computation.o fileutils.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(LBIN)/crmodels2 crmodels2.o timed_computation.o fileutils.o $(LDFLAGS) $(LIBS)

crmodels2.o: crmodels2.cc
	$(CXX) $(CXXFLAGS) -c -o crmodels2.o crmodels2.cc

$(LBIN)/cr2: $(CR2OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(LBIN)/cr2 $(CR2OBJS) $(LIBS)

cr2.o: cr2.cc
	$(CXX) $(CXXFLAGS) -c -o cr2.o cr2.cc

debug.o: debug.cc
	$(CXX) $(CXXFLAGS) -c -o debug.o debug.cc

stringutils.o: stringutils.cc
	$(CXX) $(CXXFLAGS) -c -o stringutils.o stringutils.cc

fileutils.o: fileutils.cc
	$(CXX) $(CXXFLAGS) -c -o fileutils.o fileutils.cc

crlist.o: crlist.cc
	$(CXX) $(CXXFLAGS) -c -o crlist.o crlist.cc

hash.o: hash.cc
	$(CXX) $(CXXFLAGS) -c -o hash.o hash.cc

extended_data.o: extended_data.cc
	$(CXX) $(CXXFLAGS) -c -o extended_data.o extended_data.cc

program.o: program.cc
	$(CXX) $(CXXFLAGS) -c -o program.o program.cc

loader.o: loader.cc
	$(CXX) $(CXXFLAGS) -c -o loader.o loader.cc

model.o: model.cc
	$(CXX) $(CXXFLAGS) -c -o model.o model.cc

atom_store.o: atom_store.cc
	$(CXX) $(CXXFLAGS) -c -o atom_store.o atom_store.cc

filter_crrules.o: filter_crrules.cc
	$(CXX) $(CXXFLAGS) -c -o filter_crrules.o filter_crrules.cc

generator.o: generator.cc
	$(CXX) $(CXXFLAGS) -c -o generator.o generator.cc

tester.o: tester.cc
	$(CXX) $(CXXFLAGS) -c -o tester.o tester.cc

timed_computation.o: timed_computation.cc
	$(CXX) $(CXXFLAGS) -c -o timed_computation.o timed_computation.cc



install: $(EXES)
	@-( umask 22 ; mkdir -p $(bindir) ; chmod 0755 $(bindir) )
	@$(INSTALL_PROGRAM) $(EXES) $(bindir)



clean:
	@$(RM) *.o
	@$(RM) $(EXES)

distclean: clean
	@$(RM) config.log config.status *config.cache Makefile
