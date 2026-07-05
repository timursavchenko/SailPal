#pragma once

#include "BoatsTask.h"

class GetBoatsTask : public BoatsTask {

	Q_OBJECT

	public:
		GetBoatsTask(const QJsonObject& request, Module& module);

	protected:
		virtual QJsonObject process() override;

	private:
		QString m_search;
		QString m_kind;
		QString m_companyName;
		QString m_homeBase;
		QString m_companyId;
		QString m_currency;
		QString m_dateFrom;
		QString m_dateTo;
		QString m_sortBy;
		QString m_sortDirection;
		int m_minCabins = 0;
		int m_minBerths = 0;
		double m_minLength = 0.0;
};