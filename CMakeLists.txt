cmake_minimum_required(VERSION 3.15)
project(BGFXGwen VERSION 1.2.3)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_REQUIRED_FLAGS -std=c++17)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup(TARGETS)

# Find the requirements
# find_package(glfw3 REQUIRED)
# include_directories(${glfw3_INCLUDE_DIR})
# find_package(glm REQUIRED)
# include_directories(${GLM_INCLUDE_DIR})

# find_package(GTest REQUIRED)


# include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# # include_directories(${glfw3_INCLUDE_DIRS})
# include_directories(${CONAN_INCLUDE_DIRS_GLFW})
# include_directories(${CONAN_INCLUDE_DIRS_GLM})
list(APPEND SRC_FILES
    ${CMAKE_CURRENT_LIST_DIR}/Renderer/bgfxRenderer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Renderer/GlyphStash.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Renderer/RectanglePacker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Renderer/TrueTypeFont.cpp
)    

list(APPEND libs
    # -licui18n
    ${GWEN_HOME}/lib/libGWEN.so
    ../dist/lib/libbgfxRelease.a
    ./dist/lib/libbimg_decodeRelease.a
    ./dist/lib/libbimg_encodeRelease.a
    ./dist/lib/libbimgRelease.a
    ./dist/lib/libbxRelease.a
    ./dist/lib/libexample-commonRelease.a
    ./dist/lib/libexample-glueRelease.a
    ./dist/lib/libfcppRelease.a  
    ./dist/lib/libglslangRelease.a
    ./dist/lib/libglsl-optimizerRelease.a
    ./dist/lib/libspirv-crossRelease.a
    ./dist/lib/libspirv-optRelease.a
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Render
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/../bgfx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../bx/include
    ${GWEN_HOME}/include
)
# Library
add_library(bgfxGWEN SHARED ${SRC_FILES})
target_compile_definitions(bgfxGWEN PRIVATE BX_CONFIG_DEBUG=0)
target_link_libraries(bgfxGWEN ${libs})

# Samples
add_executable(bgfxSample.bin ${CMAKE_CURRENT_SOURCE_DIR}/Sample/bgfxSample.cpp)
target_include_directories(bgfxSample.bin PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)
# target_include_directories(trianglegl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# set_target_properties(trianglegl
#     PROPERTIES
#         PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/trianglegl.hpp 
#         PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/common/shader.hpp
# )
target_link_libraries(bgfxSample.bin PUBLIC bgfxGWEN ${libs})
# target_link_libraries(trianglegl ${CONAN_LIBS})


# Tests
# enable_testing()
# add_executable(triangletests ${CMAKE_CURRENT_SOURCE_DIR}/test/triangletest.cpp)
# target_link_libraries(triangletests trianglegl CONAN_PKG::gtest CONAN_PKG:glfw CONAN_PKG::glm)
# add_test(NAME triangletests COMMAND triangletests --gtest_output=xml:results.xml)

# # Installation (relative to CMAKE_INSTALL_PREFIX)
# install(TARGETS trianglegl
#     ARCHIVE DESTINATION lib 
#     LIBRARY DESTINATION lib
#     PUBLIC_HEADER DESTINATION include
# )

# # Bundling
# set(CPACK_PACKAGE_CONTACT "snarf the cat")
# set(CPACK_GENERATOR "STGZ;TGZ;TZ;DEB")
# SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CMake + Conan OpenGL")
# SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
# include(CPack)