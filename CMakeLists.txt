cmake_minimum_required(VERSION 3.23)

project(MediaDecoder)

add_library(${PROJECT_NAME} STATIC
	src/MediaDecoder.c include/MediaDecoder.h
	src/ImageResizer.c src/ImageResizer.h
	src/SoundResampler.c src/SoundResampler.h
	src/Internal.c src/Internal.h
)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
	FILES include/MediaDecoder.h)

install(TARGETS MediaDecoder FILE_SET HEADERS)