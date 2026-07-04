#include "BoatsModule.h"

#include "tasks/ApplyBoatsSettingsTask.h"
#include "tasks/GetBoatsTask.h"
#include "tasks/GetBoatsSettingsTask.h"
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
	registerTask<GetBoatsSettingsTask, ShowBoatsTask>();
	registerTask<ApplyBoatsSettingsTask, ShowBoatsTask>();
	registerTask<GetBoatsTask, ShowBoatsTask>();

	Module::initialize(params);
}

BoatsStorageAdapter& BoatsModule::storage() {

	return m_storageAdapter;
}

const BoatsStorageAdapter& BoatsModule::storage() const {

	return m_storageAdapter;
}
