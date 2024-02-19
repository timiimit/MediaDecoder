cmake_minimum_required(VERSION 3.23)
project(MediaDecoder)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

set(CMAKE_TLS_VERIFY TRUE)

add_library(${PROJECT_NAME} STATIC)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE On)

target_sources(${PROJECT_NAME}
	PRIVATE
		src/MediaDecoder.c
		src/ImageResizer.c src/ImageResizer.h
		src/SoundResampler.c src/SoundResampler.h
		src/Internal.c src/Internal.h
)

target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET HEADERS BASE_DIRS include FILES include/${PROJECT_NAME}/MediaDecoder.h
)

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFmpeg REQUIRED IMPORTED_TARGET
	libavcodec
	#libavdevice
	#libavfilter
	libavformat
	libavutil
	libswresample
	libswscale
)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::FFmpeg)

install(TARGETS ${PROJECT_NAME}
	EXPORT "${PROJECT_NAME}Targets"
	FILE_SET HEADERS
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	#INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" # NOTE: due to use of FILE_SET this is not used
)
install(EXPORT "${PROJECT_NAME}Targets"
	FILE "${PROJECT_NAME}Targets.cmake"
	NAMESPACE "${PROJECT_NAME}::"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

set(version 0.1.0)
set_property(TARGET ${PROJECT_NAME} PROPERTY VERSION ${version})
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION 3)
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERFACE_${PROJECT_NAME}_MAJOR_VERSION 3)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

# generate the version file for the config file
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	VERSION "${version}"
	COMPATIBILITY AnyNewerVersion
)