create_app_binary(DepthTester)
target_sources(DepthTester PRIVATE
    DepthTester.cpp DepthTester.h
    CaptureSessionDelegate.cpp
    ConfigurationGui.cpp
    StreamingGui.cpp
    PlayIcon.c
    StopIcon.c
)
use_scsdk_defaults_for(DepthTester)
target_link_libraries(DepthTester PRIVATE
    StructurePrebuilt
    GuiSupport
    SampleProcessing
    JSON
)
use_link_time_rpath_for(DepthTester)
copy_libs_for_executable(DepthTester StructurePrebuilt)
create_app(DepthTesterApp
    NAME DepthTester
    BINARIES DepthTester StructurePrebuilt
)
target_compile_definitions(DepthTester PRIVATE HAS_STRUCTURE_PRIVATE=1)
