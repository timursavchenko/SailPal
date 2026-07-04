#pragma once

#include <nexus.h>

class BookingManagerClient {

	public:
		struct YachtFilters {
			QString language;
			QString currency;
			QString companyId;
			QString kind;
		};

		struct Response {
			bool ok = false;
			int statusCode = 0;
			QString errorMessage;
			QJsonValue payload;
		};

	public:
		BookingManagerClient(const QString& baseUrl, const QString& bearerToken);

	public:
		Response getYachts(const YachtFilters& filters) const;

	public:
		static bool requiresBearerToken(const QString& baseUrl);

	private:
		Response get(const QString& path, const QUrlQuery& query) const;

	private:
		QString m_baseUrl;
		QString m_bearerToken;
};