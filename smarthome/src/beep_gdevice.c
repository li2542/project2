#include "gdevice.h"
struct gdevice beep_gdev = {
    .dev_name = "beep",
    .key = 0x45,
    .gpio_pin = 9,
    .gpio_mode = OUTPUT,
    .gpio_status = HIGH,
    .check_face_status = 0,
    .voice_set_status = 1,
};
struct gdevice *add_beep_to_gdevice_list(struct gdevice *pgdevhead)
{//头插法
    return add_device_to_gdevice_list(pgdevhead, &beep_gdev);
};