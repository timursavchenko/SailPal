#include "GetBoatsSettingsTask.h"

#include "../api/BookingManagerClient.h"

GetBoatsSettingsTask::GetBoatsSettingsTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module)) {
}

QJsonObject GetBoatsSettingsTask::process() {

	const auto settings = m_module.storage().bookingManagerSettings();
	return QJsonObject({
		{"baseUrl", settings.baseUrl},
		{"currency", settings.currency},
		{"companyId", settings.companyId},
		{"tokenConfigured", !settings.bearerToken.isEmpty()},
		{"tokenRequired", BookingManagerClient::requiresBearerToken(settings.baseUrl)}
	});
}
