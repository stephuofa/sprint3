set(KATHERINE_SOURCES
    "src/acquisition.c"
    "src/px_config.c"
    "src/config.c"
    "src/device.c"
    "src/status.c"
    "src/udp_nix.c"
    "src/udp_win.c"
    "src/crd.h"
    "src/bitfields.h"
    "src/command_interface.h"
    "src/md.h"
)

set(KATHERINE_HEADERS
    "include/katherine/acquisition.h"
    "include/katherine/config.h"
    "include/katherine/device.h"
    # "include/katherine/global.h"
    "include/katherine/katherine.h"
    "include/katherine/px_config.h"
    "include/katherine/px.h"
    "include/katherine/status.h"
    "include/katherine/udp.h"
    "include/katherine/udp_nix.h"
    "include/katherine/udp_win.h"
)

add_library(katherine STATIC ${KATHERINE_SOURCES})
target_include_directories(katherine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/katherine>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# On Windows, link to ws2_32
if(WIN32 OR MINGW)
  target_link_libraries(katherine PUBLIC ws2_32)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/katherine.pc.in katherine.pc @ONLY)

install(TARGETS katherine LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${KATHERINE_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/katherine)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/katherine.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
