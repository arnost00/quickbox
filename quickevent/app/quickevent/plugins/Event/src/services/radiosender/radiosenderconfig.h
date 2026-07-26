#pragma once

#include <QString>
#include <QVariantMap>

namespace Event::services {

struct RadioSenderConfig
{
	static RadioSenderConfig fromVariantMap(const QVariantMap &map);
	QVariantMap toVariantMap() const;

	QString listenAddress = QStringLiteral("0.0.0.0");
	int port = 1122;
	int startControl = 0;
	int finishControl = 255;
	int startToleranceMs = 3000;
	int finishToleranceMs = 2000;
};

} // namespace Event::services
