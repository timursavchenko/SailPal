#include "GetBoatOffersTask.h"

#include "../api/BookingManagerClient.h"

namespace {

	QJsonObject offerSummaryJson(const QJsonObject& offer) {
		return QJsonObject({
			{QStringLiteral("yachtId"), offer.value(QStringLiteral("yachtId")).toVariant().toString()},
			{QStringLiteral("yacht"), offer.value(QStringLiteral("yacht")).toString()},
			{QStringLiteral("startBase"), offer.value(QStringLiteral("startBase")).toString()},
			{QStringLiteral("endBase"), offer.value(QStringLiteral("endBase")).toString()},
			{QStringLiteral("dateFrom"), offer.value(QStringLiteral("dateFrom")).toString()},
			{QStringLiteral("dateTo"), offer.value(QStringLiteral("dateTo")).toString()},
			{QStringLiteral("product"), offer.value(QStringLiteral("product")).toString()},
			{QStringLiteral("price"), offer.value(QStringLiteral("price")).toDouble()},
			{QStringLiteral("currency"), offer.value(QStringLiteral("currency")).toString()},
			{QStringLiteral("startPrice"), offer.value(QStringLiteral("startPrice")).toDouble()},
			{QStringLiteral("discountPercentage"), offer.value(QStringLiteral("discountPercentage")).toDouble()}
		});
	}

}

GetBoatOffersTask::GetBoatOffersTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module))
	,m_boatId(request.value(QStringLiteral("boatId")).toString())
	,m_companyId(request.value(QStringLiteral("companyId")).toString(m_module.storage().bookingManagerSettings().companyId))
	,m_currency(request.value(QStringLiteral("currency")).toString(m_module.storage().bookingManagerSettings().currency))
	,m_kind(request.value(QStringLiteral("kind")).toString())
	,m_dateFrom(request.value(QStringLiteral("dateFrom")).toString())
	,m_dateTo(request.value(QStringLiteral("dateTo")).toString()) {
}

QJsonObject GetBoatOffersTask::process() {

	if(m_dateFrom.trimmed().isEmpty() || m_dateTo.trimmed().isEmpty()) {
		return QJsonObject({
			{QStringLiteral("offers"), QJsonArray()},
			{QStringLiteral("hasOffer"), false},
			{QStringLiteral("datesApplied"), false}
		});
	}

	const auto settings = m_module.storage().bookingManagerSettings();
	BookingManagerClient client(settings.baseUrl, settings.bearerToken);
	BookingManagerClient::OfferFilters filters;
	filters.dateFrom = m_dateFrom;
	filters.dateTo = m_dateTo;
	filters.currency = m_currency;
	filters.companyId = m_companyId;
	filters.yachtId = m_boatId;
	filters.kind = m_kind;

	const auto response = client.getOffers(filters);
	if(!response.ok) {
		throwException(response.errorMessage);
	}

	QJsonArray offersJson;
	QJsonObject bestOffer;
	bool hasOffer = false;
	const auto normalizedBoatId = m_boatId.trimmed();

	for(const auto& offerValue : response.payload.toArray()) {
		const auto offer = offerValue.toObject();
		const auto offerYachtId = offer.value(QStringLiteral("yachtId")).toVariant().toString().trimmed();
		if(!normalizedBoatId.isEmpty() && !offerYachtId.isEmpty() && offerYachtId != normalizedBoatId) {
			continue;
		}

		const auto offerJson = offerSummaryJson(offer);
		offersJson.append(offerJson);
		if(!hasOffer) {
			bestOffer = offerJson;
			hasOffer = true;
		}
	}

	return QJsonObject({
		{QStringLiteral("offers"), offersJson},
		{QStringLiteral("bestOffer"), bestOffer},
		{QStringLiteral("hasOffer"), hasOffer},
		{QStringLiteral("datesApplied"), true}
	});
}
