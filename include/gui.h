#ifndef SIF_GUI_H
#define SIF_GUI_H

/* Graphical desktop; returns when user opens Terminal (ESC/5) */
void gui_run(void);

/* Task entry: GUI first, then shell (gui command re-enters desktop) */
void gui_task(void);

#endif
