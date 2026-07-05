#include "GetBoatsTask.h"

#include "../api/BookingManagerClient.h"

#include <algorithm>

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

	static int requestInt(const QJsonObject& request, const QString& key) {
		const auto value = request.value(key);
		if(value.isDouble()) {
			return value.toInt();
		}
		if(value.isString()) {
			bool ok = false;
			const auto parsed = value.toString().trimmed().toInt(&ok);
			return ok ? parsed : 0;
		}
		return 0;
	}

	static double requestDouble(const QJsonObject& request, const QString& key) {
		const auto value = request.value(key);
		if(value.isDouble()) {
			return value.toDouble();
		}
		if(value.isString()) {
			bool ok = false;
			const auto parsed = value.toString().trimmed().toDouble(&ok);
			return ok ? parsed : 0.0;
		}
		return 0.0;
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

	static bool textFilterMatches(const QString& actualValue, const QString& expectedValue) {
		const auto normalizedExpected = expectedValue.trimmed();
		if(normalizedExpected.isEmpty()) {
			return true;
		}
		return actualValue.trimmed().compare(normalizedExpected, Qt::CaseInsensitive) == 0;
	}

	static QJsonObject yachtSummaryToJson(const QJsonObject& yacht) {
		return QJsonObject({
			{"id", jsonString(yacht, QStringLiteral("id"))},
			{"name", jsonString(yacht, QStringLiteral("name"))},
			{"model", jsonString(yacht, QStringLiteral("model"))},
			{"kind", jsonString(yacht, QStringLiteral("kind"))},
			{"year", jsonInt(yacht, QStringLiteral("year"))},
			{"company", jsonString(yacht, QStringLiteral("company"))},
			{"companyId", jsonString(yacht, QStringLiteral("companyId"))},
			{"homeBase", jsonString(yacht, QStringLiteral("homeBase"))},
			{"homeBaseId", jsonString(yacht, QStringLiteral("homeBaseId"))},
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

	static QJsonArray stringListToJson(const QStringList& values) {
		QJsonArray result;
		for(const auto& value : values) {
			result.append(value);
		}
		return result;
	}

	static QStringList uniqueSortedFieldValues(const QJsonArray& yachts, const QString& key) {
		QStringList values;
		for(const auto& yachtValue : yachts) {
			const auto value = yachtValue.toObject().value(key).toString().trimmed();
			if(value.isEmpty() || values.contains(value, Qt::CaseInsensitive)) {
				continue;
			}
			values.append(value);
		}
		std::sort(values.begin(), values.end(), [](const QString& left, const QString& right) {
			return QString::localeAwareCompare(left, right) < 0;
		});
		return values;
	}

	static bool yachtMatchesFilters(const QJsonObject& yacht,
		const QString& search,
		const QString& companyName,
		const QString& homeBase,
		const int minCabins,
		const int minBerths,
		const double minLength) {

		if(!yachtMatchesSearch(yacht, search)) {
			return false;
		}
		if(!textFilterMatches(jsonString(yacht, QStringLiteral("company")), companyName)) {
			return false;
		}
		if(!textFilterMatches(jsonString(yacht, QStringLiteral("homeBase")), homeBase)) {
			return false;
		}
		if(minCabins > 0 && jsonInt(yacht, QStringLiteral("cabins")) < minCabins) {
			return false;
		}
		if(minBerths > 0 && jsonInt(yacht, QStringLiteral("berths")) < minBerths) {
			return false;
		}
		if(minLength > 0.0 && jsonDouble(yacht, QStringLiteral("length")) < minLength) {
			return false;
		}
		return true;
	}

	static bool boatHasOffer(const QJsonObject& boat) {
		return boat.value(QStringLiteral("offer")).isObject();
	}

	static double boatOfferPrice(const QJsonObject& boat) {
		return boat.value(QStringLiteral("offer")).toObject().value(QStringLiteral("price")).toDouble();
	}

	static bool compareBoats(const QJsonObject& left, const QJsonObject& right, const QString& sortBy, const QString& sortDirection) {
		const auto normalizedSortBy = sortBy.trimmed().toLower();
		const bool ascending = sortDirection.trimmed().compare(QStringLiteral("desc"), Qt::CaseInsensitive) != 0;

		auto compareNumbers = [&](const double leftValue, const double rightValue) {
			if(qFuzzyCompare(leftValue + 1.0, rightValue + 1.0)) {
				return QString::localeAwareCompare(left.value(QStringLiteral("name")).toString(), right.value(QStringLiteral("name")).toString()) < 0;
			}
			return ascending ? leftValue < rightValue : leftValue > rightValue;
		};

		if(normalizedSortBy == QStringLiteral("price")) {
			const auto leftHasOffer = boatHasOffer(left);
			const auto rightHasOffer = boatHasOffer(right);
			if(leftHasOffer != rightHasOffer) {
				return leftHasOffer;
			}
			return compareNumbers(boatOfferPrice(left), boatOfferPrice(right));
		}

		if(normalizedSortBy == QStringLiteral("availability")) {
			const auto leftHasOffer = boatHasOffer(left);
			const auto rightHasOffer = boatHasOffer(right);
			if(leftHasOffer != rightHasOffer) {
				return leftHasOffer;
			}
			return compareNumbers(boatOfferPrice(left), boatOfferPrice(right));
		}

		if(normalizedSortBy == QStringLiteral("year")) {
			return compareNumbers(left.value(QStringLiteral("year")).toDouble(), right.value(QStringLiteral("year")).toDouble());
		}
		if(normalizedSortBy == QStringLiteral("length")) {
			return compareNumbers(left.value(QStringLiteral("length")).toDouble(), right.value(QStringLiteral("length")).toDouble());
		}
		if(normalizedSortBy == QStringLiteral("cabins")) {
			return compareNumbers(left.value(QStringLiteral("cabins")).toDouble(), right.value(QStringLiteral("cabins")).toDouble());
		}
		if(normalizedSortBy == QStringLiteral("berths")) {
			return compareNumbers(left.value(QStringLiteral("berths")).toDouble(), right.value(QStringLiteral("berths")).toDouble());
		}

		const auto leftName = left.value(QStringLiteral("name")).toString();
		const auto rightName = right.value(QStringLiteral("name")).toString();
		return ascending
			? QString::localeAwareCompare(leftName, rightName) < 0
			: QString::localeAwareCompare(leftName, rightName) > 0;
	}

}

GetBoatsTask::GetBoatsTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module))
	,m_search(request.value(QStringLiteral("search")).toString())
	,m_kind(request.value(QStringLiteral("kind")).toString())
	,m_companyName(request.value(QStringLiteral("companyName")).toString())
	,m_homeBase(request.value(QStringLiteral("homeBase")).toString())
	,m_companyId(request.value(QStringLiteral("companyId")).toString(m_module.storage().bookingManagerSettings().companyId))
	,m_currency(request.value(QStringLiteral("currency")).toString(m_module.storage().bookingManagerSettings().currency))
	,m_dateFrom(request.value(QStringLiteral("dateFrom")).toString())
	,m_dateTo(request.value(QStringLiteral("dateTo")).toString())
	,m_sortBy(request.value(QStringLiteral("sortBy")).toString(QStringLiteral("name")))
	,m_sortDirection(request.value(QStringLiteral("sortDirection")).toString(QStringLiteral("asc")))
	,m_minCabins(requestInt(request, QStringLiteral("minCabins")))
	,m_minBerths(requestInt(request, QStringLiteral("minBerths")))
	,m_minLength(requestDouble(request, QStringLiteral("minLength"))) {
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
	QVector<QJsonObject> items;
	int offersMatchedCount = 0;
	for(const auto& yachtValue : yachts) {
		const auto yacht = yachtValue.toObject();
		if(!yachtMatchesFilters(yacht, m_search, m_companyName, m_homeBase, m_minCabins, m_minBerths, m_minLength)) {
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

	std::sort(items.begin(), items.end(), [&](const QJsonObject& left, const QJsonObject& right) {
		return compareBoats(left, right, m_sortBy, m_sortDirection);
	});

	QJsonArray itemsJson;
	for(const auto& item : items) {
		itemsJson.append(item);
	}

	return QJsonObject({
		{"items", itemsJson},
		{"count", items.size()},
		{"currency", filters.currency},
		{"language", filters.language},
		{"dateFrom", m_dateFrom},
		{"dateTo", m_dateTo},
		{"datesApplied", datesApplied},
		{"offersMatchedCount", offersMatchedCount},
		{"availableCompanies", stringListToJson(uniqueSortedFieldValues(yachts, QStringLiteral("company")))},
		{"availableBases", stringListToJson(uniqueSortedFieldValues(yachts, QStringLiteral("homeBase")))}
	});
}