# Grab the targets and sources as two batches
OBJECTS = $(patsubst src%.cxx,build%.o,$(wildcard src/*.cxx))
TARGETS = $(patsubst modules%.cxx,%,$(wildcard modules/*.cxx))

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

all:

%: modules/%.cxx $(OBJECTS)
	$(CXX) $< -o $@  $(FLAGS) $(OBJECTS) $(LIBS)

build/%.o: src/%.cxx
	$(CXX) -c $< -o $@ $(FLAGS)

clean:
	rm -f $(TARGETS) $(OBJECTS)