#pragma once

#include "BoatsTask.h"

class GetBoatsSettingsTask : public BoatsTask {

	Q_OBJECT

	public:
		GetBoatsSettingsTask(const QJsonObject& request, Module& module);

	protected:
		virtual QJsonObject process() override;
};
