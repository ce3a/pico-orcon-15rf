
#include <stdio.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

#define RF_PIN_LEVEL_1              1u
#define RF_PIN_LEVEL_2              2u
#define RF_PIN_LEVEL_3              3u
#define RF_PIN_ABSENT               4u
#define RF_PIN_AUTO                 5u
#define RF_PIN_TIME                 6u

#define RF_LED_PIN_RED             26u
#define RF_LED_PIN_GREEN           27u

#define RF_DELAY_SHORT_PULSE      250u  /* ms */
#define RF_DELAY_INTER_PULSE      250u  /* ms */

#define RF_DELAY_INDICATION      1000u  /* ms */
#define RF_DELAY_ERROR           8000u  /* ms */

struct rf_command {
    char key;           /**< Command invocation character */
    uint gpio;          /**< GPIO emulating a button press */
    uint nump;          /**< Number of button presses */
    const char* help;
};

struct rf_response {
    uint green;         /**< Number of green LED pules */
    uint red;           /**< Number of red LED pules */
};

enum rf_result {
    RF_OK,              /**< Success */
    RF_E_RSP,           /**< Unexpected response (LED indication) */
    RF_E_COM,           /**< RF communication error */
    RF_E_DEV            /**< Device error */
};

static const uint rf_pins[] = {
    RF_PIN_ABSENT,      /**< GPIO for Absent mode */
    RF_PIN_AUTO,        /**< GPIO for Auto mode */
    RF_PIN_TIME,        /**< GPIO for Timer mode */
    RF_PIN_LEVEL_1,     /**< GPIO for Mode low(1) mode */
    RF_PIN_LEVEL_2,     /**< GPIO for Mode mid(2) mode */
    RF_PIN_LEVEL_3,     /**< GPIO for Mode high(3) mode */
};

static const uint rf_leds[] = {
    RF_LED_PIN_RED,     /**< GPIO for Red LED */
    RF_LED_PIN_GREEN    /**< GPIO for Green LED */
};

static const struct rf_command rf_cmds[] = {
    {'1', RF_PIN_LEVEL_1, 1, "Low power mode"},
    {'2', RF_PIN_LEVEL_2, 1, "Mid power mode"},
    {'3', RF_PIN_LEVEL_3, 1, "High power mode"},
    {'a', RF_PIN_AUTO,    1, "Automatic mode"},
    {'b', RF_PIN_TIME,    1, "High power mode for 15 minutes"},
    {'c', RF_PIN_TIME,    2, "High power mode for 30 minutes"},
    {'d', RF_PIN_TIME,    3, "High power mode for 60 minutes"},
    {'e', RF_PIN_ABSENT,  1, "Absence mode"}
};

struct rf_response rf_rsp;

static const uint rf_num_pins = sizeof(rf_pins) / sizeof(rf_pins[0]);
static const uint rf_num_leds = sizeof(rf_leds) / sizeof(rf_leds[0]);
static const uint rf_num_cmds = sizeof(rf_cmds) / sizeof(rf_cmds[0]);


void rf_on_led(uint gpio, uint32_t events) {

    (void)events;

    if (gpio == RF_LED_PIN_GREEN)
        ++rf_rsp.green;
    else
        ++rf_rsp.red;
}


static void rf_init_pins(void)
{
    for (uint i = 0; i < rf_num_pins; i++) {
        uint gpio = rf_pins[i];
        /* Configure the button press emulating GPIOs as open-drain. */
        gpio_init(gpio);
        gpio_disable_pulls(gpio);                   /* disable pulls */
        gpio_set_oeover(gpio, GPIO_OVERRIDE_LOW);   /* disable output -> Z state */
        gpio_set_outover(gpio, GPIO_OVERRIDE_LOW);  /* force output low */
        gpio_set_dir(gpio, GPIO_OUT);               /* configure as output */
    }

    for (uint i = 0; i < rf_num_leds; i++) {
        uint gpio = rf_leds[i];
        /* Configure GPIOs for monitoring the LEDs. */
        gpio_init(gpio);
        gpio_set_pulls(gpio, true, false);          /* enable pull-up only*/
        gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL, true,
                &rf_on_led);
    }
}

static void rf_trigger(uint cmd)
{
    uint nump = rf_cmds[cmd].nump;
    uint gpio = rf_cmds[cmd].gpio;

    /* Emulating open-drain output. */
    for (uint i = 0; i < nump; i++) {
        gpio_set_oeover(gpio, GPIO_OVERRIDE_HIGH);  /* enable output -> low */
        sleep_ms(RF_DELAY_SHORT_PULSE);
        gpio_set_oeover(gpio, GPIO_OVERRIDE_LOW);   /* disable output -> Z state */
        sleep_ms(RF_DELAY_INTER_PULSE);
    }
}

static enum rf_result rf_get_response(uint cmd)
{
    if (rf_rsp.red > 2)
        return RF_E_COM;

    if (rf_rsp.red > 1)
        return RF_E_DEV;

    if (rf_rsp.green != (2 * rf_cmds[cmd].nump))
        return RF_E_RSP;

    return RF_OK;
}

static void rf_execute(char key)
{
    uint cmd;
    enum rf_result ret;

    /* Clear previous response.*/
    rf_rsp.green = 0;
    rf_rsp.red = 0;

    /* Find command. */
    for (cmd = 0; cmd < rf_num_cmds; cmd++)
        if (rf_cmds[cmd].key == key)
            break;

    if (cmd == rf_num_cmds) {
        printf("error: unknown command\n");
        return;
    }

    printf("request: %s\n", rf_cmds[cmd].help);

    /* Press the button. */
    rf_trigger(cmd);

    /* Wait for confirmation. */
    sleep_ms(RF_DELAY_INDICATION * (rf_cmds[cmd].nump + 1));

    /* Get response. */
    ret = rf_get_response(cmd);

    /* Something went wrong, wait for the error indication. */
    if (ret != RF_OK) {
        sleep_ms(RF_DELAY_ERROR);
        ret = rf_get_response(cmd);
    }

    switch (ret) {
    case RF_OK:
        printf("state: %s\n", rf_cmds[cmd].help);
        break;
    case RF_E_RSP:
        printf("error: command not confirmed\n");
        break;
    case RF_E_COM:
        printf("error: device unreachable\n");
        break;
    case RF_E_DEV:
        printf("error: device error\n");
        break;
    default:
        ; /* should never happen */
    }
}

static void print_help(void)
{
    for (uint i = 0; i < rf_num_cmds; i++) {
        printf("%c: %s\n", rf_cmds[i].key, rf_cmds[i].help);
    }
    printf("u: Reboot to BOOTSEL mode\n");
    printf("?: Print this help message\n");
}

char wait_for_input(void)
{
    char c;
    gpio_put(LED_PIN, 0);
    printf("> ");
    c = getchar();
    printf("%c\n", c);
    gpio_put(LED_PIN, 1);

    return c;
}

int main(void)
{
    stdio_init_all();
    rf_init_pins();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        char c = wait_for_input();
        if (c == '?') {
            print_help();
        } else if (c == 'u') {
            printf("rebooting to BOOTSEL mode...");
            reset_usb_boot(0, 0);
        } else if (isalnum(c)) {
            rf_execute(c);
        } else {
            /* Ignore non alphanumeric characters. */
        }
    }

    return 0;
}
