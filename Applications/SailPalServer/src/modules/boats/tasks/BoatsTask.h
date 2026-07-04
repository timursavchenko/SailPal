#pragma once

#include "../BoatsModule.h"

class BoatsTask : public Task {

	Q_OBJECT

	public:
		BoatsTask(const QJsonObject& request, BoatsModule& module);

	protected:
		BoatsModule& m_module;
};
