#ifndef __TASK_LIST_H__
#define __TASK_LIST_H__

#include "akos.h"

enum
{
    TASK_BTN_ID,
    TASK_PAGE_MNG_ID,
    TASK_GUI_ID,

    TASK_EOT_ID,
};

extern void task_page_mng(void *p_arg);
extern void task_button(void *p_arg);
extern void task_gui(void *p_arg);

#endif /* __TASK_LIST_H__ */
