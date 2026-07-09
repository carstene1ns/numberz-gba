cmake_minimum_required(VERSION 3.20...4.3 FATAL_ERROR)

# enforce GBA compilation
set(DKP_GBA_PLATFORM_LIBRARY "libtonc")
set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/cmake/GBA.cmake")

project(numberz VERSION 1.1.0 LANGUAGES C ASM)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type is set to ${CMAKE_BUILD_TYPE}.")

#option(NO_MUSIC "Build without music" OFF)

# exe
add_executable(numberz
	ext/posprintf.h ext/posprintf.s
	src/audio.c src/audio.h
	src/credits.c src/credits.h
	src/game.c src/game.h
	src/input.c src/input.h
	src/intro.c src/intro.h
	src/levels.h
	src/main.c src/main.h
	src/menu.c src/menu.h)
target_include_directories(numberz PRIVATE ext)
target_compile_features(numberz PRIVATE c_std_11)
target_compile_definitions(numberz PRIVATE VERSION="${PROJECT_VERSION}")
target_compile_options(numberz PRIVATE
	-Wall -Wextra
	$<$<CONFIG:Release,RelWithDebInfo>:-mlong-calls -fomit-frame-pointer -ffast-math>)

# gfx
grit_add_binary_target(bg-game gfx/bg-game.png OPTIONS -mLs -mR4 -gzl -mzl -pn15) # sbb map, 32x32t@4, lz77
grit_add_binary_target(bg-intro gfx/bg-intro.png OPTIONS -mLs -mR4 -gzl -mzl -pn15) # sbb map, 32x32t@4, lz77
grit_add_binary_target(ts-game gfx/ts-game.png NO_MAP OPTIONS -Mw4 -Mh4 -gzl -pn16) # 4bpp tiles, lz77
dkp_add_embedded_binary_library(numberz-graphics bg-game bg-intro ts-game)

# sfx
mm_add_soundbank_target(soundbank
	HEADER "soundbank.h"
	INPUTS
		sfx/ingame.mod
		sfx/intro.mod
		sfx/animate.wav
		sfx/deselect.wav
		sfx/fail.wav
		sfx/select.wav
		sfx/win.wav)
dkp_add_embedded_binary_library(numberz-sounds soundbank)
target_include_directories(numberz-sounds INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

# libs
add_library(maxmod STATIC IMPORTED)
set_target_properties(maxmod PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "$ENV{DEVKITPRO}/libgba/include"
	IMPORTED_LOCATION "$ENV{DEVKITPRO}/libgba/lib/libmm.a")
target_link_libraries(numberz PRIVATE numberz-graphics numberz-sounds maxmod)

# debug
dkp_target_generate_symbol_list(numberz)

# rom
gba_create_rom(numberz
	TITLE "Numberz GBA"
	GAMECODE "NUMZ"
	MAKERCODE "CT")

# source code formatting
if(NOT ASTYLE_FOUND)
	find_program(ASTYLE_EXECUTABLE astyle)
	if(ASTYLE_EXECUTABLE)
		message(STATUS "Found source code formatter: astyle")
		set(ASTYLE_FOUND 1 CACHE INTERNAL "Astyle has been found")
	endif()
endif()
if(ASTYLE_EXECUTABLE)
	separate_arguments(ASTYLE_OPTIONS UNIX_COMMAND
		"--options=none --project=none --suffix=none --style=attach
		--indent=spaces=4 --indent-cases --indent-preproc-block
		--min-conditional-indent=0 --squeeze-lines=1 --max-code-length=120
		--pad-comma --pad-header --unpad-paren --align-pointer=name")
	add_custom_target(source-format
		COMMAND ${ASTYLE_EXECUTABLE} ${ASTYLE_OPTIONS}
			"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
		COMMENT "Applying source code format..." VERBATIM)
endif()
