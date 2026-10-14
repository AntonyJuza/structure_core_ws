create_app_binary(MultiRecorder)
target_sources(MultiRecorder PRIVATE
    MultiRecorder.cpp MultiRecorder.h
    ConfigurationGui.cpp StreamingGui.cpp
    CaptureSessionDelegate.cpp
)
use_scsdk_defaults_for(MultiRecorder)
target_link_libraries(MultiRecorder PRIVATE
    StructurePrebuilt
    GuiSupport
    SampleProcessing
    JSON
)
use_link_time_rpath_for(MultiRecorder)
copy_libs_for_executable(MultiRecorder StructurePrebuilt)
create_app(MultiRecorderApp
    NAME MultiRecorder
    BINARIES MultiRecorder StructurePrebuilt
    ANDROID_ACTIVITY_NAME "Multi Recorder"
)
