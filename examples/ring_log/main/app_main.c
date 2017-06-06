#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "freertos/task.h"

#include "ring_log.h"

static wl_handle_t wl_handle = WL_INVALID_HANDLE;

void write_ring_log_task(void *);
void read_ring_log_task(void *);

void app_main() {
    // Set up the FAT partition.
    const esp_vfs_fat_mount_config_t config = {
        .max_files = 4,
        .format_if_mount_failed = true
    };
    esp_vfs_fat_spiflash_mount("/log", "log", &config, &wl_handle);
    ring_log_init();

    // Start the ring log writing and reading tasks.
    xTaskCreate(write_ring_log_task, "write_ring_log", 2048, NULL, 10, NULL);
    xTaskCreate(read_ring_log_task, "read_ring_log", 2048, NULL, 10, NULL);
}

void write_ring_log_task(void *_) {
    int i = 0;
    while (1) {
        // Write one entry to the log..
        char buffer[64];
        int len = snprintf(buffer, sizeof(buffer), "this is the %ith entry\n", i++);
        ring_log_write_tail("/log/test", buffer, len);
        // (in two parts)
        char msg[] = "you can write write_tail as many times as you like to append to the log entry in progress\n";
        ring_log_write_tail("/log/test", msg, sizeof(msg));
        ring_log_write_tail_complete("/log/test");

        // .. every second.
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void read_ring_log_task(void *_) {
    while (1) {
        // Read out all the /log/test entries ..
        puts("** reading out /log/test entries!");
        while (ring_log_has_unread("/log/test")) {
            puts("here's a /log/test entry:");
            // (16 bytes at a time)
            char buffer[16];
            size_t read_now, read_total = 0;
            while ((read_now = ring_log_read_head("/log/test", buffer, sizeof(buffer), &read_total))) {
                printf("%.*s", read_now, buffer);
            }
            puts("");
            ring_log_read_head_success("/log/test");
        }

        // .. every 5 seconds.
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
