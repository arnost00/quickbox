#pragma once

#include <QDateTime>
#include <QString>
#include <QVariantMap>

namespace Event {


struct StageData
{
	QDateTime startDateTime;
	bool useAllMaps = false;
	QVariantMap drawingConfig;
	QString qxApiToken;
};

}

