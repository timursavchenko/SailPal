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

}

GetBoatsTask::GetBoatsTask(const QJsonObject& request, Module& module)
	:BoatsTask(request, dynamic_cast<BoatsModule&>(module))
	,m_search(request.value(QStringLiteral("search")).toString())
	,m_kind(request.value(QStringLiteral("kind")).toString())
	,m_companyId(request.value(QStringLiteral("companyId")).toString(m_module.storage().bookingManagerSettings().companyId))
	,m_currency(request.value(QStringLiteral("currency")).toString(m_module.storage().bookingManagerSettings().currency)) {
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

	const auto yachts = response.payload.toArray();
	QJsonArray items;
	for(const auto& yachtValue : yachts) {
		const auto yacht = yachtValue.toObject();
		if(!yachtMatchesSearch(yacht, m_search)) {
			continue;
		}
		items.append(yachtSummaryToJson(yacht));
	}

	return QJsonObject({
		{"items", items},
		{"count", items.size()},
		{"currency", filters.currency},
		{"language", filters.language}
	});
}