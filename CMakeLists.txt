cmake_minimum_required(VERSION 3.10)
project(graphtoys)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (EXISTS ${CMAKE_BINARY_DIR}/local.cmake)
include(${CMAKE_BINARY_DIR}/local.cmake)
endif ()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(CMOCKA REQUIRED cmocka)
pkg_check_modules(VULKAN REQUIRED vulkan)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

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
set_property(TARGET astc_codec PROPERTY CXX_STANDARD 17)

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
target_compile_options(ktx PUBLIC -DKHRONOS_STATIC PRIVATE -DKTX_FEATURE_WRITE)
if (MINGW)
    target_compile_options(ktx PUBLIC -DS_IFSOCK=0xC000)
endif ()

add_library(json contrib/json/json.c)

set(SHADERS
    models/base.vert
    models/dot.vert
    models/mandelbrot.vert
    models/mandelbulb.vert
    models/particles3.vert
    models/particles2.vert
    models/particles.vert
    models/stl.vert
    models/torus.vert
    models/triangle.vert
    models/base.frag
    models/dot.frag
    models/mandelbrot.frag
    models/mandelbulb.frag
    models/particles3.frag
    models/particles2.frag
    models/particles.frag
    models/stl.frag
    models/triangle.frag
    models/particles.comp
    models/particles3_move.comp
    models/particles3_parts.comp
    models/particles3_mass.comp
    models/particles3_mass_sum.comp
    models/particles3_poisson0.comp
    models/particles3_poisson.comp
    models/particles3_poisson2.comp
    models/particles3_strength.comp
    models/particles3_pp.comp
    models/particles3_pp_sort.comp
    models/particles3_count16.comp
    models/particles3_offsets16.comp
    models/particles3_cubes16.comp
    models/particles3_mass16_64.comp
    font/font.frag
    font/font.vert
    )
set(FONTS font/RobotoMono-Regular.ttf)
set(GENERATED "")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/models)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/font)
foreach (shader ${SHADERS})
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${shader}.h
        COMMAND rcc ${CMAKE_SOURCE_DIR}/${shader} -o ${CMAKE_BINARY_DIR}/${shader}.h --strip ${CMAKE_BINARY_DIR}/ -q
        DEPENDS ${CMAKE_SOURCE_DIR}/${shader} rcc)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${shader}.spv
        COMMAND glslc -g -O --target-env=vulkan1.2 -DGLSLC -fauto-bind-uniforms ${CMAKE_SOURCE_DIR}/${shader} -o ${CMAKE_BINARY_DIR}/${shader}.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/${shader} rcc)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${shader}.spv.h
        COMMAND rcc ${CMAKE_BINARY_DIR}/${shader}.spv -o ${CMAKE_BINARY_DIR}/${shader}.spv.h --strip ${CMAKE_BINARY_DIR}/ -q
        DEPENDS ${CMAKE_SOURCE_DIR}/${shader} ${CMAKE_BINARY_DIR}/${shader}.spv rcc)
    list(APPEND GENERATED ${CMAKE_BINARY_DIR}/${shader}.h)
    list(APPEND GENERATED ${CMAKE_BINARY_DIR}/${shader}.spv.h)
endforeach ()
foreach (font ${FONTS})
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${font}.h
        COMMAND rcc ${CMAKE_SOURCE_DIR}/${font} -o ${CMAKE_BINARY_DIR}/${font}.h --strip ${CMAKE_BINARY_DIR}/ -q
        DEPENDS ${CMAKE_SOURCE_DIR}/${font} rcc)
    list(APPEND GENERATED ${CMAKE_BINARY_DIR}/${font}.h)
endforeach ()

add_library(engine
    models/gltf.c
    models/triangle.c
    models/torus.c
    models/asp_fft.c
    models/mandelbrot.c
    models/mandelbulb.c
    models/particles_data.c
    models/particles.c
    models/particles2.c
    models/particles3.c
    models/poisson_test.c
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
    lib/time.c
    font/font.c
    ${GENERATED}
    )

target_include_directories(engine
    PRIVATE ${FREETYPE2_INCLUDE_DIRS} ${VULKAN_INCLUDE_DIRS} contrib/ktx/other_include contrib/ktx/include
    PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${GLFW3_INCLUDE_DIRS})
target_compile_options(engine PRIVATE ${FREETYPE2_CFLAGS_OTHER})
target_link_directories(engine PUBLIC ${FREETYPE2_STATIC_LIBRARY_DIRS} ${GLFW3_STATIC_LIBRARY_DIRS})
target_link_libraries(engine ktx astc_codec json ${FREETYPE2_STATIC_LIBRARIES} ${GLFW3_STATIC_LIBRARIES} Threads::Threads)
if (MSVC)
    target_compile_options(engine PUBLIC /W4)
else()
    target_compile_options(engine PUBLIC -Wall)
endif()
set_property(TARGET engine PROPERTY CXX_STANDARD 17)

add_executable(main main.c)
target_link_libraries(main engine)

add_executable(test_base64 test/base64.c)
target_include_directories(test_base64 PRIVATE ${CMOCKA_INCLUDE_DIRS})
target_link_directories(test_base64 PRIVATE ${CMOCKA_LIBRARY_DIRS})
target_link_libraries(test_base64 engine ${CMOCKA_LIBRARIES})

add_executable(test_gltf test/gltf.c)
target_include_directories(test_gltf PRIVATE ${CMOCKA_INCLUDE_DIRS})
target_link_directories(test_gltf PRIVATE ${CMOCKA_LIBRARY_DIRS})
target_link_libraries(test_gltf engine ${CMOCKA_LIBRARIES})

add_executable(test_config test/config.c)
target_include_directories(test_config PRIVATE ${CMOCKA_INCLUDE_DIRS})
target_link_directories(test_config PRIVATE ${CMOCKA_LIBRARY_DIRS})
target_link_libraries(test_config engine ${CMOCKA_LIBRARIES})

add_executable(rcc tools/rcc.c)

enable_testing()
add_test(NAME test_gltf COMMAND ${CMAKE_BINARY_DIR}/test_gltf ${CMAKE_SOURCE_DIR})
add_test(NAME test_base64 COMMAND ${CMAKE_BINARY_DIR}/test_base64)
add_test(NAME test_config COMMAND ${CMAKE_BINARY_DIR}/test_config ${CMAKE_SOURCE_DIR})
set_tests_properties(test_gltf PROPERTIES ENVIRONMENT "CMOCKA_MESSAGE_OUTPUT=xml;CMOCKA_XML_FILE=test_gltf.xml")
set_tests_properties(test_base64 PROPERTIES ENVIRONMENT "CMOCKA_MESSAGE_OUTPUT=xml;CMOCKA_XML_FILE=test_base64.xml")
set_tests_properties(test_config PROPERTIES ENVIRONMENT "CMOCKA_MESSAGE_OUTPUT=xml;CMOCKA_XML_FILE=test_config.xml")
