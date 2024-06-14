#include "gdevice.h"

struct gdevice bled_gdev = {
    .dev_name = "BR led",//卧室灯
    .key = 0x42,
    .gpio_pin = 5,
    .gpio_mode = OUTPUT,
    .gpio_status = HIGH,
    .check_face_status = 0,
    .voice_set_status = 0,
};
struct gdevice *add_bled_to_gdevice_list(struct gdevice *pgdevhead)
{//头插法
    return add_device_to_gdevice_list(pgdevhead, &bled_gdev);
};