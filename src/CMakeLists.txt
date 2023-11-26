
add_executable(${CMAKE_PROJECT_NAME})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE main.c)

pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})

target_link_libraries(${CMAKE_PROJECT_NAME} pico_stdlib)

#
# On chip debug configuration
#
find_program(OPENOCD_EXECUTABLE openocd)
if(OPENOCD_EXECUTABLE)
    add_custom_target(upload
        COMMAND ${OPENOCD_EXECUTABLE}
                -f interface/cmsis-dap.cfg
                -f target/rp2040.cfg
                -c 'adapter speed 5000'
                -c 'program $<TARGET_FILE:${CMAKE_PROJECT_NAME}> verify reset exit'
        DEPENDS ${CMAKE_PROJECT_NAME})
endif()

#
# Intallation
#
install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION ${CMAKE_BUILD_TYPE})
install(FILES
    $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/$<TARGET_NAME:${CMAKE_PROJECT_NAME}>.uf2
    DESTINATION ${CMAKE_BUILD_TYPE})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:WITH_PROMPT>)
