file(GLOB_RECURSE SOURCES
    "src/*.c"
    "orbits/*.c"
    "orbits/*.cpp"
)

idf_component_register(
    SRCS
        ${SOURCES}
    INCLUDE_DIRS
        "inc"
        "orbits"
    REQUIRES
        esp_lcd
        esp_lvgl_port
        esp_timer
        fatfs
        sdmmc
        driver
)

target_link_libraries(${COMPONENT_LIB} PRIVATE perturb)

lvgl_port_create_c_image("images/world_480x320.png" "images/" "RGB565" "NONE")
lvgl_port_add_images(${COMPONENT_LIB} "images/")
