cmake_minimum_required(VERSION 3.23)
project(MediaDecoder)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	# on windows we download a specific FFmpeg build
	set(FFMPEG_BUILD_NAME "autobuild-2024-02-13-15-10")
	set(FFMPEG_VERSION "ffmpeg-n6.1.1-5-g3e79074c41-win64-lgpl-shared-6.1")
	set(FFMPEG_DOWNLOAD_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/${FFMPEG_BUILD_NAME}/${FFMPEG_VERSION}.zip")

	set(FFMPEG_DIR "${CMAKE_BINARY_DIR}/${FFMPEG_VERSION}")
	set(FFMPEG_ZIP "${CMAKE_BINARY_DIR}/${FFMPEG_VERSION}.zip")

	if (NOT EXISTS "${FFMPEG_DIR}")
		if (NOT EXISTS "${FFMPEG_ZIP}")
			message("Downloading ${FFMPEG_DOWNLOAD_URL} ...")
			file(DOWNLOAD "${FFMPEG_DOWNLOAD_URL}" "${FFMPEG_ZIP}" SHOW_PROGRESS)
		endif()
		message("Extracting ${FFMPEG_ZIP} ...")
		file(ARCHIVE_EXTRACT INPUT "${FFMPEG_ZIP}" DESTINATION "${CMAKE_BINARY_DIR}")
		if (EXISTS "${FFMPEG_DIR}")
			file(REMOVE "${FFMPEG_ZIP}")
		endif()
	endif()
	if (EXISTS "${FFMPEG_DIR}")
		add_library(avcodec SHARED IMPORTED)
		set_target_properties(avcodec PROPERTIES
			IMPORTED_LOCATION "${FFMPEG_DIR}/bin/avcodec-60.dll"
			IMPORTED_IMPLIB "${FFMPEG_DIR}/lib/avcodec.lib"
		)

		add_library(avdevice SHARED IMPORTED)
		set_target_properties(avdevice PROPERTIES
			IMPORTED_LOCATION "${FFMPEG_DIR}/bin/avdevice-60.dll"
			IMPORTED_IMPLIB "${FFMPEG_DIR}/lib/avdevice.lib"
		)

		add_library(avfilter SHARED IMPORTED)
		set_target_properties(avfilter PROPERTIES
			IMPORTED_LOCATION "${FFMPEG_DIR}/bin/avfilter-9.dll"
			IMPORTED_IMPLIB "${FFMPEG_DIR}/lib/avfilter.lib"
		)

		add_library(avformat SHARED IMPORTED)
		set_target_properties(avformat PROPERTIES
			IMPORTED_LOCATION "${FFMPEG_DIR}/bin/avformat-60.dll"
			IMPORTED_IMPLIB "${FFMPEG_DIR}/lib/avformat.lib"
		)

		add_library(avutil SHARED IMPORTED)
		set_target_properties(avutil PROPERTIES
			IMPORTED_LOCATION "${FFMPEG_DIR}/bin/avutil-58.dll"
			IMPORTED_IMPLIB "${FFMPEG_DIR}/lib/avutil.lib"
		)

		add_library(swresample SHARED IMPORTED)
		set_target_properties(swresample PROPERTIES
			IMPORTED_LOCATION "${FFMPEG_DIR}/bin/swresample-4.dll"
			IMPORTED_IMPLIB "${FFMPEG_DIR}/lib/swresample.lib"
		)

		add_library(swscale SHARED IMPORTED)
		set_target_properties(swscale PROPERTIES
			IMPORTED_LOCATION "${FFMPEG_DIR}/bin/swscale-7.dll"
			IMPORTED_IMPLIB "${FFMPEG_DIR}/lib/swscale.lib"
		)

		add_library(FFmpeg INTERFACE)
		target_include_directories(FFmpeg INTERFACE
			"$<BUILD_INTERFACE:${FFMPEG_DIR}/include>"
			"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
		)
		target_link_directories(FFmpeg INTERFACE
			"$<BUILD_INTERFACE:${FFMPEG_DIR}/lib>"
			"$<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>"
		)
	endif()
	target_link_libraries(${PROJECT_NAME} PUBLIC FFmpeg)

	# # install FFmpeg
	# install(TARGETS FFmpeg
    #     EXPORT "FFmpegTargets"
	# 	FILE_SET HEADERS
    #     LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    #     ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    #     RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    #     INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	# )
	# install(EXPORT "FFmpegTargets"
    #     FILE "FFmpegTargets.cmake"
    #     NAMESPACE "${PROJECT_NAME}::"
    #     DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/FFmpeg"
	# )
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	# on linux we expect FFmpeg to be installed system-wide
	find_package(PkgConfig)
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
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

# set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE On)

# Install library
install(TARGETS ${PROJECT_NAME}
	EXPORT "${PROJECT_NAME}Targets"
	FILE_SET HEADERS
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	#INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" # NOTE: due to use of FILE_SET this is not used
)
# Install library import file
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

get_target_property(test PkgConfig::FFmpeg INTERFACE_LINK_LIBRARIES)
message("LINK_LIBRARIES: ${test}")

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