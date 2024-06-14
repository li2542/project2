#include "gdevice.h"

struct gdevice lrled_gdev = {
    .dev_name = "LV led", //客厅灯
    .key = 0x41,
    .gpio_pin = 2,
    .gpio_mode = OUTPUT,
    .gpio_status = HIGH,
    .check_face_status = 0,
    .voice_set_status = 0,
};
struct gdevice *add_lrled_to_gdevice_list(struct gdevice *pgdevhead)
{//头插法
    return add_device_to_gdevice_list(pgdevhead, &lrled_gdev);
}