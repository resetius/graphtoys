.PHONY: All
.DEFAULT_GOAL := All

CC=gcc
UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
CFLAGS?=-g -O2 -Wall
GLSLC=glslc
CFLAGS += -I. $(shell pkg-config --cflags glfw3,freetype2)

LDFLAGS+=$(shell pkg-config --static --libs glfw3,freetype2)

VULKAN_LOADER=-lvulkan

ifeq ($(UNAME_S),Darwin)
    VULKAN_INCLUDE?=$(HOME)/VulkanSDK/1.2.198.1/macOS/include
    VULKAN_LIB?=$(HOME)/VulkanSDK/1.2.198.1/macOS/lib
    VULKAN_BIN?=$(HOME)/VulkanSDK/1.2.198.1/macOS/bin

    LDFLAGS+=-framework OpenGl
    LDFLAGS+=-L$(VULKAN_LIB)

    CFLAGS += -I$(VULKAN_INCLUDE)
    CFLAGS += -DGL_SILENCE_DEPRECATION

    GLSLC=$(VULKAN_BIN)/glslc
endif

ifneq (,$(findstring MINGW,$(UNAME_S)))
    VULKAN_SDK=c:/VulkanSDK/1.2.198.1
    VULKAN_INCLUDE?=$(VULKAN_SDK)/include
    VULKAN_LIB?=$(VULKAN_SDK)/lib
    VULKAN_BIN?=$(VULKAN_SDK)/bin
    GLSLC=$(VULKAN_BIN)/glslc

    LDFLAGS+=-L$(VULKAN_LIB)

    CFLAGS += -I$(VULKAN_INCLUDE)

    VULKAN_LOADER=-lvulkan-1
endif

LDFLAGS+=$(VULKAN_LOADER)

SOURCES=main.c\
	models/triangle.c\
	models/torus.c\
	models/mandelbrot.c\
	models/mandelbulb.c\
	opengl/program.c\
	opengl/render.c\
	opengl/pipeline.c\
	render/program.c\
	render/render.c\
	vulkan/char.c\
	vulkan/drawcommandbuffer.c\
	vulkan/pipeline.c\
	vulkan/render.c\
	vulkan/renderpass.c\
	vulkan/rendertarget.c\
	vulkan/swapchain.c\
	vulkan/tools.c\
	object.c\
	font/font.c\
	ref.c

SHADERS=models/triangle_fragment_shader.frag\
	models/triangle_vertex_shader.vert\
	models/torus_vertex_shader.vert\
	models/mandelbrot_vs.vert\
	models/mandelbrot_fs.frag\
	models/mandelbulb_vs.vert\
	models/mandelbulb_fs.frag\
	font/font_vs.vert\
	font/font_fs.frag

FONTS=font/RobotoMono-Regular.ttf

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
DEPS=$(patsubst %.c,%.d,$(SOURCES))
GENERATED1=$(patsubst %.frag,%.frag.h,$(SHADERS))
GENERATED=$(patsubst %.vert,%.vert.h,$(GENERATED1))
GENERATED+=$(patsubst %.ttf,%.ttf.h,$(FONTS))

All: main.exe

clean:
	rm -f *.exe
	rm -f $(OBJECTS) $(DEPS)
	rm -f $(GENERATED)

main.exe: $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

tools/rcc.exe: tools/rcc.o
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

%.spv.h: %.spv tools/rcc.exe
	./tools/rcc.exe $< -o $@

%.vert.spv: %.vert
	$(GLSLC) -fauto-bind-uniforms $< -o $@

%.frag.spv: %.frag
	$(GLSLC) -fauto-bind-uniforms $< -o $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
