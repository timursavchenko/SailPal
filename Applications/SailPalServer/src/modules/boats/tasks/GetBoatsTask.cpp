#include "GetBoatsTask.h"

#include "../api/BookingManagerClient.h"

namespace {

	static QString bmLanguageForSailPalLanguage(const QString& lang) {
		const auto normalizedLang = lang.trimmed().toLower();
		if(normalizedLang == "deu") return QStringLiteral("de");
		if(normalizedLang == "fra") return QStringLiteral("fr");
		if(normalizedLang == "spa") return QStringLiteral("es");
		if(normalizedLang == "por") return QStringLiteral("pt");
		if(normalizedLang == "ita") return QStringLiteral("it");
		if(normalizedLang == "tur") return QStringLiteral("tr");
		if(normalizedLang == "hun") return QStringLiteral("hu");
		if(normalizedLang == "rus") return QStringLiteral("ru");
		if(normalizedLang == "ell") return QStringLiteral("el");
		if(normalizedLang == "zho") return QStringLiteral("zh");
		if(normalizedLang == "kor") return QStringLiteral("ko");
		if(normalizedLang == "aze") return QStringLiteral("tr");
		return QStringLiteral("en");
	}

	static QString jsonString(const QJsonObject& object, const QString& key) {
		const auto value = object.value(key);
		if(value.isString()) {
			return value.toString();
		}
		if(value.isDouble()) {
			return QString::number(value.toDouble());
		}
		return {};
	}

	static int jsonInt(const QJsonObject& object, const QString& key) {
		return object.value(key).toInt();
	}

	static double jsonDouble(const QJsonObject& object, const QString& key) {
		return object.value(key).toDouble();
	}

	static QString firstImageUrl(const QJsonObject& yacht) {
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

	static bool yachtMatchesSearch(const QJsonObject& yacht, const QString& search) {
		const auto normalizedSearch = search.trimmed();
		if(normalizedSearch.isEmpty()) {
			return true;
		}

		const QStringList searchableFields({
			jsonString(yacht, QStringLiteral("name")),
			jsonString(yacht, QStringLiteral("model")),
			jsonString(yacht, QStringLiteral("kind")),
			jsonString(yacht, QStringLiteral("homeBase")),
			jsonString(yacht, QStringLiteral("company"))
		});

		for(const auto& field : searchableFields) {
			if(field.contains(normalizedSearch, Qt::CaseInsensitive)) {
				return true;
			}
		}
		return false;
	}

	static QJsonObject yachtSummaryToJson(const QJsonObject& yacht) {
		return QJsonObject({
			{"id", jsonString(yacht, QStringLiteral("id"))},
			{"name", jsonString(yacht, QStringLiteral("name"))},
			{"model", jsonString(yacht, QStringLiteral("model"))},
			{"kind", jsonString(yacht, QStringLiteral("kind"))},
			{"year", jsonInt(yacht, QStringLiteral("year"))},
			{"company", jsonString(yacht, QStringLiteral("company"))},
			{"homeBase", jsonString(yacht, QStringLiteral("homeBase"))},
			{"cabins", jsonInt(yacht, QStringLiteral("cabins"))},
			{"berths", jsonInt(yacht, QStringLiteral("berths"))},
			{"heads", jsonInt(yacht, QStringLiteral("wc"))},
			{"length", jsonDouble(yacht, QStringLiteral("length"))},
			{"currency", jsonString(yacht, QStringLiteral("currency"))},
			{"imageUrl", firstImageUrl(yacht)}
		});
	}

	static QJsonObject offerSummaryToJson(const QJsonObject& offer) {
		return QJsonObject({
			{"yachtId", offer.value(QStringLiteral("yachtId")).toVariant().toString()},
			{"yacht", offer.value(QStringLiteral("yacht")).toString()},
			{"startBase", offer.value(QStringLiteral("startBase")).toString()},
			{"endBase", offer.value(QStringLiteral("endBase")).toString()},
			{"dateFrom", offer.value(QStringLiteral("dateFrom")).toString()},
			{"dateTo", offer.value(QStringLiteral("dateTo")).toString()},
			{"product", offer.value(QStringLiteral("product")).toString()},
			{"price", offer.value(QStringLiteral("price")).toDouble()},
			{"currency", offer.value(QStringLiteral("currency")).toString()},
			{"startPrice", offer.value(QStringLiteral("startPrice")).toDouble()},
			{"discountPercentage", offer.value(QStringLiteral("discountPercentage")).toDouble()}
		});
	}

}

GetBoatsTask::GetBoatsTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module))
	,m_search(request.value(QStringLiteral("search")).toString())
	,m_kind(request.value(QStringLiteral("kind")).toString())
	,m_companyId(request.value(QStringLiteral("companyId")).toString(m_module.storage().bookingManagerSettings().companyId))
	,m_currency(request.value(QStringLiteral("currency")).toString(m_module.storage().bookingManagerSettings().currency))
	,m_dateFrom(request.value(QStringLiteral("dateFrom")).toString())
	,m_dateTo(request.value(QStringLiteral("dateTo")).toString()) {
}

QJsonObject GetBoatsTask::process() {
	const auto settings = m_module.storage().bookingManagerSettings();
	BookingManagerClient client(
		settings.baseUrl,
		settings.bearerToken
	);

	BookingManagerClient::YachtFilters filters;
	filters.language = bmLanguageForSailPalLanguage(m_lang);
	filters.currency = m_currency;
	filters.companyId = m_companyId;
	filters.kind = m_kind;

	const auto response = client.getYachts(filters);
	if(!response.ok) {
		throw response.errorMessage;
	}

	QHash<QString, QJsonObject> offersByYachtId;
	const bool datesApplied = !m_dateFrom.trimmed().isEmpty() && !m_dateTo.trimmed().isEmpty();
	if(datesApplied) {
		BookingManagerClient::OfferFilters offerFilters;
		offerFilters.dateFrom = m_dateFrom;
		offerFilters.dateTo = m_dateTo;
		offerFilters.currency = m_currency;
		offerFilters.companyId = m_companyId;
		offerFilters.kind = m_kind;

		const auto offersResponse = client.getOffers(offerFilters);
		if(!offersResponse.ok) {
			throw offersResponse.errorMessage;
		}

		for(const auto& offerValue : offersResponse.payload.toArray()) {
			const auto offer = offerValue.toObject();
			const auto yachtId = offer.value(QStringLiteral("yachtId")).toVariant().toString().trimmed();
			if(yachtId.isEmpty() || offersByYachtId.contains(yachtId)) {
				continue;
			}
			offersByYachtId.insert(yachtId, offerSummaryToJson(offer));
		}
	}

	const auto yachts = response.payload.toArray();
	QJsonArray items;
	int offersMatchedCount = 0;
	for(const auto& yachtValue : yachts) {
		const auto yacht = yachtValue.toObject();
		if(!yachtMatchesSearch(yacht, m_search)) {
			continue;
		}
		auto yachtJson = yachtSummaryToJson(yacht);
		const auto yachtId = yachtJson.value(QStringLiteral("id")).toString().trimmed();
		if(offersByYachtId.contains(yachtId)) {
			yachtJson.insert(QStringLiteral("offer"), offersByYachtId.value(yachtId));
			++offersMatchedCount;
		}
		items.append(yachtJson);
	}

	return QJsonObject({
		{"items", items},
		{"count", items.size()},
		{"currency", filters.currency},
		{"language", filters.language},
		{"dateFrom", m_dateFrom},
		{"dateTo", m_dateTo},
		{"datesApplied", datesApplied},
		{"offersMatchedCount", offersMatchedCount}
	});
}