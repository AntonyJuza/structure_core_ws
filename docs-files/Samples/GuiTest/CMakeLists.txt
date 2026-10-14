create_app_binary(GuiTest)
target_sources(GuiTest PRIVATE GuiTest.cpp)
use_scsdk_defaults_for(GuiTest)
target_link_libraries(GuiTest PRIVATE GuiSupport)
use_link_time_rpath_for(GuiTest)
create_app(GuiTestApp
    NAME GuiTest
    BINARIES GuiTest
)
