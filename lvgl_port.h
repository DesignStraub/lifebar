#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif


void lvgl_port_init(void);

/* Mutex fuer LVGL-Zugriffe aus anderen Tasks (z.B. Arduino loop) */
bool lvgl_lock(int timeout_ms);
void lvgl_unlock(void);


#ifdef __cplusplus
}
#endif



#endif










