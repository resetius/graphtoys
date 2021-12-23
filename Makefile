UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
LIBGL=
ifeq ($(UNAME_S),Darwin)
	LIBGL=-framework OpenGl
endif

All: main.exe rcc.exe

main.exe: main.o triangle.o torus.o program.o mesh.o
	gcc $^ `pkg-config --static --libs glfw3` $(LIBGL) -o $@

rcc.exe: rcc.c
	gcc $^ -o $@

program.o: program.h

mesh.o: mesh.h

triangle.o: triangle.h triangle_vertex_shader.h triangle_fragment_shader.h

torus.o: torus.h triangle_fragment_shader.h torus_vertex_shader.h mesh.h

%.o: %.c Makefile object.h triangle.h
	gcc  -Wall -g -c -DGL_SILENCE_DEPRECATION `pkg-config --cflags glfw3` $< -o $@

%.h: %.vert rcc.exe
	./rcc.exe $< -o $@

%.h: %.frag rcc.exe
	./rcc.exe $< -o $@
