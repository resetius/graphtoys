.PHONY: All
.DEFAULT_GOAL := All

# for macos install vulkan sdk: https://vulkan.lunarg.com/sdk/home
# for linux and windows (mingw) use package manager and install headers and glslc
CC=gcc
UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
CFLAGS?=-g -O2 -Wall
# Ubuntu: apt-get install shaderc
# Mingw: pacman -S mingw-w64-x86_64-spirv-tools mingw-w64-x86_64-vulkan-headers
GLSLC=glslc
CFLAGS += -I. $(shell pkg-config --cflags glfw3,freetype2)

LDFLAGS+=$(shell pkg-config --static --libs glfw3,freetype2)

ifeq ($(UNAME_S),Darwin)
    LDFLAGS+=-framework OpenGl
    CFLAGS += -DGL_SILENCE_DEPRECATION
endif

LDFLAGS+=$(VULKAN_LOADER)

SOURCES=main.c\
	models/triangle.c\
	models/torus.c\
	models/mandelbrot.c\
	models/mandelbulb.c\
	models/particles.c\
	models/particles2.c\
	models/stl.c\
	opengl/buffer.c\
	opengl/program.c\
	opengl/render.c\
	opengl/pipeline.c\
	render/buffer.c\
	render/pipeline.c\
	render/program.c\
	render/render.c\
	vulkan/loader.c\
	vulkan/buffer.c\
	vulkan/char.c\
	vulkan/drawcommandbuffer.c\
	vulkan/pipeline.c\
	vulkan/render.c\
	vulkan/renderpass.c\
	vulkan/rendertarget.c\
	vulkan/swapchain.c\
	vulkan/tools.c\
	lib/config.c\
	lib/object.c\
	lib/ref.c\
	font/font.c

SHADERS=models/triangle.frag\
	models/triangle.vert\
	models/torus.vert\
	models/mandelbrot.vert\
	models/mandelbrot.frag\
	models/mandelbulb.vert\
	models/mandelbulb.frag\
	font/font.vert\
	font/font.frag

FONTS=font/RobotoMono-Regular.ttf

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
DEPS=$(patsubst %.c,%.d,$(SOURCES))
GENERATED1=$(patsubst %.frag,%.frag.h,$(SHADERS))
GENERATED=$(patsubst %.vert,%.vert.h,$(GENERATED1))
GENERATED+=$(patsubst %.ttf,%.ttf.h,$(FONTS))

All: main.exe tools/stlprint.exe tools/cfgprint.exe

clean:
	rm -f *.exe
	rm -f $(OBJECTS) $(DEPS)
	rm -f $(GENERATED)

main.exe: $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

tools/rcc.exe: tools/rcc.o
	$(CC) $^ -o $@

tools/stlprint.exe: tools/stlprint.o
	$(CC) $^ -o $@

tools/cfgprint.exe: tools/cfgprint.o lib/config.o
	$(CC) $^ -o $@

%.d: %.c Makefile
	$(CC) $(CFLAGS) -M -MG -MT '$(patsubst %.c,%.o,$<)' $< -MF $@

%.o: %.c %.d Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.ttf.h: %.ttf tools/rcc.exe
	./tools/rcc.exe $< -o $@

%.vert.h: %.vert tools/rcc.exe
	./tools/rcc.exe $< -o $@

%.frag.h: %.frag tools/rcc.exe
	./tools/rcc.exe $< -o $@

%.comp.h: %.comp tools/rcc.exe
	./tools/rcc.exe $< -o $@

%.spv.h: %.spv tools/rcc.exe
	./tools/rcc.exe $< -o $@

%.vert.spv: %.vert
	$(GLSLC) -DGLSLC -fauto-bind-uniforms $< -o $@

%.frag.spv: %.frag
	$(GLSLC) -DGLSLC -fauto-bind-uniforms $< -o $@

%.comp.spv: %.comp
	$(GLSLC) -DGLSLC -fauto-bind-uniforms $< -o $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
