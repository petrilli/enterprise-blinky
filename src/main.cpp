/**
 * @file main.cpp
 * @brief A somewhat complicated example of blinking an LED in Zephyr with some logging over a faux USB serial port.
 *
 * A few of the things that are demonstrsated are GPIO, logging, USB, random numbers, and finally NVS storage for a
 * reboot counter.
 */
#include <zephyr/bindesc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/usb/usb_device.h>


LOG_MODULE_REGISTER(main);

/// Minimum time to sleep between state changes in milliseconds
#define BASE_SLEEP_TIME_MS 100

/// Maximum time to sleep between blinks is BASE_SLEEP_TIME_MS + MAX_SLEEP_TIME_MS
#define MAX_SLEEP_TIME_MS 1001

/// Devicetree identifier alias for the LED to blink
#define LED0_NODE DT_ALIAS(led0)

#define NVS_PARTITION storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define NVS_DATA_REBOOT_COUNT_ID 1

static void input_cb([[maybe_unused]] input_event *event, [[maybe_unused]] void *user_data) {
    LOG_INF("Got input event: %d %d %s", event->type, event->code, (event->value == 1) ? "DOWN" : "UP");
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

/**
 *
 * @param fs nvs_fs struct to be initialized
 * @return -1 on error, 0 otherwise
 */
int nvs_initialize(nvs_fs *fs) {
    flash_pages_info page_info = {}; //< Page info for NVS
    int ret;                                //< Default return collector

    /* Define the NFS filesystem */
    fs->flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(fs->flash_device)) {
        LOG_ERR("Flash device not ready");
        return -1;
    } else {
        LOG_INF("Flash device ready");
    }
    fs->offset = NVS_PARTITION_OFFSET;
    ret = flash_get_page_info_by_offs(fs->flash_device, fs->offset, &page_info);
    if (ret > 0) {
        LOG_ERR("Failed to get page info for NVS (return = %d)", ret);
        return -1;
    }
    fs->sector_size = page_info.size;
    fs->sector_count = 2U;

    ret = nvs_mount(fs);
    if (ret > 0) {
        LOG_ERR("Failed to mount NVS (return = %d)", ret);
        return -1;
    }
    return 0;
}

/**
 * Generate a random delay for use later
 *
 * @return random delay between 0 and 999
 */
uint16_t random_delay_get() {
    const uint32_t rand_val = sys_rand32_get(); // Const only for this function call
    return static_cast<uint16_t>(rand_val % MAX_SLEEP_TIME_MS) + BASE_SLEEP_TIME_MS;
}

int main(void) {
    int ret;                                                                //< General return value collector
    const device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));     //< UART for the console device
    uint32_t dtr = 0U;                                                      //< DTR status
    uint32_t reboot_count = 0U;                                             //< Reboot counter
    static nvs_fs fs;                                                       //< NVS filesystem
    static constexpr gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios); //< Build error w/o LED defined

    /*
     * Initialize the console. Bring up the USB stack, and then wait for a connection, sleeping to give other threads
     * a chance to execute.
     */
    if (usb_enable(nullptr) != 0) {
        return 0;
    }

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(250));
    }

    LOG_INF("Greetings developer! (%s @ %s)", CONFIG_BOARD, BINDESC_GET_STR(build_date_time_string));

    ret = nvs_initialize(&fs);
    if (ret < 0) {
        LOG_ERR("Failed to initialize NVS");
        return 0;
    }

    /* Get the reboot count */
    ret = nvs_read(&fs, NVS_DATA_REBOOT_COUNT_ID, &reboot_count, sizeof(reboot_count));
    if (ret > 0) {
        LOG_INF("Reboot count: %d at id=%d", reboot_count, NVS_DATA_REBOOT_COUNT_ID);
        reboot_count++;
        LOG_INF("Setting reboot count to %d at id=%d", reboot_count, NVS_DATA_REBOOT_COUNT_ID);
        (void) nvs_write(&fs, NVS_DATA_REBOOT_COUNT_ID, &reboot_count, sizeof(reboot_count));
        // XXX Do something with return
    } else {
        LOG_INF("No reboot count found at id=%d. Initializing to 0.", NVS_DATA_REBOOT_COUNT_ID);
        (void) nvs_write(&fs, NVS_DATA_REBOOT_COUNT_ID, &reboot_count, sizeof(reboot_count));
        // XXX Do something with return
    }

    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED device not ready");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED pin");
        return 0;
    }

    LOG_INF("Starting the blinking loop");

    while (true) {
        ret = gpio_pin_toggle_dt(&led);

        if (ret != 0) {
            return 0;
        }
        uint16_t delay = random_delay_get();
        LOG_INF("LED state: %s (%dms)", gpio_pin_get_dt(&led) ? "ON" : "OFF", delay);
        k_msleep(delay);
    }
    return 0;
}
