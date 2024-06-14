#include "gdevice.h"
struct gdevice lock_gdev = {
    .dev_name = "lock",
    .key = 0x44,
    .gpio_pin = 7,
    .gpio_mode = OUTPUT,
    .gpio_status = HIGH,
    .check_face_status = 1,
    .voice_set_status = 1,
};
struct gdevice *add_lock_to_gdevice_list(struct gdevice *pgdevhead)
{//头插法
    return add_device_to_gdevice_list(pgdevhead, &lock_gdev);
};