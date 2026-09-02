# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/syscall.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/startup_stm32f407xx.S"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/gpio/gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/rcc/rcc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/nvic/nvic.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/dma/dma.c"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/stm32f407xg_flash.ld" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/stm32f407xg_flash.ld")
