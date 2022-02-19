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
CFLAGS += -I. $(shell pkg-config --cflags glfw3,freetype2) -Icontrib/ktx/lib/basisu/zstd  -Icontrib/ktx/other_include -Icontrib/ktx/lib/dfdutils -Icontrib/ktx/utils -Icontrib/ktx/include -Icontrib/astc-codec -DKTX_FEATURE_WRITE=1 $(SANITIZE)

LDFLAGS+=$(shell pkg-config --static --libs glfw3,freetype2) $(SANITIZE)

ifneq (,$(findstring MINGW,$(UNAME_S)))
    CFLAGS += -DS_IFSOCK=0xC000 -DKHRONOS_STATIC
endif

ifeq ($(UNAME_S),Darwin)
    CFLAGS += -DGL_SILENCE_DEPRECATION
endif

LDFLAGS+=$(VULKAN_LOADER)

ASTC_SOURCES=\
	contrib/astc-codec/src/decoder/astc_file.cc\
	contrib/astc-codec/src/decoder/codec.cc\
	contrib/astc-codec/src/decoder/endpoint_codec.cc\
	contrib/astc-codec/src/decoder/footprint.cc\
	contrib/astc-codec/src/decoder/integer_sequence_codec.cc\
	contrib/astc-codec/src/decoder/intermediate_astc_block.cc\
	contrib/astc-codec/src/decoder/logical_astc_block.cc\
	contrib/astc-codec/src/decoder/partition.cc\
	contrib/astc-codec/src/decoder/physical_astc_block.cc\
	contrib/astc-codec/src/decoder/quantization.cc\
	contrib/astc-codec/src/decoder/weight_infill.cc

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
	contrib/ktx/lib/texture1.c\
	contrib/ktx/lib/vkloader.c\
	contrib/ktx/lib/etcdec.cxx\
	contrib/ktx/lib/etcunpack.cxx\
	contrib/ktx/lib/writer1.c\
	contrib/ktx/lib/writer2.c\

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
	lib/astc_unpack.cpp\
	lib/glloader.c\
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
OBJECTS2=$(patsubst %.cc,%.o,$(OBJECTS1))
OBJECTS3=$(patsubst %.cpp,%.o,$(OBJECTS2))
OBJECTS=$(patsubst %.cxx,%.o,$(OBJECTS3))

KTX_OBJECTS1=$(patsubst %.c,%.o,$(KTX_SOURCES))
KTX_OBJECTS=$(patsubst %.cxx,%.o,$(KTX_OBJECTS1))

ASTC_OBJECTS=$(patsubst %.cc,%.o,$(ASTC_SOURCES))

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
	rm -f tools/*.exe tools/*.o

main.exe: $(OBJECTS) $(KTX_OBJECTS) $(ASTC_OBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $@

tools/rcc.exe: tools/rcc.o
	$(CC) $^ $(SANITIZE) -o $@

tools/stlprint.exe: tools/stlprint.o
	$(CC) $^ $(SANITIZE) -o $@

tools/cfgprint.exe: tools/cfgprint.o lib/config.o
	$(CC) $^ $(SANITIZE) -o $@

tools/gltfprint.exe: lib/astc_unpack.o vulkan/loader.o tools/gltfprint.o lib/formats/base64.o lib/formats/gltf.o contrib/json/json.o $(KTX_OBJECTS) $(ASTC_OBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $@

tools/ktx2tga.exe: lib/astc_unpack.o vulkan/loader.o tools/ktx2tga.o $(KTX_OBJECTS) $(ASTC_OBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $@

.deps/%.d: %.c Makefile
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -M -MG -MT '$(patsubst %.c,%.o,$<)' $< -MF $@

%.o: %.c .deps/%.d Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cxx Makefile
	$(CXX) $(CFLAGS) -c $< -o $@

%.o: %.cpp Makefile
	$(CXX) $(CFLAGS) -std=c++17 -c $< -o $@

%.o: %.cc Makefile
	$(CXX) $(CFLAGS) -std=c++17 -c $< -o $@

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
