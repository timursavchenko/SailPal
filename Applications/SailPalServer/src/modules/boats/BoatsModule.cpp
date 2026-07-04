#include "BoatsModule.h"

#include "tasks/ShowBoatsTask.h"

BoatsModule::BoatsModule(Storage& storage, UserSessions& userSessions)
	:Module(QLatin1String("boats"), m_storageAdapter, userSessions)
	,m_storageAdapter(
		m_moduleName,
		"fa-ship",
		QStringLiteral("Boats module stub."),
		storage
	){

	m_moduleIconName = "fa-ship";

	Q_INIT_RESOURCE(boats);
}

void BoatsModule::initialize(const QVariantHash& params) {
	registerTask<ShowBoatsTask>();

	Module::initialize(params);
}
