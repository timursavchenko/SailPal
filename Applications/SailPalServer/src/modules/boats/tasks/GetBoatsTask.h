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
		QString m_companyId;
		QString m_currency;
};