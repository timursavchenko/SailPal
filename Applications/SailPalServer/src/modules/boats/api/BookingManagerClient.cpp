#include "BookingManagerClient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

namespace {

	constexpr qint64 kYachtsCacheTtlMs = 5 * 60 * 1000;
	constexpr qint64 kYachtCacheTtlMs = 5 * 60 * 1000;
	constexpr qint64 kOffersCacheTtlMs = 30 * 1000;
	constexpr int kMaxCacheEntries = 128;

	struct CacheEntry {
		qint64 expiresAtMs = 0;
		BookingManagerClient::Response response;
	};

	static QString normalizeBaseUrl(QString baseUrl) {
		baseUrl = baseUrl.trimmed();
		while(baseUrl.endsWith('/')) {
			baseUrl.chop(1);
		}
		return baseUrl.isEmpty() ? QStringLiteral("https://virtserver.swaggerhub.com/mmksystems/bm-api/2.0.0") : baseUrl;
	}

	static void appendQueryItemIfNotEmpty(QUrlQuery& query, const QString& name, const QString& value) {
		const auto normalizedValue = value.trimmed();
		if(!normalizedValue.isEmpty()) {
			query.addQueryItem(name, normalizedValue);
		}
	}

	static QMutex& cacheMutex() {
		static QMutex mutex;
		return mutex;
	}

	static QHash<QString, CacheEntry>& responseCache() {
		static QHash<QString, CacheEntry> cache;
		return cache;
	}

	static qint64 cacheTtlMs(const QString& path) {
		if(path == QStringLiteral("/offers")) {
			return kOffersCacheTtlMs;
		}
		if(path == QStringLiteral("/yachts")) {
			return kYachtsCacheTtlMs;
		}
		if(path.startsWith(QStringLiteral("/yacht/"))) {
			return kYachtCacheTtlMs;
		}
		return 0;
	}

	static void pruneCacheLocked(const qint64 nowMs) {
		auto& cache = responseCache();
		for(auto it = cache.begin(); it != cache.end(); ) {
			if(it->expiresAtMs <= nowMs) {
				it = cache.erase(it);
				continue;
			}
			++it;
		}

		if(cache.size() > kMaxCacheEntries) {
			cache.clear();
		}
	}

	static bool tryGetCachedResponse(const QString& key, BookingManagerClient::Response& response) {
		QMutexLocker locker(&cacheMutex());
		const auto nowMs = QDateTime::currentMSecsSinceEpoch();
		pruneCacheLocked(nowMs);

		const auto it = responseCache().constFind(key);
		if(it == responseCache().constEnd()) {
			return false;
		}

		response = it->response;
		return true;
	}

	static void storeCachedResponse(const QString& key, const QString& path, const BookingManagerClient::Response& response) {
		const auto ttlMs = cacheTtlMs(path);
		if(ttlMs <= 0 || !response.ok) {
			return;
		}

		QMutexLocker locker(&cacheMutex());
		const auto nowMs = QDateTime::currentMSecsSinceEpoch();
		pruneCacheLocked(nowMs);
		responseCache().insert(key, CacheEntry{nowMs + ttlMs, response});
	}

	static QString trimmedResponseText(const QByteArray& responseBody) {
		auto text = QString::fromUtf8(responseBody).simplified();
		if(text.size() > 240) {
			text = text.left(237) + QStringLiteral("...");
		}
		return text;
	}

	static QString errorDetailFromJsonValue(const QJsonValue& value) {
		if(value.isString()) {
			return value.toString().trimmed();
		}
		if(value.isObject()) {
			const auto object = value.toObject();
			for(const auto& key : {
				QStringLiteral("message"),
				QStringLiteral("error"),
				QStringLiteral("detail"),
				QStringLiteral("description"),
				QStringLiteral("title")
			}) {
				const auto detail = errorDetailFromJsonValue(object.value(key));
				if(!detail.isEmpty()) {
					return detail;
				}
			}
			return {};
		}
		if(value.isArray()) {
			const auto array = value.toArray();
			for(const auto& item : array) {
				const auto detail = errorDetailFromJsonValue(item);
				if(!detail.isEmpty()) {
					return detail;
				}
			}
		}
		return {};
	}

	static QString httpStatusSummary(const int statusCode) {
		switch(statusCode) {
			case 400: return QStringLiteral("Booking Manager API rejected the request.");
			case 401: return QStringLiteral("Booking Manager API rejected the bearer token.");
			case 403: return QStringLiteral("Booking Manager API denied access to this resource.");
			case 404: return QStringLiteral("Booking Manager API endpoint was not found. Check the module base URL.");
			case 429: return QStringLiteral("Booking Manager API rate limit was reached.");
			case 500: return QStringLiteral("Booking Manager API returned an internal server error.");
			case 502:
			case 503:
			case 504:
				return QStringLiteral("Booking Manager API is temporarily unavailable.");
			default:
				return QStringLiteral("Booking Manager API returned HTTP %1.").arg(statusCode);
		}
	}

	static QString networkErrorSummary(const QNetworkReply::NetworkError networkError, const QString& networkErrorString) {
		switch(networkError) {
			case QNetworkReply::ConnectionRefusedError:
				return QStringLiteral("Booking Manager API connection was refused.");
			case QNetworkReply::RemoteHostClosedError:
				return QStringLiteral("Booking Manager API closed the connection unexpectedly.");
			case QNetworkReply::HostNotFoundError:
				return QStringLiteral("Booking Manager API host was not found. Check the module base URL.");
			case QNetworkReply::TimeoutError:
			case QNetworkReply::OperationCanceledError:
				return QStringLiteral("Booking Manager API request timed out.");
			case QNetworkReply::SslHandshakeFailedError:
				return QStringLiteral("Booking Manager API TLS handshake failed.");
			default:
				return QStringLiteral("Booking Manager API request failed: %1").arg(networkErrorString);
		}
	}

	static QString withOptionalDetail(QString message, const QString& detail) {
		const auto normalizedDetail = detail.trimmed();
		if(normalizedDetail.isEmpty()) {
			return message;
		}
		return message + QStringLiteral(" Details: ") + normalizedDetail;
	}

}

BookingManagerClient::BookingManagerClient(const QString& baseUrl, const QString& bearerToken)
	:m_baseUrl(normalizeBaseUrl(baseUrl))
	,m_bearerToken(bearerToken.trimmed()) {
}

BookingManagerClient::Response BookingManagerClient::getYachts(const YachtFilters& filters) const {
	QUrlQuery query;
	appendQueryItemIfNotEmpty(query, QStringLiteral("language"), filters.language);
	appendQueryItemIfNotEmpty(query, QStringLiteral("currency"), filters.currency);
	appendQueryItemIfNotEmpty(query, QStringLiteral("companyId"), filters.companyId);
	appendQueryItemIfNotEmpty(query, QStringLiteral("kind"), filters.kind);

	return get(QStringLiteral("/yachts"), query);
}

BookingManagerClient::Response BookingManagerClient::getYacht(const QString& yachtId, const QString& language, const QString& currency) const {

	const auto normalizedYachtId = yachtId.trimmed();
	if(normalizedYachtId.isEmpty()) {
		return Response{false, 0, QStringLiteral("Boat id is empty."), {}};
	}

	QUrlQuery query;
	appendQueryItemIfNotEmpty(query, QStringLiteral("language"), language);
	appendQueryItemIfNotEmpty(query, QStringLiteral("currency"), currency);

	return get(QStringLiteral("/yacht/") + QUrl::toPercentEncoding(normalizedYachtId), query);
}

BookingManagerClient::Response BookingManagerClient::getOffers(const OfferFilters& filters) const {

	QUrlQuery query;
	appendQueryItemIfNotEmpty(query, QStringLiteral("dateFrom"), filters.dateFrom);
	appendQueryItemIfNotEmpty(query, QStringLiteral("dateTo"), filters.dateTo);
	appendQueryItemIfNotEmpty(query, QStringLiteral("currency"), filters.currency);
	appendQueryItemIfNotEmpty(query, QStringLiteral("companyId"), filters.companyId);
	appendQueryItemIfNotEmpty(query, QStringLiteral("yachtId"), filters.yachtId);
	appendQueryItemIfNotEmpty(query, QStringLiteral("kind"), filters.kind);

	return get(QStringLiteral("/offers"), query);
}

bool BookingManagerClient::requiresBearerToken(const QString& baseUrl) {

	const QUrl url(normalizeBaseUrl(baseUrl));
	return url.host().compare(QStringLiteral("virtserver.swaggerhub.com"), Qt::CaseInsensitive) != 0;
}

BookingManagerClient::Response BookingManagerClient::get(const QString& path, const QUrlQuery& query) const {
	if(m_bearerToken.isEmpty() && requiresBearerToken(m_baseUrl)) {
		return Response{
			false,
			0,
			QStringLiteral("Booking Manager API token is not configured."),
			{}
		};
	}

	Response cachedResponse;
	const auto requestCacheKey = cacheKey(path, query);
	if(tryGetCachedResponse(requestCacheKey, cachedResponse)) {
		return cachedResponse;
	}

	QUrl url(m_baseUrl + path);
	url.setQuery(query);

	QNetworkRequest request(url);
	request.setRawHeader("Accept", QByteArrayLiteral("application/json"));
	if(!m_bearerToken.isEmpty()) {
		request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_bearerToken.toUtf8());
	}

	QNetworkAccessManager manager;
	QEventLoop eventLoop;
	QTimer timeoutTimer;
	timeoutTimer.setSingleShot(true);

	QNetworkReply* reply = manager.get(request);
	QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
	QObject::connect(&timeoutTimer, &QTimer::timeout, &eventLoop, [&]() {
		reply->abort();
		eventLoop.quit();
	});

	timeoutTimer.start(30000);
	eventLoop.exec();
	timeoutTimer.stop();

	const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const auto responseBody = reply->readAll();
	const auto networkError = reply->error();
	const auto networkErrorString = reply->errorString();
	reply->deleteLater();

	QJsonParseError parseError;
	QJsonValue payload;
	const auto hasResponseBody = !responseBody.trimmed().isEmpty();
	if(hasResponseBody) {
		const auto document = QJsonDocument::fromJson(responseBody, &parseError);
		if(parseError.error == QJsonParseError::NoError) {
			payload = document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object());
		}
	}

	if(statusCode >= 400) {
		const auto detail = !payload.isUndefined() ? errorDetailFromJsonValue(payload) : trimmedResponseText(responseBody);
		return Response{false, statusCode, withOptionalDetail(httpStatusSummary(statusCode), detail), payload};
	}

	if(networkError != QNetworkReply::NoError) {
		return Response{
			false,
			statusCode,
			withOptionalDetail(networkErrorSummary(networkError, networkErrorString), trimmedResponseText(responseBody)),
			payload
		};
	}

	if(parseError.error != QJsonParseError::NoError) {
		return Response{
			false,
			statusCode,
			withOptionalDetail(QStringLiteral("Booking Manager API returned invalid JSON."), trimmedResponseText(responseBody)),
			{}
		};
	}

	const auto response = Response{
		true,
		statusCode,
		{},
		payload
	};
	storeCachedResponse(requestCacheKey, path, response);
	return response;
}

QString BookingManagerClient::cacheKey(const QString& path, const QUrlQuery& query) const {
	QUrl url(m_baseUrl + path);
	url.setQuery(query);
	const auto tokenHash = QString::fromLatin1(QCryptographicHash::hash(m_bearerToken.toUtf8(), QCryptographicHash::Sha1).toHex());
	return url.toString(QUrl::FullyEncoded) + QStringLiteral("#") + tokenHash;
}