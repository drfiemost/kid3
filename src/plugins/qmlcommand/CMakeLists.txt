set(plugin_SRCS
  qmlcommandplugin.cpp
)

set(plugin_MOC_HDRS
  qmlcommandplugin.h
)

set(plugin_NAME QmlCommand)

if (WITH_GCC_PCH)
  add_definitions(${GCC_PCH_COMPILE_FLAGS})
endif (WITH_GCC_PCH)

string(TOLOWER ${plugin_NAME} plugin_TARGET)

qt4_wrap_cpp(plugin_GEN_MOC_SRCS ${plugin_MOC_HDRS})

add_library(${plugin_TARGET} ${plugin_SRCS} ${plugin_GEN_MOC_SRCS})
target_link_libraries(${plugin_TARGET} kid3-core ${BASIC_LIBRARIES})

INSTALL_KID3_PLUGIN(${plugin_TARGET} ${plugin_NAME})
