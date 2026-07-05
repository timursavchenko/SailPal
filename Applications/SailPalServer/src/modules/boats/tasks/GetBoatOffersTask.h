#pragma once

#include "BoatsTask.h"

class GetBoatOffersTask : public BoatsTask {

	Q_OBJECT

	public:
		GetBoatOffersTask(const QJsonObject& request, Module& module);

	protected:
		virtual QJsonObject process() override;

	private:
		QString m_boatId;
		QString m_companyId;
		QString m_currency;
		QString m_kind;
		QString m_dateFrom;
		QString m_dateTo;
};
