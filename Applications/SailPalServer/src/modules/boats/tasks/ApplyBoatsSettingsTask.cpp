#include "ApplyBoatsSettingsTask.h"

#include "../api/BookingManagerClient.h"

ApplyBoatsSettingsTask::ApplyBoatsSettingsTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module))
	,m_baseUrl(request.value(QStringLiteral("baseUrl")).toString())
	,m_bearerToken(request.value(QStringLiteral("bearerToken")).toString())
	,m_clearBearerToken(request.value(QStringLiteral("clearBearerToken")).toBool(false))
	,m_currency(request.value(QStringLiteral("currency")).toString())
	,m_companyId(request.value(QStringLiteral("companyId")).toString()) {
}

QJsonObject ApplyBoatsSettingsTask::process() {

	auto settings = m_module.storage().bookingManagerSettings();
	settings.baseUrl = m_baseUrl;
	settings.currency = m_currency;
	settings.companyId = m_companyId;

	if(m_clearBearerToken) {
		settings.bearerToken = "";
	} else if(!m_bearerToken.trimmed().isEmpty()) {
		settings.bearerToken = m_bearerToken;
	}

	QString errorMessage;
	if(!m_module.storage().updateBookingManagerSettings(settings, &errorMessage)) {
		throwException(errorMessage.isEmpty() ? "Can't apply boats settings." : errorMessage);
	}

	settings = m_module.storage().bookingManagerSettings();
	return QJsonObject({
		{"settings", QJsonObject({
			{"baseUrl", settings.baseUrl},
			{"currency", settings.currency},
			{"companyId", settings.companyId},
			{"tokenConfigured", !settings.bearerToken.isEmpty()},
			{"tokenRequired", BookingManagerClient::requiresBearerToken(settings.baseUrl)}
		})}
	});
}
