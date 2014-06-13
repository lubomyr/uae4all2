#ifndef _GP2X_UTIL_H
#define _GP2X_UTIL_H
void gp2x_init(int argc, char **argv);
unsigned long gp2x_joystick_read(int allow_usb_joy);
void gp2x_set_volume(int);
void inputmode_init();
void volumecontrol_init();
void setBatteryLED(int);
int is_overridden_button(int button);
void handle_remapped_button_down(int button);
void handle_remapped_button_up(int button);
void switch_to_hw_sdl(int first_time);
void switch_to_sw_sdl(void);
#endif
