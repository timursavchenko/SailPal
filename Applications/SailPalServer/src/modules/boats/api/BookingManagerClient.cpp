#include "BookingManagerClient.h"

namespace {

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

	if(networkError != QNetworkReply::NoError) {
		return Response{
			false,
			statusCode,
			networkError == QNetworkReply::OperationCanceledError
				? QStringLiteral("Booking Manager API request timed out.")
				: QStringLiteral("Booking Manager API request failed: ") + networkErrorString,
			{}
		};
	}

	QJsonParseError parseError;
	const auto document = QJsonDocument::fromJson(responseBody, &parseError);
	if(parseError.error != QJsonParseError::NoError) {
		return Response{
			false,
			statusCode,
			QStringLiteral("Booking Manager API returned invalid JSON."),
			{}
		};
	}

	if(statusCode < 200 || statusCode >= 300) {
		return Response{
			false,
			statusCode,
			QStringLiteral("Booking Manager API returned HTTP %1.").arg(statusCode),
			document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object())
		};
	}

	return Response{
		true,
		statusCode,
		{},
		document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object())
	};
}