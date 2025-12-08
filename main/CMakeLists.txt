file(GLOB_RECURSE SOURCES "src/*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "inc"
    REQUIRES
        esp_lcd
        esp_timer
        fatfs
        sdmmc
        driver
)

lvgl_port_create_c_image("images/world_380x240.png" "images/""RGB565" "NONE")
lvgl_port_add_images(${COMPONENT_LIB} "images/")