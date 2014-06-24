# Grab the targets and sources as two batches
OBJECTS = $(patsubst src%.cxx, build%.o, $(wildcard src/*.cxx))
OBJ_VME = $(patsubst include/vme%.c, build%.o, $(wildcard include/vme/*.c))
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

FLAGS += $(shell root-config --cflags)
FLAGS += -Iinclude 

LIBS = $(shell root-config --libs)
LIBS += -lm -lzmq

all: $(OBJECTS) $(OBJ_VME) $(TARGETS)

fe_%: modules/fe_%.cxx $(OBJECTS) $(OBJ_VME)
	$(CXX) $< -o $@  $(FLAGS) $(OBJECTS) $(OBJ_VME) $(LIBS)

%_daq: modules/%_daq.cxx 
	$(CXX) $< -o $@  $(FLAGS) $(LIBS)

build/%.o: src/%.cxx
	$(CXX) -c $< -o $@ $(FLAGS)

build/%.o: include/vme/%.c
	$(CC) -c $< -o $@

clean:
	rm -f $(TARGETS) $(OBJECTS)