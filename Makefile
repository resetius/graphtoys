.PHONY: All
.DEFAULT_GOAL := All

CC=gcc
UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
CFLAGS?=-g -O2 -Wall
VULKAN_INCLUDE?=$(HOME)/VulkanSDK/1.2.198.1/macOS/include
VULKAN_LIB?=$(HOME)/VulkanSDK/1.2.198.1/macOS/lib
VULKAN_BIN?=$(HOME)/VulkanSDK/1.2.198.1/macOS/bin
GLSLC=$(VULKAN_BIN)/glslc
CFLAGS +=\
	-DGL_SILENCE_DEPRECATION\
	-I$(VULKAN_INCLUDE)\
	-I.\
	$(shell pkg-config --cflags glfw3,freetype2)
LIBGL=
ifeq ($(UNAME_S),Darwin)
	LIBGL=-framework OpenGl
endif
LDFLAGS+=$(shell pkg-config --static --libs glfw3,freetype2)
LDFLAGS+=$(LIBGL)
LDFLAGS+=-L$(VULKAN_LIB) -lvulkan

SOURCES=main.c\
	triangle.c\
	triangle_vk.c\
	torus.c\
	opengl/program.c\
	opengl/render.c\
	opengl/pipeline.c\
	render/program.c\
	render/render.c\
	vulkan/drawcommandbuffer.c\
	vulkan/pipeline.c\
	vulkan/render.c\
	vulkan/renderpass.c\
	vulkan/rendertarget.c\
	vulkan/swapchain.c\
	vulkan/tools.c\
	mesh.c\
	mandelbrot.c\
	mandelbulb.c\
	object.c\
	font/font.c\
	ref.c

SHADERS=triangle_fragment_shader.frag\
	triangle_vertex_shader.vert\
	torus_vertex_shader.vert\
	mandelbrot_vs.vert\
	mandelbrot_fs.frag\
	mandelbulb_vs.vert\
	mandelbulb_fs.frag\
	font/font_vs.vert\
	font/font_fs.frag

FONTS=font/RobotoMono-Regular.ttf

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
DEPS=$(patsubst %.c,%.d,$(SOURCES))
GENERATED1=$(patsubst %.frag,%.frag.h,$(SHADERS))
GENERATED=$(patsubst %.vert,%.vert.h,$(GENERATED1))
GENERATED+=$(patsubst %.ttf,%.ttf.h,$(FONTS))

All: main.exe rcc.exe

clean:
	rm -f *.exe
	rm -f $(OBJECTS) $(DEPS)
	rm -f $(GENERATED)

main.exe: $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

rcc.exe: rcc.o
	$(CC) $^ -o $@

program.o: program.h

mesh.o: mesh.h

%.d: %.c Makefile
	$(CC) $(CFLAGS) -M -MG -MT '$(patsubst %.c,%.o,$<)' $< -MF $@

%.o: %.c %.d Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.ttf.h: %.ttf rcc.exe
	./rcc.exe $< -o $@

%.vert.h: %.vert rcc.exe
	./rcc.exe $< -o $@

%.frag.h: %.frag rcc.exe
	./rcc.exe $< -o $@

%.spv.h: %.spv rcc.exe
	./rcc.exe $< -o $@

%.vert.spv: %.vert
	$(GLSLC) -fauto-bind-uniforms $< -o $@

%.frag.spv: %.frag
	$(GLSLC) -fauto-bind-uniforms $< -o $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
