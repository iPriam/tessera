set(WPEPlatformTessera_SOURCES
    ${WEBKIT_DIR}/WPEPlatform/wpe/tessera/WPEDisplayTessera.cpp
    ${WEBKIT_DIR}/WPEPlatform/wpe/tessera/WPEToplevelTessera.cpp
    ${WEBKIT_DIR}/WPEPlatform/wpe/tessera/WPEViewTessera.cpp
)

set(WPEPlatformTessera_INSTALLED_HEADERS
    ${WEBKIT_DIR}/WPEPlatform/wpe/tessera/wpe-tessera.h
    ${WEBKIT_DIR}/WPEPlatform/wpe/tessera/WPEDisplayTessera.h
    ${WEBKIT_DIR}/WPEPlatform/wpe/tessera/WPEToplevelTessera.h
    ${WEBKIT_DIR}/WPEPlatform/wpe/tessera/WPEViewTessera.h
)

set(WPEPlatformTessera_PRIVATE_INCLUDE_DIRECTORIES
    "${WPEPlatform_PRIVATE_INCLUDE_DIRECTORIES}"
    "${WEBKIT_DIR}/WPEPlatform/wpe/tessera"
)

add_library(WPEPlatformTessera OBJECT ${WPEPlatformTessera_SOURCES})
target_include_directories(WPEPlatformTessera PRIVATE ${WPEPlatformTessera_PRIVATE_INCLUDE_DIRECTORIES})
target_include_directories(WPEPlatformTessera SYSTEM PRIVATE ${WPEPlatform_SYSTEM_INCLUDE_DIRECTORIES})
target_link_libraries(WPEPlatformTessera ${WPEPlatform_LIBRARIES})
add_dependencies(WPEPlatformTessera WPEPlatformGeneratedEnumTypesHeader)
WEBKIT_ADD_TARGET_UNSAFE_BUFFER_WARNINGS(WPEPlatformTessera)

install(FILES ${WPEPlatformTessera_INSTALLED_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wpe-webkit-${WPE_API_VERSION}/wpe-platform/wpe/tessera"
        COMPONENT "Development"
)
