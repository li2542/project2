#ifndef __GARBAGE__H
#define __GARBAGE__H
void garbage_init();
void garbage_final();
char *garbage_category(char *category);

#define WGET_CMD "wget http://127.0.0.1:8080/?action=snapshot -O /tmp/garbage.jpg"
#define GARBAGE_FILE "/tmp/garbage.jpg"
#endif
