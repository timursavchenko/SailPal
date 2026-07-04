#pragma once

#include "BoatsTask.h"

class ApplyBoatsSettingsTask : public BoatsTask {

	Q_OBJECT

	public:
		ApplyBoatsSettingsTask(const QJsonObject& request, Module& module);

	protected:
		virtual QJsonObject process() override;

	private:
		QString m_baseUrl;
		QString m_bearerToken;
		bool m_clearBearerToken = false;
		QString m_currency;
		QString m_companyId;
};
