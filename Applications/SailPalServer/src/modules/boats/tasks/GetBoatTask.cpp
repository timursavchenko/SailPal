#include "GetBoatTask.h"

#include "../api/BookingManagerClient.h"

namespace {

	QString jsonString(const QJsonObject& object, const QString& key) {
		const auto value = object.value(key);
		if(value.isString()) {
			return value.toString();
		}
		if(value.isDouble()) {
			return QString::number(value.toDouble());
		}
		return {};
	}

	int jsonInt(const QJsonObject& object, const QString& key) {
		return object.value(key).toInt();
	}

	double jsonDouble(const QJsonObject& object, const QString& key) {
		return object.value(key).toDouble();
	}

	QString firstImageUrl(const QJsonObject& yacht) {
		const auto images = yacht.value(QStringLiteral("images")).toArray();
		for(const auto& imageValue : images) {
			const auto image = imageValue.toObject();
			const auto url = image.value(QStringLiteral("url")).toString().trimmed();
			if(!url.isEmpty()) {
				return url;
			}
		}
		return {};
	}

	QJsonArray imageListJson(const QJsonObject& yacht) {
		QJsonArray imagesJson;
		for(const auto& imageValue : yacht.value(QStringLiteral("images")).toArray()) {
			const auto image = imageValue.toObject();
			imagesJson.append(QJsonObject({
				{QStringLiteral("url"), image.value(QStringLiteral("url")).toString()},
				{QStringLiteral("description"), image.value(QStringLiteral("description")).toString()},
				{QStringLiteral("sortOrder"), image.value(QStringLiteral("sortOrder")).toInt()}
			}));
		}
		return imagesJson;
	}

	QJsonArray descriptionListJson(const QJsonObject& yacht) {
		QJsonArray descriptionsJson;
		for(const auto& descriptionValue : yacht.value(QStringLiteral("descriptions")).toArray()) {
			const auto description = descriptionValue.toObject();
			const auto text = description.value(QStringLiteral("text")).toString().trimmed();
			if(text.isEmpty()) {
				continue;
			}
			descriptionsJson.append(QJsonObject({
				{QStringLiteral("category"), description.value(QStringLiteral("category")).toString()},
				{QStringLiteral("text"), text}
			}));
		}
		return descriptionsJson;
	}

	QJsonArray productsJson(const QJsonObject& yacht) {
		QJsonArray products;
		for(const auto& productValue : yacht.value(QStringLiteral("products")).toArray()) {
			const auto product = productValue.toObject();
			QJsonArray extras;
			for(const auto& extraValue : product.value(QStringLiteral("extras")).toArray()) {
				const auto extra = extraValue.toObject();
				extras.append(QJsonObject({
					{QStringLiteral("name"), extra.value(QStringLiteral("name")).toString()},
					{QStringLiteral("price"), extra.value(QStringLiteral("price")).toDouble()},
					{QStringLiteral("currency"), extra.value(QStringLiteral("currency")).toString()},
					{QStringLiteral("obligatory"), extra.value(QStringLiteral("obligatory")).toBool(false)},
					{QStringLiteral("unit"), extra.value(QStringLiteral("unit")).toString()},
					{QStringLiteral("payableInBase"), extra.value(QStringLiteral("payableInBase")).toBool(false)}
				}));
			}
			products.append(QJsonObject({
				{QStringLiteral("name"), product.value(QStringLiteral("name")).toString()},
				{QStringLiteral("isDefaultProduct"), product.value(QStringLiteral("isDefaultProduct")).toBool(false)},
				{QStringLiteral("crewedByDefault"), product.value(QStringLiteral("crewedByDefault")).toBool(false)},
				{QStringLiteral("extras"), extras}
			}));
		}
		return products;
	}

}

GetBoatTask::GetBoatTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module))
	,m_boatId(request.value(QStringLiteral("boatId")).toString())
	,m_currency(request.value(QStringLiteral("currency")).toString(m_module.storage().bookingManagerSettings().currency)) {
}

QJsonObject GetBoatTask::process() {

	const auto normalizedBoatId = m_boatId.trimmed();
	if(normalizedBoatId.isEmpty()) {
		throwException(QStringLiteral("Boat id is not provided."));
	}

	const auto settings = m_module.storage().bookingManagerSettings();
	BookingManagerClient client(settings.baseUrl, settings.bearerToken);
	const auto response = client.getYacht(normalizedBoatId, m_lang, m_currency);
	if(!response.ok) {
		throwException(response.errorMessage);
	}

	const auto yacht = response.payload.toObject();
	return QJsonObject({
		{QStringLiteral("boat"), QJsonObject({
			{QStringLiteral("id"), jsonString(yacht, QStringLiteral("id"))},
			{QStringLiteral("name"), jsonString(yacht, QStringLiteral("name"))},
			{QStringLiteral("model"), jsonString(yacht, QStringLiteral("model"))},
			{QStringLiteral("kind"), jsonString(yacht, QStringLiteral("kind"))},
			{QStringLiteral("year"), jsonInt(yacht, QStringLiteral("year"))},
			{QStringLiteral("company"), jsonString(yacht, QStringLiteral("company"))},
			{QStringLiteral("companyId"), jsonString(yacht, QStringLiteral("companyId"))},
			{QStringLiteral("homeBase"), jsonString(yacht, QStringLiteral("homeBase"))},
			{QStringLiteral("homeBaseId"), jsonString(yacht, QStringLiteral("homeBaseId"))},
			{QStringLiteral("shipyardId"), jsonString(yacht, QStringLiteral("shipyardId"))},
			{QStringLiteral("draught"), jsonDouble(yacht, QStringLiteral("draught"))},
			{QStringLiteral("beam"), jsonDouble(yacht, QStringLiteral("beam"))},
			{QStringLiteral("length"), jsonDouble(yacht, QStringLiteral("length"))},
			{QStringLiteral("waterCapacity"), jsonDouble(yacht, QStringLiteral("waterCapacity"))},
			{QStringLiteral("fuelCapacity"), jsonDouble(yacht, QStringLiteral("fuelCapacity"))},
			{QStringLiteral("engine"), jsonString(yacht, QStringLiteral("engine"))},
			{QStringLiteral("deposit"), jsonDouble(yacht, QStringLiteral("deposit"))},
			{QStringLiteral("currency"), jsonString(yacht, QStringLiteral("currency"))},
			{QStringLiteral("cabins"), jsonInt(yacht, QStringLiteral("cabins"))},
			{QStringLiteral("berths"), jsonInt(yacht, QStringLiteral("berths"))},
			{QStringLiteral("heads"), jsonInt(yacht, QStringLiteral("wc"))},
			{QStringLiteral("imageUrl"), firstImageUrl(yacht)},
			{QStringLiteral("images"), imageListJson(yacht)},
			{QStringLiteral("descriptions"), descriptionListJson(yacht)},
			{QStringLiteral("products"), productsJson(yacht)}
		})}
	});
}
