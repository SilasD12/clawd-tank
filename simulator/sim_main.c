#include "lvgl.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    lv_init();
    printf("LVGL initialized. Simulator build works!\n");
    return 0;
}
