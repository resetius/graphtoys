.PHONY: All
.DEFAULT_GOAL := All

# SANITIZE=-fsanitize=address
# SANITIZE=-fsanitize=undefined

# for macos install vulkan sdk: https://vulkan.lunarg.com/sdk/home
# for linux and windows (mingw) use package manager and install headers and glslc
CC=gcc
UNAME_S := $(shell uname -s)
PLATFORM=$(UNAME_S)
CFLAGS?=-g -O2 -Wall
# Ubuntu: apt-get install shaderc
# Mingw: pacman -S mingw-w64-x86_64-shaderc mingw-w64-x86_64-spirv-tools mingw-w64-x86_64-vulkan-headers
GLSLC=glslc
CFLAGS += -I. $(shell pkg-config --cflags glfw3,freetype2) -Icontrib/ktx/lib/basisu/zstd  -Icontrib/ktx/other_include -Icontrib/ktx/lib/dfdutils -Icontrib/ktx/utils -Icontrib/ktx/include $(SANITIZE)

LDFLAGS+=$(shell pkg-config --static --libs glfw3,freetype2) $(SANITIZE)

ifeq ($(UNAME_S),Darwin)
    LDFLAGS+=-framework OpenGl
    CFLAGS += -DGL_SILENCE_DEPRECATION
endif

LDFLAGS+=$(VULKAN_LOADER)

KTX_SOURCES=contrib/ktx/lib/basisu/zstd/zstd.c\
	contrib/ktx/lib/dfdutils/createdfd.c\
	contrib/ktx/lib/dfdutils/colourspaces.c\
	contrib/ktx/lib/dfdutils/interpretdfd.c\
	contrib/ktx/lib/dfdutils/printdfd.c\
	contrib/ktx/lib/dfdutils/queries.c\
	contrib/ktx/lib/dfdutils/vk2dfd.c\
	contrib/ktx/lib/checkheader.c\
	contrib/ktx/lib/filestream.c\
	contrib/ktx/lib/hashlist.c\
	contrib/ktx/lib/info.c\
	contrib/ktx/lib/memstream.c\
	contrib/ktx/lib/strings.c\
	contrib/ktx/lib/swap.c\
	contrib/ktx/lib/texture.c\
	contrib/ktx/lib/texture2.c\
	contrib/ktx/lib/vkformat_check.c\
	contrib/ktx/lib/vkformat_str.c\
	contrib/ktx/lib/gl_funcs.c\
	contrib/ktx/lib/glloader.c\
	contrib/ktx/lib/texture1.c\
	contrib/ktx/lib/vk_funcs.c\
	contrib/ktx/lib/vkloader.c\
	contrib/ktx/lib/etcdec.cxx\
	contrib/ktx/lib/etcunpack.cxx

SOURCES=main.c\
	models/gltf.c\
	models/triangle.c\
	models/torus.c\
	models/mandelbrot.c\
	models/mandelbulb.c\
	models/particles_data.c\
	models/particles.c\
	models/particles2.c\
	models/stl.c\
	opengl/buffer.c\
	opengl/program.c\
	opengl/render.c\
	opengl/pipeline.c\
	render/buffer.c\
	render/pipeline.c\
	render/render.c\
	vulkan/loader.c\
	vulkan/buffer.c\
	vulkan/char.c\
	vulkan/device.c\
	vulkan/commandbuffer.c\
	vulkan/pipeline.c\
	vulkan/render.c\
	vulkan/renderpass.c\
	vulkan/rendertarget.c\
	vulkan/swapchain.c\
	vulkan/tools.c\
	vulkan/stats.c\
	vulkan/frame.c\
	lib/formats/base64.c\
	lib/formats/stl.c\
	lib/formats/gltf.c\
	lib/camera.c\
	lib/config.c\
	lib/object.c\
	lib/ref.c\
	font/font.c\
	contrib/json/json.c\
	$(KTX_SOURCES)

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

OBJECTS1=$(patsubst %.c,%.o,$(SOURCES))
OBJECTS=$(patsubst %.cxx,%.o,$(OBJECTS1))

KTX_OBJECTS1=$(patsubst %.c,%.o,$(KTX_SOURCES))
KTX_OBJECTS=$(patsubst %.cxx,%.o,$(KTX_OBJECTS1))

DEPS=$(patsubst %.c,%.d,$(SOURCES))
GENERATED1=$(patsubst %.frag,%.frag.h,$(SHADERS))
GENERATED=$(patsubst %.vert,%.vert.h,$(GENERATED1))
GENERATED+=$(patsubst %.ttf,%.ttf.h,$(FONTS))

All: main.exe tools/stlprint.exe tools/cfgprint.exe tools/gltfprint.exe tools/ktx2tga.exe

clean:
	rm -f *.exe
	rm -f $(OBJECTS)
	rm -rf .deps
	rm -f $(GENERATED)

main.exe: $(OBJECTS) $(KTX_OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

tools/rcc.exe: tools/rcc.o
	$(CC) $^ $(SANITIZE) -o $@

tools/stlprint.exe: tools/stlprint.o
	$(CC) $^ $(SANITIZE) -o $@

tools/cfgprint.exe: tools/cfgprint.o lib/config.o
	$(CC) $^ $(SANITIZE) -o $@

tools/gltfprint.exe: tools/gltfprint.o lib/formats/base64.o lib/formats/gltf.o contrib/json/json.o $(KTX_OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

tools/ktx2tga.exe: tools/ktx2tga.o $(KTX_OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.deps/%.d: %.c Makefile
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -M -MG -MT '$(patsubst %.c,%.o,$<)' $< -MF $@

%.o: %.c .deps/%.d Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cxx Makefile
	$(CXX) $(CFLAGS) -c $< -o $@

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

INC_DEPS=$(foreach var,$(DEPS), .deps/$(var))
ifneq ($(MAKECMDGOALS),clean)
-include $(INC_DEPS)
endif
