# Grab the targets from the src directory.
COREOBJ = $(patsubst core/src/%.cxx, core/build/%.o, $(wildcard core/src/*.cxx))
DEPSOBJ = $(patsubst deps/src/drs/%.c, deps/build/%.o, $(wildcard deps/src/drs/*.c))
DEPSOBJ += $(patsubst deps/src/drs/%.cpp, deps/build/%.o, $(wildcard deps/src/drs/*.cpp))
DEPSOBJ += $(patsubst deps/src/vme/%.c, deps/build/%.o, $(wildcard deps/src/vme/*.c))
TARGETS = $(patsubst src/%.cxx, bin/%, $(wildcard src/*.cxx))

# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CXX = clang++
	CC  = clang
	FLAGS = -std=c++11 -DOS_DARWIN
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CXX = g++
	CC  = gcc
	FLAGS = -std=c++0x -DOS_LINUX
endif

ifdef DEBUG
	FLAGS += -g
else
	FLAGS += -g
endif

# DRS flags
FLAGS += -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX

# ROOT libs and flags
FLAGS += $(shell root-config --cflags)
LIBS = $(shell root-config --libs)

# wxWidgets libs and flags
WXLIBS        = $(shell wx-config --libs)
WXFLAGS       = $(shell wx-config --cxxflags)

LIBS += -lm -lzmq -ljson_spirit -lCAENDigitizer -lusb-1.0 -lutil

all: $(TARGETS) $(COREOBJ) $(DEPSOBJ) 

bin/fe_%: src/fe_%.cxx $(OBJECTS) $(OBJ_VME) $(OBJ_DRS)
	$(CXX) $< -o $@ $(FLAGS) $(WXFLAGS) \
	$(COREOBJ) $(DEPSOBJ) $(LIBS) $(WXLIBS)

bin/%_daq: modules/%_daq.cxx 
	$(CXX) $< -o $@ $(FLAGS) $(LIBS)

bin/%: src/%.cxx $(OBJECTS) $(OBJ_VME) $(OBJ_DRS)
	$(CXX) $< -o $@ $(FLAGS) $(WXFLAGS) \
	$(COREOBJ) $(DEPSOBJ) $(LIBS) $(WXLIBS)

core/build/%.o: core/src/%.cxx
	cd core; make; cd ..

deps/build/%.o: 
	cd deps; make; cd ..

clean:
	rm -f $(TARGETS)
