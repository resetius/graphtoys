.PHONY: All
.DEFAULT_GOAL := All

UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
CFLAGS?=-g -O2 -Wall
CFLAGS += -DGL_SILENCE_DEPRECATION -I. `pkg-config --cflags glfw3,freetype2`
LIBGL=
ifeq ($(UNAME_S),Darwin)
	LIBGL=-framework OpenGl
endif

SOURCES=main.c\
	triangle.c\
	torus.c\
	opengl/program.c\
	opengl/render.c\
	render/program.c\
	render/render.c\
	mesh.c\
	mandelbrot.c\
	mandelbulb.c\
	object.c\
	font/font.c\
	ref.c

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
DEPS=$(patsubst %.c,%.d,$(SOURCES))

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
	gcc $(CFLAGS) -MM -MT '$(patsubst %.c,%.o,$<)' $< -MF $@

%.o: %.c Makefile
	gcc $(CFLAGS) -c $< -o $@

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
