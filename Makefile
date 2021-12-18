All: main.exe

main.exe: main.o triangle.o
	gcc $^ `pkg-config --static --libs glfw3` -framework OpenGl -o $@

%.o: %.c Makefile object.h triangle.h
	gcc -g -c -DGL_SILENCE_DEPRECATION `pkg-config --cflags glfw3` $< -o $@
