# Grab the targets and sources as two batches
OBJECTS = $(patsubst src%.cxx, build%.o, $(wildcard src/*.cxx))
OBJ_VME = $(patsubst include/vme%.c, build%.o, $(wildcard include/vme/*.c))
OBJ_DRS = $(patsubst src/drs%.cpp, build%.o, $(wildcard src/drs/*.cpp))
OBJ_DRS += $(patsubst src/drs%.c, build%.o, $(wildcard src/drs/*.c))
TARGETS = $(patsubst modules/%.cxx, %, $(wildcard modules/*.cxx))

# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CXX = clang++
	CC  = clang
	FLAGS = -std=c++11
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CXX = g++
	CC  = gcc
	FLAGS = -std=c++0x
endif

ifdef DEBUG
	FLAGS += -g
else
	FLAGS += -g
endif


# ROOT libs and flags
FLAGS += $(shell root-config --cflags)
LIBS = $(shell root-config --libs)

# wxWidgets libs and flags
WXLIBS        = $(shell wx-config --libs)
WXFLAGS       = $(shell wx-config --cxxflags)

FLAGS += -Iinclude -Iinclude/drs
LIBS += -lm -lzmq -ljson_spirit -lCAENDigitizer

all: $(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(TARGETS) build

fe_%: modules/fe_%.cxx $(OBJECTS) $(OBJ_VME) $(OBJ_DRS)
	$(CXX) $< -o $@  $(FLAGS) $(OBJECTS) $(OBJ_VME) $(LIBS) $(WXLIBS)

%_daq: modules/%_daq.cxx 
	$(CXX) $< -o $@  $(FLAGS) $(LIBS)

build:
	mkdir build

build/%.o: src/%.cxx
	$(CXX) -c $< -o $@ $(FLAGS)

build/%.o: include/vme/%.c
	$(CC) -c $< -o $@

build/%.o: src/drs/*.cpp
	$(CXX) -c $< -o $@ $(FLAGS) $(WXFLAGS)

build/%.o: src/drs/*.c
	$(CC) -c $< -o $@ $(FLAGS) $(WXFLAGS)

clean:
	rm -f $(TARGETS) $(OBJECTS) $(OBJ_VME) $(OBJ_DRS)
