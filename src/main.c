
#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

#define RF_PIN_LEVEL_1          1u
#define RF_PIN_LEVEL_2          2u
#define RF_PIN_LEVEL_3          3u
#define RF_PIN_ABSENT           4u
#define RF_PIN_AUTO             5u
#define RF_PIN_TIME             6u

#define RF_DELAY_SHORT_PULSE  250u  /* ms */
#define RF_DELAY_INTER_PULSE  250u  /* ms */

struct rf_command {
    char key;           /**< Command invocation character */
    uint gpio;          /**< GPIO emulating a button press */
    uint nump;          /**< Number of button presses */
    const char* help;
};

enum rf_result {
    RF_OK,              /**< Success */
    RF_E_CMD,           /**< Invalid command */
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

static const uint rf_num_pins = sizeof(rf_pins) / sizeof(rf_pins[0]);
static const uint rf_num_cmds = sizeof(rf_cmds) / sizeof(rf_cmds[0]);

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
}

static void rf_trigger(uint gpio, uint nump)
{
    /* Emulating open-drain output. */
    for (uint i = 0; i < nump; i++) {
        gpio_set_oeover(gpio, GPIO_OVERRIDE_HIGH);  /* enable output -> low */
        sleep_ms(RF_DELAY_SHORT_PULSE);
        gpio_set_oeover(gpio, GPIO_OVERRIDE_LOW);   /* disable output -> Z state */
        sleep_ms(RF_DELAY_INTER_PULSE);
    }
}

static enum rf_result rf_execute(char key)
{
    enum rf_result ret = RF_E_CMD;

    for (uint i = 0; i < rf_num_cmds; i++) {
        if (rf_cmds[i].key == key) {
            printf("%c: %s\n", key, rf_cmds[i].help);
            rf_trigger(rf_cmds[i].gpio, rf_cmds[i].nump);
            ret = RF_OK;
            break;
        }
    }

    return ret;
}

static void print_help(void)
{
    for (uint i = 0; i < rf_num_cmds; i++) {
        printf("%c: %s\n", rf_cmds[i].key, rf_cmds[i].help);
    }
    printf("?: Print this help message\n");
}

int main(void)
{
    stdio_init_all();
    rf_init_pins();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    printf("ORCON 15RF button pusher\n");
    printf("  Press ? for help.\n");

    while (true) {
        gpio_put(LED_PIN, 0);
        printf("> ");
        char c = getchar();
        printf("%c\n", c);
        gpio_put(LED_PIN, 1);

        if (c == '?') {
            print_help();
        } else {
            enum rf_result ret = rf_execute(c);
            switch (ret) {
            case RF_OK:
                break;
            case RF_E_CMD:
                printf("error: unknown command\n");
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
    }

    return 0;
}
