include_directories(utils config tags export import model)

set(core_SRCS)
set(core_MOC_HDRS)
foreach (_subdir utils config tags export import model)
  include(${_subdir}/Sources.cmake)
  set(core_SRCS ${core_SRCS} ${${_subdir}_SRCS})
  set(core_MOC_HDRS ${core_MOC_HDRS} ${${_subdir}_MOC_HDRS})
endforeach()

add_definitions(-DKID3_BUILD_CORE_LIB)
if (NOT BUILD_SHARED_LIBS)
  add_definitions(${QT_EXECUTABLE_COMPILE_FLAGS})
endif (NOT BUILD_SHARED_LIBS)
if (WITH_GCC_PCH)
  add_definitions(${GCC_PCH_COMPILE_FLAGS})
endif (WITH_GCC_PCH)

qt4_wrap_cpp(core_GEN_MOC_SRCS ${core_MOC_HDRS})

add_library(kid3-core ${core_SRCS} ${core_GEN_MOC_SRCS})
set_target_properties(kid3-core PROPERTIES VERSION ${KID3_VERSION})
target_link_libraries(kid3-core ${BASIC_LIBRARIES} ${PLUGIN_LIBRARIES})

if (BUILD_SHARED_LIBS)
  install(TARGETS kid3-core
    LIBRARY DESTINATION ${WITH_LIBDIR}
    RUNTIME DESTINATION ${WITH_BINDIR})
endif (BUILD_SHARED_LIBS)

if (BUILD_KDE_APP)
  install(FILES model/net.sourceforge.Kid3.xml
          DESTINATION ${WITH_DBUSDIR})
endif (BUILD_KDE_APP)
