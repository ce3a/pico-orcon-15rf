
add_executable(${CMAKE_PROJECT_NAME})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE main.c)

pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})

target_link_libraries(${CMAKE_PROJECT_NAME} pico_stdlib)

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION ${CMAKE_BUILD_TYPE})
install(FILES
    $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/$<TARGET_NAME:${CMAKE_PROJECT_NAME}>.uf2
    DESTINATION ${CMAKE_BUILD_TYPE})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:WITH_PROMPT>)
