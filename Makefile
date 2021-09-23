CC=gcc
CFLAGS=-I/usr/include/GL -D_GNU_SOURCE -DPTHREADS -Wall -Wpointer-arith -Wmissing-declarations -fno-strict-aliasing -O2 
LFLAGS=-lGL -lGLEW -lGLU -lGL -lm -lX11 -lXext

glxgears: glxgears.o
	$(CXX) -o $@ $^ $(CFLAGS) $(LFLAGS)

glxgears.o: glxgears.cpp
	$(CXX) -c -o $@ $< $(CFLAGS) -MT $< -MD -MP -MF glxgears.Tpo

clean:
	rm *.o
	rm glxgears
