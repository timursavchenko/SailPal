#include "BoatsTask.h"

BoatsTask::BoatsTask(const QJsonObject& request, BoatsModule& module)
	:Task(request)
	,m_module(module) {
}
