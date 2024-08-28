cmake_minimum_required(VERSION 3.7)

set(SOURCE_FILES main.cpp sdl_render.cpp vmath.cpp bitfont.cpp)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(flecs CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)

add_executable(breakout ${SOURCE_FILES})
target_link_libraries(breakout
	 $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
	$<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
	$<IF:$<TARGET_EXISTS:flecs::flecs>,flecs::flecs,flecs::flecs_static>)


## Copy SDL2 DLLs to output folder on Windows
#if(WIN32)
#    foreach(DLL ${SDL2_DLLS})
#        add_custom_command(TARGET breakout POST_BUILD COMMAND
#            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:breakout>)
#    endforeach()
#	foreach(IMGDLL ${SDL2_IMAGE_DLLS}) 
#        add_custom_command(TARGET breakout POST_BUILD COMMAND
#            ${CMAKE_COMMAND} -E copy_if_different ${IMGDLL} $<TARGET_FILE_DIR:breakout>)
#    endforeach()
#endif()
