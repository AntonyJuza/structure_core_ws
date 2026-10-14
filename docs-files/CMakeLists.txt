cmake_minimum_required(VERSION 3.5)
project(StructureSDK-CrossPlatform)
set(SCSDK_ROOT ${PROJECT_SOURCE_DIR})

if(NOT SCSDK_TARGET_ARCH)
    message(WARNING "SCSDK_TARGET_ARCH is not set (build scripts will do this automatically). Assuming x86_64.")
    set(SCSDK_TARGET_ARCH x86_64)
endif()

set(SCSDK_STRUCTURE_HEADERS ${SCSDK_ROOT}/Libraries/Structure/Headers)

if(APPLE)
    set(SCSDK_IS_MACOS YES)
    set(SCSDK_STRUCTURE_LIBDIR ${SCSDK_ROOT}/Libraries/Structure/macOS/${SCSDK_TARGET_ARCH})
    set(SCSDK_STRUCTURE_LIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.dylib)
    set(SCSDK_STRUCTURE_IMPORTLIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.dylib)
    # 20190814: For avoiding respecting to host platforms. Might need to change in the
    # furture if we decided to move foward this target version.  
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13")
elseif(ANDROID)
    set(SCSDK_IS_ANDROID YES)
    set(SCSDK_STRUCTURE_LIBDIR ${SCSDK_ROOT}/Libraries/Structure/Android/${SCSDK_TARGET_ARCH})
    set(SCSDK_STRUCTURE_LIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.so)
    set(SCSDK_STRUCTURE_IMPORTLIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.so)
elseif(UNIX)
    execute_process(COMMAND uname -s RESULT_VARIABLE unameResult OUTPUT_VARIABLE unameOutput)
    string(STRIP ${unameOutput} unameOutput)
    if(NOT unameResult STREQUAL "0")
        message(FATAL_ERROR "Failed to run uname, unable to identify platform.")
    elseif(NOT unameOutput STREQUAL "Linux")
        message(FATAL_ERROR "This Unix platform is not supported. uname: ${unameOutput}")
    endif()
    set(SCSDK_IS_LINUX YES)
    set(SCSDK_STRUCTURE_LIBDIR ${SCSDK_ROOT}/Libraries/Structure/Linux/${SCSDK_TARGET_ARCH})
    set(SCSDK_STRUCTURE_LIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.so)
    set(SCSDK_STRUCTURE_IMPORTLIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.so)
elseif(WIN32)
    set(SCSDK_IS_WINDOWS YES)
    set(SCSDK_STRUCTURE_LIBDIR ${SCSDK_ROOT}/Libraries/Structure/Windows/${SCSDK_TARGET_ARCH})
    set(SCSDK_STRUCTURE_LIB ${SCSDK_STRUCTURE_LIBDIR}/Structure.dll)
    set(SCSDK_STRUCTURE_IMPORTLIB ${SCSDK_STRUCTURE_LIBDIR}/Structure.lib)
else()
    message(FATAL_ERROR "Unknown platform for Structure Core SDK.")
endif()

include(${SCSDK_ROOT}/CMake/project.cmake)
if(EXISTS ${SCSDK_ROOT}/CMake/private.cmake)
    include(${SCSDK_ROOT}/CMake/private.cmake)
endif()

if(NOT TARGET StructurePrebuilt)
    add_library(StructurePrebuilt UNKNOWN IMPORTED)
    set_target_properties(StructurePrebuilt PROPERTIES
        IMPORTED_LOCATION ${SCSDK_STRUCTURE_IMPORTLIB}
        INTERFACE_INCLUDE_DIRECTORIES ${SCSDK_STRUCTURE_HEADERS}
    )
    set_target_properties(StructurePrebuilt PROPERTIES SCSDK_LIBCOPY_SRC ${SCSDK_STRUCTURE_LIB})
endif()

add_subdirectory(${SCSDK_ROOT}/External/ImGui)
add_subdirectory(${SCSDK_ROOT}/External/JSON)

if(NOT SCSDK_IS_ANDROID)
    set(BUILD_UTILS OFF CACHE BOOL "unused (GLEW)" FORCE)
    add_subdirectory(${SCSDK_ROOT}/External/GLEW/build/cmake)
    add_library(scsdk_glew INTERFACE)
    target_link_libraries(scsdk_glew INTERFACE glew_s)
    target_compile_definitions(scsdk_glew INTERFACE GLEW_NO_GLU=1)
    target_include_directories(scsdk_glew INTERFACE ${SCSDK_ROOT}/External/GLEW/include)

    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "unused (GLFW)" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "unused (GLFW)" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "unused (GLFW)" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "unused (GLFW)" FORCE)
    add_subdirectory(${SCSDK_ROOT}/External/GLFW)
    add_library(scsdk_glfw INTERFACE)
    target_link_libraries(scsdk_glfw INTERFACE glfw)
    target_include_directories(scsdk_glfw INTERFACE ${SCSDK_ROOT}/External/GLFW/include)
endif()

add_subdirectory(${SCSDK_ROOT}/Libraries/GuiSupport)
add_subdirectory(${SCSDK_ROOT}/Libraries/SampleProcessing)
add_subdirectory(${SCSDK_ROOT}/Samples/CorePlayground)
add_subdirectory(${SCSDK_ROOT}/Samples/GuiTest)
add_subdirectory(${SCSDK_ROOT}/Samples/SimpleStreamer)
add_subdirectory(${SCSDK_ROOT}/Samples/DepthTester)
add_subdirectory(${SCSDK_ROOT}/Samples/MultiRecorder)

add_custom_target(Samples DEPENDS
    CorePlaygroundApp
    GuiTestApp
    SimpleStreamerApp
    DepthTesterApp
    MultiRecorderApp
)
