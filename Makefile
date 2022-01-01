.PHONY: All
.DEFAULT_GOAL := All

UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
CFLAGS?=-g -O2 -Wall
VULKAN_INCLUDE?=$(HOME)/VulkanSDK/1.2.198.1/macOS/include
VULKAN_LIB?=$(HOME)/VulkanSDK/1.2.198.1/macOS/lib
CFLAGS +=\
	-DGL_SILENCE_DEPRECATION\
	-I$(VULKAN_INCLUDE)\
	-I.\
	`pkg-config --cflags glfw3,freetype2`
LIBGL=
ifeq ($(UNAME_S),Darwin)
	LIBGL=-framework OpenGl
endif
LDFLAGS+=`pkg-config --static --libs glfw3,freetype2`
LDFLAGS+=$(LIBGL)
LDFLAGS+=-L$(VULKAN_LIB) -lvulkan

SOURCES=main.c\
	triangle.c\
	torus.c\
	opengl/program.c\
	opengl/render.c\
	render/program.c\
	render/render.c\
	vulkan/render.c\
	vulkan/renderpass.c\
	vulkan/swapchain.c\
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
	gcc $^ $(LDFLAGS) -o $@

rcc.exe: rcc.o
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
