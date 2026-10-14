function(use_scsdk_defaults_for target)
    set_target_properties(${target} PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    if(SCSDK_IS_MACOS OR SCSDK_IS_LINUX OR SCSDK_IS_ANDROID)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    elseif(SCSDK_IS_WINDOWS)
        target_compile_options(${target} PRIVATE /W4 /MD)
    endif()

    if(SCSDK_IS_LINUX OR SCSDK_IS_ANDROID)
        target_compile_options(${target} PRIVATE -pthread)
        get_target_property(linkFlags ${target} LINK_FLAGS)
        if(linkFlags)
            set(linkFlags "${linkFlags} -pthread")
        else()
            set(linkFlags "-pthread")
        endif()
        set_target_properties(${target} PROPERTIES
            LINK_FLAGS "${linkFlags}"
        )

        if(EXTRA_LIBRARIES)
            target_link_libraries(${target} PRIVATE ${EXTRA_LIBRARIES})
        endif()
    endif()
endfunction()

# Sometimes needed on Linux for gcc cross/sysroot builds if gcc fails to search
# sysroot's ld.so.conf paths. Also needed for transitive shared library
# dependencies in Android NDK.
function(use_link_time_rpath_for target)
    if(SCSDK_LINK_TIME_RPATH)
        if(NOT (SCSDK_IS_LINUX OR SCSDK_IS_ANDROID))
            message(FATAL_ERROR "SCSDK_LINK_TIME_RPATH is only supported on Linux and Android")
        endif()
        string(REPLACE ";" ":" paths "${SCSDK_LINK_TIME_RPATH}")
        set(linkRpathFlags "-Wl,-rpath-link,${paths}")
        get_target_property(linkFlags ${target} LINK_FLAGS)
        if(linkFlags)
            set(linkFlags "${linkFlags} ${linkRpathFlags}")
        else()
            set(linkFlags "${linkRpathFlags}")
        endif()
        set_target_properties(${target} PROPERTIES
            LINK_FLAGS "${linkFlags}"
        )
    endif()
endfunction()

function(copy_libs_for_executable executable)
    if(NOT TARGET ${executable})
        message(FATAL_ERROR "${executable} is not a target, don't know how to copy libs for it")
    endif()

    # Set runtime load path if applicable
    if(SCSDK_IS_MACOS)
        get_target_property(linkFlags ${executable} LINK_FLAGS)
        if(linkFlags)
            set(linkFlags "${linkFlags} -Wl,-rpath,@loader_path")
        else()
            set(linkFlags "-Wl,-rpath,@loader_path")
        endif()
        set_target_properties(${executable} PROPERTIES
            SKIP_BUILD_RPATH YES
            LINK_FLAGS "${linkFlags}"
        )
    elseif(SCSDK_IS_LINUX OR SCSDK_IS_ANDROID)
        get_target_property(linkFlags ${executable} LINK_FLAGS)
        # Assume link flags will pass through shell, escape $ORIGIN accordingly
        if(linkFlags)
            set(linkFlags "${linkFlags} -Wl,-rpath,'$ORIGIN'")
        else()
            set(linkFlags "-Wl,-rpath,'$ORIGIN'")
        endif()
        set_target_properties(${executable} PROPERTIES
            SKIP_BUILD_RPATH YES
            LINK_FLAGS "${linkFlags}"
        )
    elseif(SCSDK_IS_WINDOWS)
        # Windows already allows this
    endif()

    # Copy libraries
    set(depends)
    set(commands)
    foreach(lib ${ARGN})
        get_src_base_dep_name_for_target(libSrc libBase libDep_ ${lib})
        set(libDest $<TARGET_FILE_DIR:${executable}>/${libBase})
        list(APPEND depends ${lib})
        list(APPEND commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${libSrc} ${libDest}
        )
    endforeach()
    add_custom_command(TARGET ${executable} POST_BUILD
        ${commands}
    )
    add_dependencies(${executable} ${depends})
endfunction()

# It is sometimes necessary to determine, at configuration time, the filename of
# a target. While the generator expressions TARGET_FILE and TARGET_FILE_NAME
# exist, they cannot be used in (e.g.) the OUTPUT argument of
# add_custom_command.
#
# srcVar is set to the full path of the target's output file and may be a
# TARGET_FILE generator expression.
#
# baseVar is set to the base filename of the target's output file and will never
# be a generator expression. This requires making some assumptions about how
# executables and shared libraries are named on the target platform.
#
# depVar is set to the full path of the target's output file if it is known at
# configuration time (not a generator expression), otherwise the target name.
function(get_src_base_dep_name_for_target srcVar baseVar depVar target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} is not a target, don't know how to get its path")
    endif()
    get_target_property(src ${target} SCSDK_LIBCOPY_SRC)
    if(src)
        set(${srcVar} ${src} PARENT_SCOPE)
        get_filename_component(base ${src} NAME)
        set(${baseVar} ${base} PARENT_SCOPE)
        set(${depVar} ${src} PARENT_SCOPE)
    else()
        get_target_property(type ${target} TYPE)
        if(SCSDK_IS_WINDOWS)
            set(executableSuffix .exe)
            set(libraryPrefix "")
            set(librarySuffix .dll)
        elseif(SCSDK_IS_MACOS)
            set(executableSuffix "")
            set(libraryPrefix lib)
            set(librarySuffix .dylib)
        elseif(SCSDK_IS_LINUX OR SCSDK_IS_ANDROID)
            set(executableSuffix "")
            set(libraryPrefix lib)
            set(librarySuffix .so)
        else()
            message(FATAL_ERROR "Don't know executable and shared library suffixes for this platform!")
        endif()
        if(type STREQUAL "EXECUTABLE")
            set(${srcVar} $<TARGET_FILE:${target}> PARENT_SCOPE)
            set(${baseVar} ${target}${executableSuffix} PARENT_SCOPE)
            set(${depVar} ${target} PARENT_SCOPE)
        elseif(type STREQUAL "SHARED_LIBRARY")
            set(${srcVar} $<TARGET_FILE:${target}> PARENT_SCOPE)
            set(${baseVar} ${libraryPrefix}${target}${librarySuffix} PARENT_SCOPE)
            set(${depVar} ${target} PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Don't know how to handle targets of type: ${type}")
        endif()
    endif()
endfunction()

if(NOT SCSDK_APP_OUTPUT_DIR)
    set(SCSDK_APP_OUTPUT_DIR ${SCSDK_ROOT}/Apps)
endif()

function(create_console_app appTarget)
    cmake_parse_arguments(CREATE_CONSOLE_APP "" "NAME" "BINARIES" ${ARGN})
    if(NOT CREATE_CONSOLE_APP_NAME)
        message(FATAL_ERROR "NAME parameter is required")
    endif()

    set(dir ${SCSDK_APP_OUTPUT_DIR}/${CREATE_CONSOLE_APP_NAME})
    set(outputs)
    foreach(binary ${CREATE_CONSOLE_APP_BINARIES})
        get_src_base_dep_name_for_target(binSrc binBase binDep ${binary})
        add_custom_command(
            OUTPUT ${dir}/${binBase}
            DEPENDS ${binDep}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
            COMMAND ${CMAKE_COMMAND} -E copy ${binSrc} ${dir}/${binBase}
        )
        list(APPEND outputs ${dir}/${binBase})
    endforeach()
    add_custom_target(${appTarget} DEPENDS ${outputs})
endfunction()

function(create_app_binary binaryName)
    if(SCSDK_IS_ANDROID)
        file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dummy.c CONTENT "static void _dummy(void) {}")
        add_library(${binaryName} SHARED ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
    else()
        add_executable(${binaryName} ${SCSDK_ROOT}/Libraries/Shared/AppInterface_Desktop.cpp)
    endif()
    target_include_directories(${binaryName} PRIVATE ${SCSDK_ROOT}/Libraries/Shared)
endfunction()

function(create_app appTarget)
    cmake_parse_arguments(CREATE_APP "" "NAME;ANDROID_ACTIVITY_NAME" "BINARIES" ${ARGN})
    if(NOT CREATE_APP_NAME)
        message(FATAL_ERROR "NAME parameter is required")
    endif()
    if(SCSDK_IS_MACOS)
        set(bundle ${SCSDK_APP_OUTPUT_DIR}/${CREATE_APP_NAME}.app)
        set(plistSrc ${CMAKE_CURRENT_BINARY_DIR}/${CREATE_APP_NAME}-Info.plist)
        configure_file(${SCSDK_ROOT}/Libraries/Shared/Info.plist.in ${plistSrc} @ONLY)
        set(outputs
            ${bundle}/Contents/Info.plist
            ${bundle}/Contents/PkgInfo
        )
        add_custom_command(
            OUTPUT ${bundle}/Contents/Info.plist ${bundle}/Contents/PkgInfo
            DEPENDS ${plistSrc} ${SCSDK_ROOT}/Scripts/create-macos-pkginfo.sh
            COMMAND ${CMAKE_COMMAND} -E make_directory ${bundle}
            COMMAND ${CMAKE_COMMAND} -E copy ${plistSrc} ${bundle}/Contents/Info.plist
            COMMAND ${SCSDK_ROOT}/Scripts/create-macos-pkginfo.sh ${bundle}/Contents/PkgInfo
        )
        foreach(binary ${CREATE_APP_BINARIES})
            get_src_base_dep_name_for_target(binSrc binBase binDep ${binary})
            add_custom_command(
                OUTPUT ${bundle}/Contents/MacOS/${binBase}
                DEPENDS ${binDep}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${bundle}/Contents/MacOS
                COMMAND ${CMAKE_COMMAND} -E copy ${binSrc} ${bundle}/Contents/MacOS/${binBase}
            )
            list(APPEND outputs ${bundle}/Contents/MacOS/${binBase})
        endforeach()
        add_custom_target(${appTarget} DEPENDS ${outputs})
    elseif(SCSDK_IS_LINUX OR SCSDK_IS_WINDOWS)
        set(dir ${SCSDK_APP_OUTPUT_DIR}/${CREATE_APP_NAME})
        set(outputs)
        foreach(binary ${CREATE_APP_BINARIES})
            get_src_base_dep_name_for_target(binSrc binBase binDep ${binary})
            add_custom_command(
                OUTPUT ${dir}/${binBase}
                DEPENDS ${binDep}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
                COMMAND ${CMAKE_COMMAND} -E copy ${binSrc} ${dir}/${binBase}
            )
            list(APPEND outputs ${dir}/${binBase})
        endforeach()
        add_custom_target(${appTarget} DEPENDS ${outputs})
    elseif(SCSDK_IS_ANDROID)
        if(NOT CREATE_APP_ANDROID_ACTIVITY_NAME)
            set(CREATE_APP_ANDROID_ACTIVITY_NAME ${CREATE_APP_NAME})
        endif()
        set(copyFiles
            Libraries/AndroidProjectTemplate/gradlew:gradlew
            Libraries/AndroidProjectTemplate/gradlew.bat:gradlew.bat
            Libraries/AndroidProjectTemplate/gradle-wrapper.jar:gradle/wrapper/gradle-wrapper.jar
            Libraries/AndroidProjectTemplate/gradle-wrapper.properties:gradle/wrapper/gradle-wrapper.properties
            Libraries/AndroidProjectTemplate/sample.p12:sample.p12
            Libraries/Shared/AppInterface.h:src/main/cpp/AppInterface.h
        )
        set(configureFiles
            Libraries/AndroidProjectTemplate/CMakeLists.txt.in:CMakeLists.txt
            Libraries/AndroidProjectTemplate/build.gradle.in:build.gradle
            Libraries/AndroidProjectTemplate/settings.gradle.in:settings.gradle
            Libraries/AndroidProjectTemplate/AndroidManifest.xml.in:src/main/AndroidManifest.xml
            Libraries/AndroidProjectTemplate/MainActivity.java.in:src/main/java/com/occipital/${CREATE_APP_NAME}/MainActivity.java
            Libraries/AndroidProjectTemplate/MainActivity.cpp.in:src/main/cpp/MainActivity.cpp
            Libraries/AndroidProjectTemplate/StructureCoreUSBHelper.java.in:src/main/java/com/occipital/${CREATE_APP_NAME}/StructureCoreUSBHelper.java
        )
        set(appBuildDir ${CMAKE_CURRENT_BINARY_DIR}/${CREATE_APP_NAME}-android)
        set(gradleDeps)
        file(MAKE_DIRECTORY ${appBuildDir})
        foreach(srcAndDest ${copyFiles})
            string(REGEX REPLACE ":.*" "" src ${srcAndDest})
            string(REGEX REPLACE ".*:" "" dest ${srcAndDest})
            set(src ${SCSDK_ROOT}/${src})
            set(dest ${appBuildDir}/${dest})
            get_filename_component(destDir ${dest} DIRECTORY)
            add_custom_command(
                OUTPUT ${dest}
                DEPENDS ${src}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${destDir}
                COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dest}
            )
            list(APPEND gradleDeps ${dest})
        endforeach()
        foreach(srcAndDest ${configureFiles})
            string(REGEX REPLACE ":.*" "" src ${srcAndDest})
            string(REGEX REPLACE ".*:" "" dest ${srcAndDest})
            set(src ${SCSDK_ROOT}/${src})
            set(dest ${appBuildDir}/${dest})
            get_filename_component(destDir ${dest} DIRECTORY)
            file(MAKE_DIRECTORY ${destDir})
            configure_file(${src} ${dest} @ONLY)
            list(APPEND gradleDeps ${dest})
        endforeach()
        set(binDir ${appBuildDir}/lib/${ANDROID_ABI})
        foreach(binary ${CREATE_APP_BINARIES})
            get_src_base_dep_name_for_target(binSrc binBase binDep ${binary})
            add_custom_command(
                OUTPUT ${binDir}/${binBase}
                DEPENDS ${binDep}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${binDir}
                COMMAND ${CMAKE_COMMAND} -E copy ${binSrc} ${binDir}/${binBase}
            )
            list(APPEND gradleDeps ${binDir}/${binBase})
        endforeach()
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(gradleTask assembleDebug)
            set(apk ${appBuildDir}/build/outputs/apk/debug/${CREATE_APP_NAME}-debug.apk)
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            set(gradleTask assembleRelease)
            set(apk ${appBuildDir}/build/outputs/apk/release/${CREATE_APP_NAME}-release.apk)
        else()
            message(WARNING "Don't understand CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}, assuming APK should build under Debug")
            set(gradleTask assembleDebug)
            set(apk ${appBuildDir}/build/outputs/apk/debug/${CREATE_APP_NAME}-debug.apk)
        endif()
        if(CMAKE_HOST_SYSTEM MATCHES "Windows")
            set(gradleCmd gradlew.bat)
        else()
            set(gradleCmd ./gradlew)
        endif()
        add_custom_command(
            OUTPUT ${apk}
            DEPENDS ${gradleDeps}
            COMMAND ${CMAKE_COMMAND} -E chdir ${appBuildDir} ${gradleCmd} ${gradleTask}
        )
        set(finalApk ${SCSDK_APP_OUTPUT_DIR}/${CREATE_APP_NAME}.apk)
        add_custom_command(
            OUTPUT ${finalApk}
            DEPENDS ${apk}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SCSDK_APP_OUTPUT_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${apk} ${finalApk}
        )
        add_custom_target(${appTarget} DEPENDS ${finalApk})
    endif()
endfunction()
