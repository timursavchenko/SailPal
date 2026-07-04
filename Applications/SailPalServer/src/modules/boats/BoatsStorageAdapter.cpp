#include "BoatsStorageAdapter.h"

namespace {

	static const QString bookingManagerSettingsTableName = "boats_booking_manager_settings";
	static const QString bookingManagerSettingsTableId = "id";
	static const QString bookingManagerSettingsTableBaseUrl = "base_url";
	static const QString bookingManagerSettingsTableBearerToken = "bearer_token";
	static const QString bookingManagerSettingsTableCurrency = "currency";
	static const QString bookingManagerSettingsTableCompanyId = "company_id";

	static const quint32 settingsRowId = 1;

	QString normalizedBaseUrl(const QString& baseUrl) {

		const auto trimmed = baseUrl.trimmed();
		return trimmed.isEmpty() ? BoatsStorageAdapter::defaultBookingManagerBaseUrl() : trimmed;
	}

	QString normalizedCurrency(const QString& currency) {

		const auto trimmed = currency.trimmed().toUpper();
		return trimmed.isEmpty() ? BoatsStorageAdapter::defaultBookingManagerCurrency() : trimmed;
	}

}

BoatsStorageAdapter::BoatsStorageAdapter(const QString& moduleName, const QString& moduleIconName, const QString& moduleDescription, Storage& storage)
 :StorageAdapter(moduleName, moduleIconName, moduleDescription, storage) {
}

void BoatsStorageAdapter::initialize(const QVariantHash& params) {

	StorageAdapter::initialize(params);

	if(!m_storage.isTableExists(bookingManagerSettingsTableName)) {
		m_storage.createTable(
			bookingManagerSettingsTableName,
			{
				{bookingManagerSettingsTableId, Storage::Column::Type::Integer, true},
				{bookingManagerSettingsTableBaseUrl, Storage::Column::Type::Text},
				{bookingManagerSettingsTableBearerToken, Storage::Column::Type::Text},
				{bookingManagerSettingsTableCurrency, Storage::Column::Type::Text},
				{bookingManagerSettingsTableCompanyId, Storage::Column::Type::Text}
			}
		);
	}

	const auto records = m_storage.select(
		"select * from " + bookingManagerSettingsTableName +
			" where " + bookingManagerSettingsTableId + " = ? limit 1",
		{settingsRowId}
	);
	if(records.isEmpty()) {
		m_storage.insertIntoTable(
			bookingManagerSettingsTableName,
			{
				{bookingManagerSettingsTableId, settingsRowId},
				{bookingManagerSettingsTableBaseUrl, defaultBookingManagerBaseUrl()},
				{bookingManagerSettingsTableBearerToken, ""},
				{bookingManagerSettingsTableCurrency, defaultBookingManagerCurrency()},
				{bookingManagerSettingsTableCompanyId, ""}
			}
		);
		return;
	}

	const auto& settingsRecord = records.first();
	const auto baseUrl = normalizedBaseUrl(settingsRecord.value(bookingManagerSettingsTableBaseUrl).toString());
	const auto bearerToken = settingsRecord.value(bookingManagerSettingsTableBearerToken).toString().trimmed();
	if(bearerToken.isEmpty() && baseUrl == QStringLiteral("https://www.booking-manager.com/api/v2")) {
		m_storage.updateTable(
			bookingManagerSettingsTableName,
			settingsRowId,
			{{bookingManagerSettingsTableBaseUrl, defaultBookingManagerBaseUrl()}}
		);
	}
}

BookingManagerSettings BoatsStorageAdapter::bookingManagerSettings() const {

	const auto records = m_storage.select(
		"select * from " + bookingManagerSettingsTableName +
			" where " + bookingManagerSettingsTableId + " = ? limit 1",
		{settingsRowId}
	);

	if(records.isEmpty()) {
		return {
			defaultBookingManagerBaseUrl(),
			"",
			defaultBookingManagerCurrency(),
			""
		};
	}

	const auto& record = records.first();
	return {
		normalizedBaseUrl(record.value(bookingManagerSettingsTableBaseUrl).toString()),
		record.value(bookingManagerSettingsTableBearerToken).toString().trimmed(),
		normalizedCurrency(record.value(bookingManagerSettingsTableCurrency).toString()),
		record.value(bookingManagerSettingsTableCompanyId).toString().trimmed()
	};
}

bool BoatsStorageAdapter::updateBookingManagerSettings(const BookingManagerSettings& settings, QString* error) const {

	const auto setError = [error](const QString& text) {
		if(error) {
			*error = text;
		}
	};

	const auto baseUrl = normalizedBaseUrl(settings.baseUrl);
	const QUrl parsedBaseUrl(baseUrl);
	if(!parsedBaseUrl.isValid() || parsedBaseUrl.scheme().isEmpty() || parsedBaseUrl.host().isEmpty()) {
		setError("Booking Manager API base URL is invalid.");
		return false;
	}

	const auto currency = normalizedCurrency(settings.currency);
	if(currency.size() != 3) {
		setError("Booking Manager currency must contain 3 letters.");
		return false;
	}

	m_storage.updateTable(
		bookingManagerSettingsTableName,
		settingsRowId,
		{
			{bookingManagerSettingsTableBaseUrl, baseUrl},
			{bookingManagerSettingsTableBearerToken, settings.bearerToken.trimmed()},
			{bookingManagerSettingsTableCurrency, currency},
			{bookingManagerSettingsTableCompanyId, settings.companyId.trimmed()}
		}
	);

	return true;
}

QString BoatsStorageAdapter::defaultBookingManagerBaseUrl() {

	return QStringLiteral("https://virtserver.swaggerhub.com/mmksystems/bm-api/2.0.0");
}

QString BoatsStorageAdapter::defaultBookingManagerCurrency() {

	return QStringLiteral("EUR");
}
