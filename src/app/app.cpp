#include "app/app.h"
#include "tasks/system_task.h"
#include "tasks/comm_link_task.h"
#include "tasks/notif_ui_task.h"
#include "tasks/alarms_events_task.h"

void App::init() {
    // Inicializar las tareas del sistema
    SystemTask::getInstance().init();
    CommLinkTask::getInstance().init();
    NotifUiTask::getInstance().init();
    AlarmsEventsTask::getInstance().init();
}
