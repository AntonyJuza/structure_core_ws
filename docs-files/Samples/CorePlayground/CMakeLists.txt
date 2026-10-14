create_app_binary(CorePlayground)
target_sources(CorePlayground PRIVATE
    CorePlayground.cpp CorePlayground.h
    CaptureSessionDelegate.cpp
    ConfigurationGui.cpp
    StreamingGui.cpp
    PlayIcon.c
    StopIcon.c
)
use_scsdk_defaults_for(CorePlayground)
target_link_libraries(CorePlayground PRIVATE
    StructurePrebuilt
    GuiSupport
    SampleProcessing
    JSON
)
use_link_time_rpath_for(CorePlayground)
copy_libs_for_executable(CorePlayground StructurePrebuilt)
create_app(CorePlaygroundApp
    NAME CorePlayground
    BINARIES CorePlayground StructurePrebuilt
)
