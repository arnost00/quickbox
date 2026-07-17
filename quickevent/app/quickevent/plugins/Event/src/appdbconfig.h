#pragma once

#include "eventconfig.h"
#include "receiptsconfig.h"
#include "ofeedconfig.h"

#include <QObject>
#include <QVariantMap>
#include <QSet>
#include <QDateTime>

#include <optional>
#include <qhashfunctions.h>

namespace Event {

struct QxConfig
{
	QString apiToken;
};

struct OResultsConfig
{
	QMap<int, QString> apiKeys;
};

class AppDbConfig
{
public:
	explicit AppDbConfig() = default;
public:
	QVariantMap values() const {return m_data;}
	QVariant value(const QStringList &path, const QVariant &default_value = QVariant()) const;
	QVariant value(const QString &path, const QVariant &default_value = QVariant()) const {
		return value(path.split('.'), default_value);
	}
	void setValue(const QStringList &path, const QVariant &val);
	void setValue(const QString &path, const QVariant &val) {setValue(path.split('.'), val);}
	void load();
	void save(const QString &path_to_save = QString());

	bool isHandicap() const {return eventConfig().handicapLength > 0;}


	int dbVersion() const;
	std::optional<int> maximumCardCheckAdvanceSec() const;
	EventConfig eventConfig() const;
	ReceiptsConfig receiptsConfig(int stage_id) const;
	void setReceiptsConfig(int stage_id, const ReceiptsConfig &config);
	OFeedConfig ofeedConfig(int stage_id) const;
	void setOfeedConfig(int stage_id, const OFeedConfig &config);
private:
	void save_helper(QVariantMap &ret, const QString &current_path, const QVariant &val);
	QVariantMap setValue_helper(const QVariantMap &m, const QStringList &path, const QVariant &val);
private:
	QVariantMap m_data;
};

}
