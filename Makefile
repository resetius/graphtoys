All: main.exe rcc.exe

main.exe: main.o triangle.o torus.o program.o
	gcc $^ `pkg-config --static --libs glfw3` -framework OpenGl -o $@

rcc.exe: rcc.c
	gcc $^ -o $@

triangle.o: triangle.h triangle_vertex_shader.h triangle_fragment_shader.h

torus.o: torus.h triangle_fragment_shader.h torus_vertex_shader.h

%.o: %.c Makefile object.h triangle.h
	gcc  -Wall -g -c -DGL_SILENCE_DEPRECATION `pkg-config --cflags glfw3` $< -o $@

%.h: %.vert rcc.exe
	./rcc.exe $< -o $@

%.h: %.frag rcc.exe
	./rcc.exe $< -o $@
