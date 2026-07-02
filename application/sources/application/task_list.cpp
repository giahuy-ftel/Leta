#include "task_list.h"

AKOS_THREAD_DEFINE(task_button_desc, TASK_BTN_ID, task_button, NULL, 0u, 0u, 100u);
AKOS_THREAD_DEFINE(task_page_mng_desc, TASK_PAGE_MNG_ID, task_page_mng, NULL, 1u, 8u, 300u);
AKOS_THREAD_DEFINE(task_gui_desc, TASK_GUI_ID, task_gui, NULL, 2u, 8u, 100u);
