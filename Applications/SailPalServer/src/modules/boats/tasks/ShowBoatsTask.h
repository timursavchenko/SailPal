#pragma once

#include "BoatsTask.h"

class ShowBoatsTask : public BoatsTask {

	Q_OBJECT

	public:
		ShowBoatsTask(const QJsonObject& request, Module& module);

	protected:
		virtual QJsonObject process() override;
};
