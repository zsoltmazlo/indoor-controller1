#include "debug.h"
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static portMUX_TYPE mux;

void debug::init() { vPortCPUInitializeMutex(&mux); }

void debug::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    taskENTER_CRITICAL(&mux);
    vprintf(fmt, args);
    taskEXIT_CRITICAL(&mux);
    va_end(args);
}