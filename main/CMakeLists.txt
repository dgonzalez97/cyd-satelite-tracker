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