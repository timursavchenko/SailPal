#pragma once

#include <nexus.h>

class BoatsModule : public Module {

	public:
		BoatsModule(Storage& storage, UserSessions& userSessions);

	public:
		virtual void initialize(const QVariantHash& params = {}) override;

	private:
		StorageAdapter m_storageAdapter;
};
