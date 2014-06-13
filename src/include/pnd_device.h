
#ifndef h_pnd_device_h
#define h_pnd_device_h

#ifdef __cplusplus
extern "C" {
#endif

// do we have a 'minimal' lib yet anywhere formalized? if not, we could
// attempt to include it here.

// for reference material, see the Pandora wiki. Specifically, some good
// places to look:
// http://pandorawiki.org/Kernel_interface

/* for reference, here are some devices of use; most of these should already have
 * functions below, to avoid you having to code your own.
 */
#define PND_DEVICE_PROC_CLOCK "/proc/pandora/cpu_mhz_max"
#define PND_DEVICE_SYS_BACKLIGHT_BRIGHTNESS "/sys/class/backlight/gpio-backlight/brightness"
#define PND_DEVICE_FRAMEBUFFER "/dev/fb0"
#define PND_DEVICE_NUB1 "/dev/input/js1"
#define PND_DEVICE_NUB2 "/dev/input/js2"
#define PND_DEVICE_BATTERY_GAUGE_PERC "/sys/class/power_supply/bq27500-0/capacity"
#define PND_DEVICE_CHARGE_CURRENT "/sys/class/power_supply/bq27500-0/current_now"

#define PND_DEVICE_LED_CHARGER "/sys/class/leds/pandora::charger"
#define PND_DEVICE_LED_POWER   "/sys/class/leds/pandora::power"
#define PND_DEVICE_LED_SD1     "/sys/class/leds/pandora::sd1"
#define PND_DEVICE_LED_SD2     "/sys/class/leds/pandora::sd2"
#define PND_DEVICE_LED_WIFI    "/sys/class/leds/pandora::wifi"
#define PND_DEVICE_LED_BT      "/sys/class/leds/pandora::bluetooth"
#define PND_DEVICE_LED_SUFFIX_BRIGHTNESS "/brightness"

// device names
#define PND_EVDEV_NUB1    "nub0" /*"vsense66"*/
#define PND_EVDEV_NUB2    "nub1" /*"vsense67"*/
#define PND_EVDEV_KEYPAD  "keypad" /*"omap_twl4030keypad"*/
#define PND_EVDEV_GPIO    "gpio-keys"
#define PND_EVDEV_TS      "touchscreen" /*"ADS784x Touchscreen"*/
#define PND_EVDEV_POWER   "power-button" /*"triton2-pwrbutton"*/

/* utility
 */
unsigned char pnd_device_open_write_close ( char *name, char *v );
unsigned char pnd_device_open_read_close ( char *name, char *r_buffer, unsigned int buffer_len );

/* overall clock speed
 * WARN: No boundaries are checked, so try to avoid setting clock to 2GHz :)
 * NOTE: get-clock() is not implemented yet.
 */
unsigned char pnd_device_set_clock ( unsigned int c ); // returns >0 on success
unsigned int pnd_device_get_clock ( void );

/* return the battery current %age level; 0-100%
 * On error, returns -1
 */
int pnd_device_get_battery_gauge_perc ( void );
unsigned char pnd_device_get_charge_current ( int *result ); // returns + - current; if charging, current is +ve.

// LCD to set on/off

// Backlight control
unsigned char pnd_device_set_backlight ( unsigned int v ); // value to set; 0 is off
unsigned int pnd_device_get_backlight ( void );

// set one or more LEDs on
unsigned char pnd_device_set_led_power_brightness ( unsigned char v ); // 0-255
unsigned char pnd_device_set_led_charger_brightness ( unsigned char v ); // 0-255

// suspend/hibernate/etc

#ifdef __cplusplus
} /* "C" */
#endif

#endif
