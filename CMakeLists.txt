cmake_minimum_required(VERSION 3.23)

project(MediaDecoder)

set(CMAKE_TLS_VERIFY TRUE)

add_library(${PROJECT_NAME} STATIC
	src/MediaDecoder.c include/MediaDecoder.h
	src/ImageResizer.c src/ImageResizer.h
	src/SoundResampler.c src/SoundResampler.h
	src/Internal.c src/Internal.h
)

	PUBLIC FILE_SET HEADERS BASE_DIRS include FILES include/${PROJECT_NAME}/MediaDecoder.h

target_include_directories(${PROJECT_NAME} PUBLIC include)

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
	endif()

	target_include_directories(${PROJECT_NAME} PUBLIC "${FFMPEG_DIR}/include")
	target_link_directories(${PROJECT_NAME} PUBLIC "${FFMPEG_DIR}/lib")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	# on linux we expect FFmpeg to be installed system-wide
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE
	"avcodec"
	#"avdevice"
	#"avfilter"
	"avformat"
	"avutil"
	"swresample"
	"swscale"
)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE On)

install(TARGETS MediaDecoder FILE_SET HEADERS)