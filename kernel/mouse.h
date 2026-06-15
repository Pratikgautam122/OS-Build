#ifndef MOUSE_H
#define MOUSE_H

void mouse_init(void);
void mouse_get_position(int* x, int* y);
int mouse_get_buttons(void);
void mouse_render_cursor(void);

#endif
