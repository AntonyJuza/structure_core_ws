add_library(SampleProcessing STATIC
    SampleProcessing.h
    FrameRenderer.h FrameRenderer.cpp
)
use_scsdk_defaults_for(SampleProcessing)
target_link_libraries(SampleProcessing PRIVATE StructurePrebuilt)
target_include_directories(SampleProcessing INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if(SCSDK_IS_ANDROID)
    target_link_libraries(SampleProcessing PRIVATE GLESv3)
else()
    target_link_libraries(SampleProcessing PRIVATE scsdk_glew)
endif()
