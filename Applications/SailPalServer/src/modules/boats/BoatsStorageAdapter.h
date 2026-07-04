#pragma once

#include <nexus.h>

struct BookingManagerSettings {
	QString baseUrl;
	QString bearerToken;
	QString currency;
	QString companyId;
};

class BoatsStorageAdapter : public StorageAdapter {

	public:
		BoatsStorageAdapter(const QString& moduleName, const QString& moduleIconName, const QString& moduleDescription, Storage& storage);

	public:
		virtual void initialize(const QVariantHash& params = {}) override;

	public:
		BookingManagerSettings bookingManagerSettings() const;
		bool updateBookingManagerSettings(const BookingManagerSettings& settings, QString* error = nullptr) const;

	public:
		static QString defaultBookingManagerBaseUrl();
		static QString defaultBookingManagerCurrency();
};
