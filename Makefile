# Grab the targets and sources as two batches
SOURCES = $(wildcard src/*.cxx)
HEADERS = $(SOURCES:.cxx=.hh)
OBJECTS = $(patsubst src%.cxx,build%.o,$(SOURCES))

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

FLAGS += -Iinclude
LIBS = -lm -lzmq

all:

fe_master: src/fe_master.cxx
	$(CXX) $(FLAGS) $(LIBS) $< -o $@

build/%.o: src/%.cxx
	$(CXX) $(FLAGS) -o $@ -c $<

clean:
	rm -f $(TARGETS) build/* 