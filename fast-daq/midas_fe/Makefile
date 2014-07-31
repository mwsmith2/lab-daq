.SUFFIXES: .o .c .cxx

FLAGS = -I../../packages/midas-lite/include 
FLAGS += $(shell root-config --cflags)

LIBS = -L../../packages/midas-lite/linux/lib -lmidas-shared -lzmq
LIBS += $(shell root-config --libs)

.c.o:
	g++ -c $(FLAGS) $<

.cxx.o:
	g++ -c $(FLAGS) $<

fe_vme: fe_vme.o frontend_rpc.o rpc_g2.o
	g++ -o $@ ../../packages/midas-lite/linux/lib/mfe.o $+ $(LIBS)

fe_fake_vme: fe_fake_vme.o frontend_rpc.o rpc_g2.o
	g++ -o $@ ../../packages/midas-lite/linux/lib/mfe.o $+ $(LIBS)

clean:

	rm fe_vme *.o 