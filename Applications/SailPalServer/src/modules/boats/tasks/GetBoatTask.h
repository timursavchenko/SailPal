#pragma once

#include "BoatsTask.h"

class GetBoatTask : public BoatsTask {

	Q_OBJECT

	public:
		GetBoatTask(const QJsonObject& request, Module& module);

	protected:
		virtual QJsonObject process() override;

	private:
		QString m_boatId;
		QString m_currency;
};
