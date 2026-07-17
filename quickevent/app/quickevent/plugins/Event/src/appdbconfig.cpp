#include "appdbconfig.h"
#include "eventplugin.h"

#include <qf/core/assert.h>
#include <qf/core/utils.h>
#include <qf/core/sql/connection.h>
#include <qf/core/sql/query.h>
#include <qf/core/sql/querybuilder.h>
#include <qf/core/log.h>

#include <qf/gui/framework/mainwindow.h>
#include <qf/gui/dialogs/messagebox.h>

#include <QDateTime>

using namespace Event;

namespace {
const auto EVENT_NAME = QStringLiteral("event.name");
}

QVariant AppDbConfig::value(const QStringList &path, const QVariant &default_value) const
{
	//QF_ASSERT(knownKeys().contains(key), "Key " + key + " is not known key!", return QVariant());
	QVariant ret = default_value;
	if(!path.isEmpty()) {
		QVariantMap m = m_data;
		for (int i = 0; i < path.count() - 1; ++i) {
			const QString &key = path[i];
			m = m.value(key).toMap();
		}
		ret = m.value(path.last(), default_value);
	}
	return ret;
}

void AppDbConfig::setValue(const QStringList &path, const QVariant &val)
{
	QF_ASSERT(!path.isEmpty(), "Empty path!", return);
	m_data = setValue_helper(m_data, path, val);
}

void AppDbConfig::load()
{
	using namespace qf::core::sql;
	Connection conn = Connection::forName();

	// Check connection existence / validity
	Q_ASSERT(conn.isOpen());
	// if(!conn.isOpen()) {
	// 	qfWarning() << "EventConfig::load(): database connection is not open:"
	// 	            << conn.errorString();
	// 	return;
	// }

	Query q(conn);
	QueryBuilder qb;
	qb.select("ckey, cvalue, ctype").from("config").orderBy("ckey");
	if(q.exec(qb.toString(), qf::core::Exception::Throw)) {
		while(q.next()) {
			QString key = q.value(0).toString();
			/*
			if(!knownKeys().contains(key)) {
				qfWarning() << "Config key" << key << "is not known to the QuickEvent config system";
			}
			*/
			QVariant val = q.value(1);
			QString type = q.value(2).toString();
			QVariant v = qf::core::Utils::retypeStringValue(val.toString(), type);
			setValue(key, v);
		}
	}
	// checkApiKey();
}

void AppDbConfig::save(const QString &path_to_save)
{
	QVariantMap m;
	save_helper(m, QString(), m_data);
	using namespace qf::core::sql;
	Connection conn = Connection::forName();

	try {
		Query q_up(conn);
		q_up.prepare("UPDATE config SET cvalue=:val WHERE ckey=:key", qf::core::Exception::Throw);
		Query q_ins(conn);
		q_ins.prepare("INSERT INTO config (ckey, cvalue, ctype) VALUES (:key, :val, :type)", qf::core::Exception::Throw);
		QMapIterator<QString, QVariant> it(m);
		while(it.hasNext()) {
			it.next();
			QString key = it.key();
			if(!path_to_save.isEmpty()) {
				if(!key.startsWith(path_to_save))
					continue;
				if(key.length() > path_to_save.length() && key[path_to_save.length()] != '.')
					continue;
			}
			QVariant val = it.value();
			QString val_str;
			if(val.typeId() == qMetaTypeId<QDate>())
				val_str = val.toDate().toString(Qt::ISODate);
			else if(val.typeId() == qMetaTypeId<QTime>())
				val_str = val.toTime().toString(Qt::ISODate);
			else if(val.typeId() == qMetaTypeId<QDateTime>())
				val_str = val.toDateTime().toString(Qt::ISODate);
			else
				val_str = val.toString();
			q_up.bindValue(":key", key);
			q_up.bindValue(":val", val_str);
			q_up.exec(qf::core::Exception::Throw);
			if(q_up.numRowsAffected() < 1) {
				QString type = val.typeName();
				q_ins.bindValue(":key", key);
				q_ins.bindValue(":type", type);
				q_ins.bindValue(":val", val_str);
				q_ins.exec(qf::core::Exception::Throw);
			}
		}
	}
	catch(const qf::core::Exception &e) {
		qf::gui::framework::MainWindow *fwk = qf::gui::framework::MainWindow::frameWork();
		qf::gui::dialogs::MessageBox::showException(fwk, e);
	}

}

void AppDbConfig::save_helper(QVariantMap &ret, const QString &current_path, const QVariant &val)
{
	if(val.typeId() == qMetaTypeId<QVariantMap>()) {
		QVariantMap m = val.toMap();
		QMapIterator<QString, QVariant> it(m);
		while(it.hasNext()) {
			it.next();
			QString cp = it.key();
			if(!current_path.isEmpty())
				cp = current_path + '.' + cp;
			save_helper(ret, cp, it.value());
		}
	}
	else {
		ret[current_path] = val;
	}
}

QVariantMap AppDbConfig::setValue_helper(const QVariantMap &m, const QStringList &path, const QVariant &val)
{
	QVariantMap ret;
	QF_ASSERT(!path.isEmpty(), "Empty path!", return ret);
	if(path.count() == 1) {
		ret = m;
		ret[path.first()] = val;
	}
	else {
		QStringList p = path;
		QString key = p.takeFirst();
		ret = m;
		ret[key] = setValue_helper(m.value(key).toMap(), p, val);
	}
	return ret;
}



// namespace {
// auto const API_KEY_CONFIG_PATH = QStringLiteral("event.apiKey");
// }
// QString EventConfig::apiKey() const
// {
// 	return value(API_KEY_CONFIG_PATH).toString();
// }

// void AppDbConfig::checkApiKey()
// {
// 	if (apiKey().isEmpty()) {
// 		auto api_key = EventPlugin::createApiKey(10);
// 		setValue(API_KEY_CONFIG_PATH, api_key);
// 		save(API_KEY_CONFIG_PATH);
// 	}
// }



int AppDbConfig::dbVersion() const
{
	return value(QStringLiteral("db.version")).toInt();
}

std::optional<int> AppDbConfig::maximumCardCheckAdvanceSec() const
{
	if(auto sec = eventConfig().cardCheckTimeSec; sec > 0)
		return sec;
	return {};
}

EventConfig AppDbConfig::eventConfig() const
{
	return EventConfig::fromVariantMap(value(QStringLiteral("event")).toMap());
}

void AppDbConfig::setEventConfig(const EventConfig &config)
{
	setValue(QStringLiteral("event"), config.toVariantMap());
}

OResultsConfig AppDbConfig::oresultsConfig(int stage_id) const
{
	const QString e = QStringLiteral(".E") + QString::number(stage_id);
	OResultsConfig cfg;
	cfg.apiKey = value(QStringLiteral("oresults.apiKey") + e).toString();
	cfg.eventName = value(QStringLiteral("oresults.eventName") + e).toString();
	return cfg;
}

void AppDbConfig::setOresultsConfig(int stage_id, const OResultsConfig &cfg)
{
	const QString e = QStringLiteral(".E") + QString::number(stage_id);
	setValue(QStringLiteral("oresults.apiKey") + e, cfg.apiKey);
	setValue(QStringLiteral("oresults.eventName") + e, cfg.eventName);
}

ReceiptsConfig AppDbConfig::receiptsConfig(int stage_id) const
{
	const QString e = QStringLiteral(".E") + QString::number(stage_id);
	ReceiptsConfig cfg;
	cfg.printQrCode = value(QStringLiteral("event.receiptPrintEventQrCode") + e, false).toBool();
	cfg.linkUrl = value(QStringLiteral("event.receiptEventLinkUrl") + e).toString().trimmed();
	const QString caption = value(QStringLiteral("event.receiptPrintEventQrCodeCaption") + e).toString().trimmed();
	cfg.qrCodeCaption = caption.isEmpty() ? QStringLiteral("Live Results") : caption;
	cfg.printImage = value(QStringLiteral("event.receiptPrintEventImage") + e, false).toBool();
	const int h = value(QStringLiteral("event.receiptImageHeightMm") + e, 18).toInt();
	cfg.imageHeightMm = (h < 10) ? 10 : (h > 60) ? 60 : h;
	cfg.imageBase64 = value(QStringLiteral("event.receiptImageDataBase64") + e).toString();
	const QString fmt = value(QStringLiteral("event.receiptImageFormat") + e).toString().trimmed().toLower();
	cfg.imageFormat = fmt.isEmpty() ? QStringLiteral("png") : fmt;
	return cfg;
}

void AppDbConfig::setReceiptsConfig(int stage_id, const ReceiptsConfig &cfg)
{
	const QString e = QStringLiteral(".E") + QString::number(stage_id);
	setValue(QStringLiteral("event.receiptPrintEventQrCode") + e, cfg.printQrCode);
	setValue(QStringLiteral("event.receiptEventLinkUrl") + e, cfg.linkUrl);
	setValue(QStringLiteral("event.receiptPrintEventQrCodeCaption") + e, cfg.qrCodeCaption);
	setValue(QStringLiteral("event.receiptPrintEventImage") + e, cfg.printImage);
	setValue(QStringLiteral("event.receiptImageHeightMm") + e, cfg.imageHeightMm);
	setValue(QStringLiteral("event.receiptImageDataBase64") + e, cfg.imageBase64);
	setValue(QStringLiteral("event.receiptImageFormat") + e, cfg.imageFormat);
}

OFeedConfig AppDbConfig::ofeedConfig(int stage_id) const
{
	const QString prefix = QStringLiteral("ofeed.");
	const QString e = QStringLiteral(".E") + QString::number(stage_id);
	OFeedConfig cfg;
	const QString default_host = QStringLiteral("https://api.orienteerfeed.com");
	cfg.hostUrl = value(prefix + QStringLiteral("hostUrl") + e, default_host).toString();
	cfg.eventId = value(prefix + QStringLiteral("eventId") + e).toString();
	cfg.eventPassword = value(prefix + QStringLiteral("eventPassword") + e).toString();
	const QString origin = value(prefix + QStringLiteral("changelogOrigin") + e).toString();
	cfg.changelogOrigin = origin.isEmpty() ? QStringLiteral("START") : origin;
	const QString lcc = value(prefix + QStringLiteral("lastChangelogCall") + e).toString();
	cfg.lastChangelogCall = lcc.isEmpty() ? QDateTime::fromSecsSinceEpoch(0)
	                                        : QDateTime::fromString(lcc, Qt::ISODate);
	if (!cfg.lastChangelogCall.isValid())
		cfg.lastChangelogCall = QDateTime::fromSecsSinceEpoch(0);
	cfg.runXmlValidation = value(prefix + QStringLiteral("runXmlValidation") + e, true).toBool();
	cfg.runChangesProcessing = value(prefix + QStringLiteral("runChangesProcessing") + e, false).toBool();
	cfg.introTourShowed = value(prefix + QStringLiteral("introTourShowed"), false).toBool();
	return cfg;
}

void AppDbConfig::setOfeedConfig(int stage_id, const OFeedConfig &cfg)
{
	const QString prefix = QStringLiteral("ofeed.");
	const QString e = QStringLiteral(".E") + QString::number(stage_id);
	setValue(prefix + QStringLiteral("hostUrl") + e, cfg.hostUrl);
	setValue(prefix + QStringLiteral("eventId") + e, cfg.eventId);
	setValue(prefix + QStringLiteral("eventPassword") + e, cfg.eventPassword);
	setValue(prefix + QStringLiteral("changelogOrigin") + e, cfg.changelogOrigin);
	setValue(prefix + QStringLiteral("lastChangelogCall") + e, cfg.lastChangelogCall.toString(Qt::ISODate));
	setValue(prefix + QStringLiteral("runXmlValidation") + e, cfg.runXmlValidation);
	setValue(prefix + QStringLiteral("runChangesProcessing") + e, cfg.runChangesProcessing);
	setValue(prefix + QStringLiteral("introTourShowed"), cfg.introTourShowed);
}
