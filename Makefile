# Grab the targets and sources as two batches
SOURCES = $(wildcard src*.cxx)
#OBJECTS = $(patsubst src%.cxx,build%.o,$(SOURCES))
OBJECTS = build/daq_worker_fake.o build/event_builder.o

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

%: src/%.cxx $(OBJECTS)
	$(CXX) $(FLAGS) $(OBJECTS) $(LIBS) $< -o $@

fe_master: src/fe_master.cxx $(OBJECTS)
	$(CXX) $(FLAGS) $(OBJECTS) $(LIBS) $< -o $@

build/%.o: src/%.cxx
	$(CXX) -c $(FLAGS) $< -o $@ 

clean:
	rm -f $(TARGETS) build/* 