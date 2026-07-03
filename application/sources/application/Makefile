
include $(APP_SOURCES_DIR)/application/Pages/Makefile
include $(APP_SOURCES_DIR)/application/Tasks/Makefile

CFLAGS		+= -I./$(APP_SOURCES_DIR)/application
CPPFLAGS	+= -I./$(APP_SOURCES_DIR)/application

VPATH += $(APP_SOURCES_DIR)/application

SOURCES_CPP += $(APP_SOURCES_DIR)/application/main.cpp
SOURCES_CPP += $(APP_SOURCES_DIR)/application/task_list.cpp

SOURCES += $(APP_SOURCES_DIR)/application/app_usb_video.c
SOURCES += $(APP_SOURCES_DIR)/application/newlib_errno.c
