UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
CFLAGS?=-g -O2 -Wall
LIBGL=
ifeq ($(UNAME_S),Darwin)
	LIBGL=-framework OpenGl
endif

OBJECTS=main.o\
	triangle.o\
	torus.o\
	program.o\
	mesh.o mandelbrot.o\
	mandelbulb.o\
	object.o\
	font/font.o

All: main.exe rcc.exe

main.exe: $(OBJECTS)
	gcc $^ `pkg-config --static --libs glfw3,freetype2` $(LIBGL) -o $@

rcc.exe: rcc.c
	gcc $^ -o $@

program.o: program.h

mesh.o: mesh.h

mandelbulb.o: mandelbulb.h mandelbulb_vs.h mandelbulb_fs.h

mandelbrot.o: mandelbrot.h mandelbrot_vs.h mandelbrot_fs.h

triangle.o: triangle.h triangle_vertex_shader.h triangle_fragment_shader.h

torus.o: torus.h triangle_fragment_shader.h torus_vertex_shader.h mesh.h

font/font.o: font/font.h font/RobotoMono-Regular.h font/font_vs.h font/font_fs.h

%.o: %.c Makefile object.h triangle.h
	gcc $(CFLAGS) -c -DGL_SILENCE_DEPRECATION -I. `pkg-config --cflags glfw3,freetype2` $< -o $@

%.h: %.ttf rcc.exe
	./rcc.exe $< -o $@

%.h: %.vs rcc.exe
	./rcc.exe $< -o $@

%.h: %.fs rcc.exe
	./rcc.exe $< -o $@

%.h: %.vert rcc.exe
	./rcc.exe $< -o $@

%.h: %.frag rcc.exe
	./rcc.exe $< -o $@
