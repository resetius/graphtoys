.PHONY: All
.DEFAULT_GOAL := All

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
	opengl/program.o\
	opengl/render.o\
	render/program.o\
	render/render.o\
	mesh.o\
	mandelbrot.o\
	mandelbulb.o\
	object.o\
	font/font.o\
	ref.o

DEPS=main.d\
	triangle.d\
	torus.d\
	opengl/program.d\
	opengl/render.d\
	render/program.d\
	render/render.d\
	mesh.d\
	mandelbrot.d\
	mandelbulb.d\
	object.d\
	font/font.d\
	ref.d

-include $(DEPS)

All: main.exe rcc.exe

clean:
	rm -f *.exe
	rm -f $(OBJECTS) $(DEPS)

main.exe: $(OBJECTS)
	gcc $^ `pkg-config --static --libs glfw3,freetype2` $(LIBGL) -o $@

rcc.exe: rcc.c
	gcc $^ -o $@

program.o: program.h

mesh.o: mesh.h

%.d: %.c
	gcc $(CFLAGS) -I. `pkg-config --cflags glfw3,freetype2` -MM -MT '$(patsubst %.c,%.o,$<)' $< -MF $@

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
