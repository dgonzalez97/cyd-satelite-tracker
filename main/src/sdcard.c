#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "driver/spi_master.h"
#include "driver/sdmmc_host.h"

#include "esp_log.h"
#include "esp_vfs_fat.h" //VFS https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/vfs.html

#include "sdmmc_cmd.h"

#include "board_pins.h"
#include "sdcard.h"

static const char *TAG = "sd_demo";

esp_err_t sdcard_demo_sdspi(void)
{
    ESP_LOGI(TAG, "SDSPI demo: using VSPI and TF card pins");

    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // assumes FAT32, don't auto-format
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t *card;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_HOST;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT; // safe default

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4 * 512, // 4 sectors -
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem at %s", MOUNT_POINT);
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. Is the card FAT32?");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card: %s", esp_err_to_name(ret));
        }
        spi_bus_free(host.slot);
        return ret;
    }

    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);

    const char *file_path = MOUNT_POINT "/DEMOTEST.txt";
    ESP_LOGI(TAG, "Creating test file: %s", file_path);

    FILE *f = fopen(file_path, "w");
    if (!f)
    {
        ESP_LOGE(TAG, " Failed to open file for writing errno=%d", errno);
    }
    else
    {
        fprintf(f, "Open SD Card FS\n");
        fclose(f);
        ESP_LOGI(TAG, "Test file written");
    }

    ESP_LOGI(TAG, "Listing files in %s", MOUNT_POINT);

    errno = 0;
    DIR *dir = opendir(MOUNT_POINT);
    if (!dir) {
        ESP_LOGE(TAG, "opendir(%s) failed, errno=%d (%s)", MOUNT_POINT, errno, strerror(errno));
        return ESP_FAIL;
    }

    errno = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        ESP_LOGI(TAG, " %s", entry->d_name);
    }

    if (errno != 0)
    {
        ESP_LOGE(TAG, "readdir(%s) failed, errno=%d (%s)", MOUNT_POINT, errno, strerror(errno));
    }

    if (closedir(dir) != 0)
    {
        ESP_LOGW(TAG, "closedir(%s) failed, errno=%d (%s)", MOUNT_POINT, errno, strerror(errno));
    }
    // Optional cleanup, not needed for demo.
    // esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    // spi_bus_free(host.slot);

    return ESP_OK;
}
