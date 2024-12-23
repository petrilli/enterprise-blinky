/**
 * @file main.c
 * @brief A somewhat complicated example of blinking an LED in Zephyr with some logging over a faux USB serial port.
 */

#include <stdio.h>
#include <zephyr/bindesc.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>


LOG_MODULE_REGISTER(main);

/// Time to sleep between blinks, where 1000 msec = 1 sec
#define SLEEP_TIME_MS   250

/// Devicetree identifier alias for the LED to blink
#define LED0_NODE DT_ALIAS(led0)


int main(void) {
    int ret;                                                                   //< General return value collector
    const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console)); //< UART for the console device
    uint32_t dtr = 0;                                                          //< DTR status
    static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios); //< Build error w/o LED defined

    /*
     * Initialize the console. Bring up the USB stack, and then wait for a connection, sleeping to give other threads
     * a chance to execute.
     */
    if (usb_enable(NULL) != 0) {
        return 0;
    }

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(250));
    }

    LOG_INF("Greetings developer! (%s @ %s)", CONFIG_BOARD, BINDESC_GET_STR(build_date_time_string));

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

    while (1) {
        ret = gpio_pin_toggle_dt(&led);

        if (ret != 0) {
            return 0;
        }

        LOG_INF("LED state: %s", gpio_pin_get_dt(&led) ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
