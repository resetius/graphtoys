project(graphtoys)
cmake_minimum_required(VERSION 3.10)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(CMOCKA REQUIRED cmocka)

add_library(astc_codec 
    contrib/astc-codec/src/decoder/astc_file.cc
    contrib/astc-codec/src/decoder/codec.cc
    contrib/astc-codec/src/decoder/endpoint_codec.cc
    contrib/astc-codec/src/decoder/footprint.cc
    contrib/astc-codec/src/decoder/integer_sequence_codec.cc
    contrib/astc-codec/src/decoder/intermediate_astc_block.cc
    contrib/astc-codec/src/decoder/logical_astc_block.cc
    contrib/astc-codec/src/decoder/partition.cc
    contrib/astc-codec/src/decoder/physical_astc_block.cc
    contrib/astc-codec/src/decoder/quantization.cc
    contrib/astc-codec/src/decoder/weight_infill.cc)
target_include_directories(astc_codec PUBLIC contrib/astc-codec)

add_library(ktx
    contrib/ktx/lib/basisu/zstd/zstd.c
    contrib/ktx/lib/dfdutils/createdfd.c
    contrib/ktx/lib/dfdutils/colourspaces.c
    contrib/ktx/lib/dfdutils/interpretdfd.c
    contrib/ktx/lib/dfdutils/printdfd.c
    contrib/ktx/lib/dfdutils/queries.c
    contrib/ktx/lib/dfdutils/vk2dfd.c
    contrib/ktx/lib/checkheader.c
    contrib/ktx/lib/filestream.c
    contrib/ktx/lib/hashlist.c
    contrib/ktx/lib/info.c
    contrib/ktx/lib/memstream.c
    contrib/ktx/lib/strings.c
    contrib/ktx/lib/swap.c
    contrib/ktx/lib/texture.c
    contrib/ktx/lib/texture2.c
    contrib/ktx/lib/vkformat_check.c
    contrib/ktx/lib/vkformat_str.c
    contrib/ktx/lib/gl_funcs.c
    contrib/ktx/lib/texture1.c
    contrib/ktx/lib/vkloader.c
    contrib/ktx/lib/etcdec.cxx
    contrib/ktx/lib/etcunpack.cxx
    contrib/ktx/lib/writer1.c
    contrib/ktx/lib/writer2.c)
target_include_directories(ktx PRIVATE
    contrib/ktx/lib/basisu/zstd 
    contrib/ktx/other_include 
    contrib/ktx/lib/dfdutils 
    contrib/ktx/utils
    contrib/ktx/include
    ${CMAKE_SOURCE_DIR}
    PUBLIC contrib/ktx/include
    )
target_compile_options(ktx PUBLIC -DKHRONOS_STATIC)
if (MINGW)
    target_compile_options(ktx PUBLIC -DS_IFSOCK=0xC000)
endif ()

add_library(json contrib/json/json.c)

set(SHADERS
    models/base.vert
    models/dot.vert
    models/mandelbrot.vert
    models/mandelbulb.vert
    models/particles2.vert
    models/particles.vert
    models/stl.vert
    models/torus.vert
    models/triangle.vert
    models/base.frag
    models/dot.frag
    models/mandelbrot.frag
    models/mandelbulb.frag
    models/particles2.frag
    models/particles.frag
    models/stl.frag
    models/triangle.frag
    models/particles.comp
    )
set(GENERATED "")
foreach (shader ${SHADERS})
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${shader}.h 
        COMMAND rcc ${CMAKE_SOURCE_DIR}/${shader} -o ${CMAKE_BINARY_DIR}/${shader}.h --strip ${CMAKE_BINARY_DIR}/ -q
        DEPENDS ${CMAKE_SOURCE_DIR}/${shader} rcc)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${shader}.spv 
        COMMAND glslc -DGLSLC -fauto-bind-uniforms ${CMAKE_SOURCE_DIR}/${shader} -o ${CMAKE_BINARY_DIR}/${shader}.spv 
        DEPENDS ${CMAKE_SOURCE_DIR}/${shader} rcc)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${shader}.spv.h 
        COMMAND rcc ${CMAKE_BINARY_DIR}/${shader}.spv -o ${CMAKE_BINARY_DIR}/${shader}.spv.h --strip ${CMAKE_BINARY_DIR}/ -q
        DEPENDS ${CMAKE_SOURCE_DIR}/${shader} ${CMAKE_BINARY_DIR}/${shader}.spv rcc)
    list(APPEND GENERATED ${CMAKE_BINARY_DIR}/${shader}.h)
    list(APPEND GENERATED ${CMAKE_BINARY_DIR}/${shader}.spv.h)
endforeach ()

add_library(engine
    models/gltf.c
    models/triangle.c
    models/torus.c
    models/mandelbrot.c
    models/mandelbulb.c
    models/particles_data.c
    models/particles.c
    models/particles2.c
    models/stl.c
    opengl/buffer.c
    opengl/program.c
    opengl/render.c
    opengl/pipeline.c
    render/buffer.c
    render/pipeline.c
    render/render.c
    vulkan/loader.c
    vulkan/buffer.c
    vulkan/char.c
    vulkan/device.c
    vulkan/commandbuffer.c
    vulkan/pipeline.c
    vulkan/render.c
    vulkan/renderpass.c
    vulkan/rendertarget.c
    vulkan/swapchain.c
    vulkan/tools.c
    vulkan/stats.c
    vulkan/frame.c
    lib/formats/base64.c
    lib/formats/stl.c
    lib/formats/gltf.c
    lib/astc_unpack.cpp
    lib/glloader.c
    lib/camera.c
    lib/config.c
    lib/object.c
    lib/ref.c
    font/font.c
    ${GENERATED}
    )

target_include_directories(engine PRIVATE ${FREETYPE2_INCLUDE_DIRS} contrib/ktx/include PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
target_compile_options(engine PRIVATE ${FREETYPE2_CFLAGS_OTHER})
target_link_libraries(engine ktx astc_codec json ${FREETYPE2_LIBRARIES} ${GLFW3_LIBRARIES})

add_executable(main main.c)
target_link_libraries(main engine)

add_executable(test_base64 test/base64.c)
target_include_directories(test_base64 PRIVATE ${CMOCKA_INCLUDE_DIRS})
target_link_libraries(test_base64 engine ${CMOCKA_LIBRARIES})

add_executable(test_gltf test/gltf.c)
target_include_directories(test_gltf PRIVATE ${CMOCKA_INCLUDE_DIRS})
target_link_libraries(test_gltf engine ${CMOCKA_LIBRARIES})

add_executable(test_config test/config.c)
target_include_directories(test_config PRIVATE ${CMOCKA_INCLUDE_DIRS})
target_link_libraries(test_config engine ${CMOCKA_LIBRARIES})

add_executable(rcc tools/rcc.c)

enable_testing()
add_test(NAME test_gltf COMMAND test_gltf)
add_test(NAME test_base64 COMMAND test_base64)
add_test(NAME test_config COMMAND test_config)


