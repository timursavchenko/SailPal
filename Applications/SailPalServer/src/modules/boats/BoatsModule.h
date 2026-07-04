#pragma once

#include <nexus.h>

#include "BoatsStorageAdapter.h"

class BoatsModule : public Module {

	public:
		BoatsModule(Storage& storage, UserSessions& userSessions);

	public:
		virtual void initialize(const QVariantHash& params = {}) override;
		BoatsStorageAdapter& storage();
		const BoatsStorageAdapter& storage() const;

	private:
		BoatsStorageAdapter m_storageAdapter;
};
