#-------------------------------------------------------------------------------
#   oryol Render module
#-------------------------------------------------------------------------------
oryol_begin_module(Render)
oryol_sources(. Attrs Core Setup Types Util base gl)
oryol_sources_emscripten(egl)
if (ORYOL_OSX OR ORYOL_WINDOWS OR ORYOL_LINUX)
    include_directories(${ORYOL_ROOT_DIR}/code/Ext/glfw/include)
    oryol_sources(glfw)
    oryol_deps(glfw3)
endif()
if (ORYOL_WINDOWS OR ORYOL_LINUX)
    oryol_deps(glew)
endif()
if (ORYOL_WINDOWS)
	oryol_deps(opengl32)
endif()
if (ORYOL_LINUX)
    oryol_deps(X11 Xrandr Xi Xxf86vm Xcursor GL)
endif()
oryol_deps(Resource HTTP Messaging IO Core)
oryol_end_module()

oryol_begin_unittest(Render)
oryol_sources(UnitTests)
oryol_deps(Render)
oryol_end_unittest()

