#pragma once

#include <QDateTime>
#include <QString>

namespace Event {

struct OFeedConfig
{
	QString hostUrl;             // Default "https://api.orienteerfeed.com" applied by AppDbConfig::ofeedConfig()
	QString eventId;
	QString eventPassword;
	QString changelogOrigin;     // Default "START" applied by AppDbConfig::ofeedConfig()
	QDateTime lastChangelogCall; // Default QDateTime::fromSecsSinceEpoch(0) applied by AppDbConfig::ofeedConfig()
	bool runXmlValidation = true;
	bool runChangesProcessing = false;
	// Non-stage-specific field, stored as ofeed.introTourShowed
	bool introTourShowed = false;
};

}
