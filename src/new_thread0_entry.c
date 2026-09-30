/*
 * new_thread0_entry.c
 * ===========================================================================
 * FSP Thread 0 Entry — Main Application Execution Thread
 *
 * Runs the AIoT-MOR-ALDC application:
 * - Initializes RTC calendar
 * - Creates FreeRTOS queues and mutexes
 * - Spawns worker tasks (sensor, AI, network)
 * - Runs the LVGL display engine and UI dashboard
 * ===========================================================================
 */

#include "new_thread0.h"
#include "FreeRTOS.h"
#include "task.h"

extern void app_main(void);

void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* Launch application: RTC, queues, worker tasks, and display UI */
    app_main();

    /* Fallback idle loop (app_main's display loop runs indefinitely) */
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
