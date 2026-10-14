add_executable(SimpleStreamer SimpleStreamer.cpp)

if(SCSDK_IS_MACOS)
    set_source_files_properties(SimpleStreamer.cpp PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
    target_link_libraries(SimpleStreamer PRIVATE
        "-framework AppKit"
    )
endif()

target_link_libraries(SimpleStreamer PRIVATE StructurePrebuilt)
use_scsdk_defaults_for(SimpleStreamer)
use_link_time_rpath_for(SimpleStreamer)
copy_libs_for_executable(SimpleStreamer StructurePrebuilt)

create_console_app(SimpleStreamerApp
    NAME SimpleStreamer
    BINARIES SimpleStreamer StructurePrebuilt
)
