#include "ShowBoatsTask.h"

ShowBoatsTask::ShowBoatsTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module)) {
}

QJsonObject ShowBoatsTask::process() {
	return {};
}
