#include "ofeedclient.h"
#include "ofeedclientwidget.h"

#include "../../eventplugin.h"

#include <qf/gui/framework/mainwindow.h>
#include <qf/gui/dialogs/dialog.h>
#include <qf/core/log.h>
#include <qf/core/utils/htmlutils.h>

#include <qf/core/sql/connection.h>
#include <qf/core/sql/query.h>
#include <qf/core/sql/transaction.h>

#include <plugins/Runs/src/runsplugin.h>
#include <plugins/Relays/src/relaysplugin.h>

#include <quickevent/core/si/checkedcard.h>
#include <quickevent/core/utils.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHttpPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QDateTime>
#include <QTimeZone>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <iostream>
#include <sstream>

using Event::EventPlugin;
using qf::gui::framework::getPlugin;
using Relays::RelaysPlugin;
using Runs::RunsPlugin;

namespace Event::services {

OFeedClient::OFeedClient(QObject *parent)
	: Super(OFeedClient::serviceName(), parent)
{
	m_networkManager = new QNetworkAccessManager(this);
	m_exportTimer = new QTimer(this);
	connect(m_exportTimer, &QTimer::timeout, this, &OFeedClient::onExportTimerTimeOut);
	connect(this, &OFeedClient::settingsChanged, this, &OFeedClient::init, Qt::QueuedConnection);
	connect(getPlugin<EventPlugin>(), &Event::EventPlugin::dbEventNotify, this, &OFeedClient::onDbEventNotify, Qt::QueuedConnection);
}

QString OFeedClient::serviceName()
{
	return QStringLiteral("OFeed");
}

void OFeedClient::run()
{
	Super::run();
	exportStartListIofXml3([this]()
						   { exportResultsIofXml3(); });
	m_exportTimer->start();
}

void OFeedClient::stop()
{
	Super::stop();
	m_exportTimer->stop();
}

void OFeedClient::exportResultsIofXml3()
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	bool is_relays = getPlugin<EventPlugin>()->eventConfig()->isRelays();

	QString str = is_relays
					  ? getPlugin<RelaysPlugin>()->resultsIofXml30()
					  : getPlugin<RunsPlugin>()->resultsIofXml30Stage(current_stage);

	sendFile("results upload", "/rest/v1/upload/iof", str);
}

void OFeedClient::exportStartListIofXml3(std::function<void()> on_success)
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	bool is_relays = getPlugin<EventPlugin>()->eventConfig()->isRelays();

	QString str = is_relays
					  ? getPlugin<RelaysPlugin>()->startListIofXml30()
					  : getPlugin<RunsPlugin>()->startListStageIofXml30(current_stage);

	sendFile("start list upload", "/rest/v1/upload/iof", str, on_success);
}

qf::gui::framework::DialogWidget *OFeedClient::createDetailWidget()
{
	auto *w = new OFeedClientWidget();
	return w;
}

void OFeedClient::init()
{
	OFeedClientSettings ss = settings();
	m_exportTimer->setInterval(ss.exportIntervalSec() * 1000);
}

void OFeedClient::onExportTimerTimeOut()
{
	// exportStartListIofXml3();
	getChangesFromStart();
	exportResultsIofXml3();
}

void OFeedClient::loadSettings()
{
	Super::loadSettings();
	init();
}

void OFeedClient::sendFile(QString name, QString request_path, QString file, std::function<void()> on_success)
{
	// Create a multi-part request (like FormData in JS)
	auto *multi_part = new QHttpMultiPart(QHttpMultiPart::FormDataType);

	// Prepare the Authorization header with Bearer token
	QString combined = eventId() + ":" + eventPassword();
	QByteArray base_64_auth = combined.toUtf8().toBase64();
	QString auth_value = "Basic " + QString(base_64_auth);
	QByteArray auth_header = auth_value.toUtf8();

	// Add eventId field
	QHttpPart event_id_part;
	event_id_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(R"(form-data; name="eventId")"));
	event_id_part.setBody(eventId().toUtf8());
	multi_part->append(event_id_part);

	// Add xml content with fake filename that must be present
	QHttpPart file_part;
	file_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(R"(form-data; name="file"; filename="uploaded_file.xml")"));
	file_part.setBody(file.toUtf8());
	multi_part->append(file_part);

	// Create network request with authorization header
	QUrl request_url(hostUrl() + request_path);
	QNetworkRequest request(request_url);
	request.setRawHeader("Authorization", auth_header);

	// Send request
	QNetworkReply *reply = m_networkManager->post(request, multi_part);
	multi_part->setParent(reply);

	// Cleanup
	connect(reply, &QNetworkReply::finished, this, [reply, name, request_url, on_success]() {
		if(reply->error()) {
			auto err_msg = "OFeed [" + name + "] " + request_url.toString() + " : ";
			auto response_body = reply->readAll();
			if (!response_body.isEmpty())
				err_msg += response_body + " | ";
			qfError() << err_msg + reply->errorString();
		}
		else {
			qfInfo() << "OFeed [" + name + "]: ok";
			if (on_success)
				on_success();
		}
		reply->deleteLater();
	});
}

void OFeedClient::onDbEventNotify(const QString &domain, int connection_id, const QVariant &data)
{
	if (status() != Status::Running)
		return;
	Q_UNUSED(connection_id)
	if (domain == QLatin1String(Event::EventPlugin::DBEVENT_CARD_PROCESSED_AND_ASSIGNED))
	{
		auto checked_card = quickevent::core::si::CheckedCard(data.toMap());
		int competitor_id = getPlugin<RunsPlugin>()->competitorForRun(checked_card.runId());
		std::cout << "DEBUG: Competitor READ-OUT, competitor id: " << competitor_id << ", runs.id: " << checked_card.runId() << std::endl;
		onCompetitorReadOut(competitor_id);
	}
	if (domain == QLatin1String(Event::EventPlugin::DBEVENT_COMPETITOR_EDITED))
	{
		int competitor_id = data.toInt();		
		std::cout << "DEBUG: Competitor EDITED, competitor id: " << competitor_id << std::endl;
		onCompetitorEdited(competitor_id);
	}
	if (domain == QLatin1String(Event::EventPlugin::DBEVENT_COMPETITOR_ADDED))
	{
		int competitor_id = data.toInt();
		std::cout << "DEBUG: Competitor ADDED, competitor id: " << competitor_id << std::endl;
		onCompetitorAdded(competitor_id);
	}
	// TODO: handle deleted competitor
}

QString OFeedClient::hostUrl() const
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	return getPlugin<EventPlugin>()->eventConfig()->value("ofeed.hostUrl.E" + QString::number(current_stage)).toString();
}

QString OFeedClient::eventId() const
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	return getPlugin<EventPlugin>()->eventConfig()->value("ofeed.eventId.E" + QString::number(current_stage)).toString();
}

QString OFeedClient::eventPassword() const
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	return getPlugin<EventPlugin>()->eventConfig()->value("ofeed.eventPassword.E" + QString::number(current_stage)).toString();
}

QDateTime OFeedClient::lastChangelogCall() {
    int current_stage = getPlugin<EventPlugin>()->currentStageId();
    QString key = "ofeed.lastChangelogCall.E" + QString::number(current_stage);

    // Retrieve the stored value from the configuration
    QVariant value = getPlugin<EventPlugin>()->eventConfig()->value(key);

    // Check if the value exists
    if (!value.isValid() || value.toString().isEmpty()) {
        // No valid value exists, set the initial value
        QDateTime initialValue = QDateTime::fromSecsSinceEpoch(0); // Default to Unix epoch (1970-01-01T00:00:00Z)
        getPlugin<EventPlugin>()->eventConfig()->setValue(key, initialValue.toString(Qt::ISODate));
        getPlugin<EventPlugin>()->eventConfig()->save("ofeed");
        qDebug() << "No lastChangelogCall found. Setting initial value to:" << initialValue.toString(Qt::ISODate);
        return initialValue;
    }

    // Convert the stored string to QDateTime
    QDateTime lastChangelog = QDateTime::fromString(value.toString(), Qt::ISODate);

    // Check if the conversion was successful
    if (!lastChangelog.isValid()) {
        // If invalid, set the default value
        QDateTime initialValue = QDateTime::fromSecsSinceEpoch(0); // Default to Unix epoch
        getPlugin<EventPlugin>()->eventConfig()->setValue(key, initialValue.toString(Qt::ISODate));
        getPlugin<EventPlugin>()->eventConfig()->save("ofeed");
        qDebug() << "Invalid lastChangelogCall found. Setting initial value to:" << initialValue.toString(Qt::ISODate);
        return initialValue;
    }

    return lastChangelog;
}

void OFeedClient::setHostUrl(QString hostUrl)
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	getPlugin<EventPlugin>()->eventConfig()->setValue("ofeed.hostUrl.E" + QString::number(current_stage), hostUrl);
	getPlugin<EventPlugin>()->eventConfig()->save("ofeed");
}

void OFeedClient::setEventId(QString eventId)
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	getPlugin<EventPlugin>()->eventConfig()->setValue("ofeed.eventId.E" + QString::number(current_stage), eventId);
	getPlugin<EventPlugin>()->eventConfig()->save("ofeed");
}

void OFeedClient::setEventPassword(QString eventPassword)
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	getPlugin<EventPlugin>()->eventConfig()->setValue("ofeed.eventPassword.E" + QString::number(current_stage), eventPassword);
	getPlugin<EventPlugin>()->eventConfig()->save("ofeed");
}

void OFeedClient::setLastChangelogCall(QDateTime lastChangelogCall)
{
	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	getPlugin<EventPlugin>()->eventConfig()->setValue("ofeed.lastChangelogCall.E" + QString::number(current_stage), lastChangelogCall);
	getPlugin<EventPlugin>()->eventConfig()->save("ofeed");
}

void OFeedClient::sendCompetitorChange(QString json_body, int runs_id)
{
	std::cout << "OFeed - request to change competitor with run id: " << runs_id << std::endl;
	// Prepare the Authorization header base64 username:password
	QString combined = eventId() + ":" + eventPassword();
	QByteArray base_64_auth = combined.toUtf8().toBase64();
	QString auth_value = "Basic " + QString(base_64_auth);
	QByteArray auth_header = auth_value.toUtf8();

	// Create the URL for the PUT request
	QUrl url(hostUrl() + "/rest/v1/events/" + eventId() + "/competitors/" + QString::number(runs_id) + "/external-id");

	// Create the network request
	QNetworkRequest request(url);
	request.setRawHeader("Authorization", auth_header);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// Send request
	QNetworkReply *reply = m_networkManager->put(request, json_body.toUtf8());

	connect(reply, &QNetworkReply::finished, this, [=]()
			{
if(reply->error()) {
	qfError() << "OFeed [competitor change]:" << reply->errorString();
}
else {
	QByteArray response = reply->readAll();
	QJsonDocument json_response = QJsonDocument::fromJson(response);
	QJsonObject json_object = json_response.object();

	if (json_object.contains("error") && !json_object["error"].toBool()) {
		QJsonObject results_object = json_object["results"].toObject();
		QJsonObject data_object = results_object["data"].toObject();

		if (data_object.contains("message")) {
			QString data_message = data_object["message"].toString();
			qfInfo() << "OFeed [competitor change]:" << data_message;
		} else {
			qfInfo() << "OFeed [competitor change]: ok, but no data message found.";
		}
	} else {
		qfError() << "OFeed [competitor change] Unexpected response:" << response;
	}
}
reply->deleteLater(); });
}

void OFeedClient::sendNewCompetitor(QString json_body)
{
	// Prepare the Authorization header base64 username:password
	QString combined = eventId() + ":" + eventPassword();
	QByteArray base_64_auth = combined.toUtf8().toBase64();
	QString auth_value = "Basic " + QString(base_64_auth);
	QByteArray auth_header = auth_value.toUtf8();

	// Create the URL for the POST request
	QUrl url(hostUrl() + "/rest/v1/events/" + eventId() + "/competitors");

	// Create the network request
	QNetworkRequest request(url);
	request.setRawHeader("Authorization", auth_header);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// Send request
	QNetworkReply *reply = m_networkManager->post(request, json_body.toUtf8());

	connect(reply, &QNetworkReply::finished, this, [=]()
			{
if(reply->error()) {
	qfError() << "OFeed [new competitor]:" << reply->errorString();
}
else {
	QByteArray response = reply->readAll();
	QJsonDocument json_response = QJsonDocument::fromJson(response);
	QJsonObject json_object = json_response.object();

	if (json_object.contains("error") && !json_object["error"].toBool()) {
		QJsonObject results_object = json_object["results"].toObject();
		QJsonObject data_object = results_object["data"].toObject();

		if (data_object.contains("message")) {
			QString data_message = data_object["message"].toString();
			qfInfo() << "OFeed [new competitor]:" << data_message;
		} else {
			qfInfo() << "OFeed [new competitor]: ok, but no data message found.";
		}
	} else {
		qfError() << "OFeed [new competitor] Unexpected response:" << response;
	}
}
reply->deleteLater(); });
}

int OFeedClient::getCompetitorExternalId(int ofeed_competitor_id){
	QUrl url(hostUrl() + "/rest/v1/events/" + eventId() + "/competitors/" + QString::number(ofeed_competitor_id));
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply *reply = m_networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [=]()
	{
		if(reply->error()) {
			qfError() << "OFeed [competitor detail]:" << reply->errorString();
		}
		else {
			QByteArray response = reply->readAll();
			QJsonDocument json_response = QJsonDocument::fromJson(response);
			QJsonObject json_object = json_response.object();

			
			if (json_object.contains("error") && !json_object["error"].toBool()) {
				QJsonObject results_object = json_object["results"].toObject();
				QJsonObject data_object = results_object["data"].toObject();

				if (data_object.contains("externalId")){
					return data_object["externalId"].toInt();
				}
				
			} else {
				qfError() << "OFeed [competitor detail] Unexpected response:" << response;
			}
		}
		reply->deleteLater();
	});
}

void OFeedClient::getChangesFromStart(){
	// Get changes
	// Prepare the Authorization header base64 username:password
	QString combined = eventId() + ":" + eventPassword();
	QByteArray base_64_auth = combined.toUtf8().toBase64();
	QString auth_value = "Basic " + QString(base_64_auth);
	QByteArray auth_header = auth_value.toUtf8();

	QDateTime lastChangelogCallValue = lastChangelogCall();
	QDateTime initialValue = QDateTime::fromSecsSinceEpoch(0); // Unix epoch
	// Create the URL for the POST request
	QUrl url(hostUrl() + "/rest/v1/events/" + eventId() + "/changelog?origin=START");

	// Create a QUrlQuery object to manage query parameters
    QUrlQuery query;

    // Check if lastChangelogCall is valid
    if (lastChangelogCallValue != initialValue) {
        // Add the 'since' parameter with the ISO 8601 UTC datetime
        query.addQueryItem("since", lastChangelogCallValue.toString(Qt::ISODate));
    }

    // Apply the query to the URL
    url.setQuery(query);
	qDebug() << "Changelog request URL:" << url.toString();

	// Create the network request
	QNetworkRequest request(url);
	request.setRawHeader("Authorization", auth_header);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// Send request
	QDateTime requestExecutonTime = QDateTime::currentDateTime();
	QNetworkReply *reply = m_networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [=]()
	{
		if(reply->error()) {
			qfError() << "OFeed [changelog]:" << reply->errorString();
		}
		else {
			QByteArray response = reply->readAll();
			QJsonDocument json_response = QJsonDocument::fromJson(response);
			QJsonObject json_object = json_response.object();

			
			if (json_object.contains("error") && !json_object["error"].toBool()) {
				QJsonObject results_object = json_object["results"].toObject();
				QJsonObject data_object = results_object["data"].toObject();
				processNewChangesFromStart(data_object);				
				
			} else {
				qfError() << "OFeed [changelog] Unexpected response:" << response;
			}
		}
		reply->deleteLater();
	});

	// Update last changelog call
	setLastChangelogCall(requestExecutonTime);
}

void OFeedClient::processNewChangesFromStart(QJsonObject data_object){
	if (data_object.isEmpty()) {
        qWarning() << "No changes to process: 'data_object' is empty.";
        return;
    }

    for (auto key : data_object.keys()) {
        QJsonValue value = data_object.value(key);

        if (!value.isObject()) {
            qWarning() << "Invalid change item: not a JSON object.";
            continue;
        }

        QJsonObject change = value.toObject();

        // Extract relevant fields
        int ofeed_competitor_id = change["competitorId"].toInt();
		int runs_id = getCompetitorExternalId(ofeed_competitor_id);
        QString type = change["type"].toString();
        QString previous_value = change["previousValue"].toString();
        QString new_value = change["newValue"].toString();

        qDebug() << "Processing change for competitorId (OFeed externalId):" << runs_id << ", type:" << type;

        // Handle each type of change
        if (type == "si_card_change") {
            processCardChange(runs_id, previous_value, new_value);
        } else if (type == "status_change" && new_value != "Active") {
            processStatusChange(runs_id, previous_value, new_value);
        } else if (type == "note_change") {
            processNoteChange(runs_id, new_value);
        } else {
            qWarning() << "Unknown change type:" << type;
        }

		// Store the processed change
    	storeChange(change);
    }
}

void OFeedClient::processCardChange(int runs_id, const QString &previous_value, const QString &new_value) {
    qDebug() << "Processing SI card change for runsId:" << runs_id << "from" << previous_value << "to" << new_value;

    qf::core::sql::Query q;
    q.prepare("UPDATE runs SET siId=:siId WHERE id=:runsId", qf::core::Exception::Throw);
    q.bindValue(":runsId", runs_id);
    q.bindValue(":siId", new_value.toInt());
    q.exec(qf::core::Exception::Throw);
}

void OFeedClient::processStatusChange(int runs_id, const QString &previous_value, const QString &new_value) {
    qDebug() << "Processing status change for runsId:" << runs_id << "from" << previous_value << "to" << new_value;
	bool notStart = new_value == "DidNotStart" ? true : false;

    qf::core::sql::Query q;
    q.prepare("UPDATE runs SET notStart=:notStart WHERE id=:runsId", qf::core::Exception::Throw);
    q.bindValue(":runsId", runs_id);
    q.bindValue(":notStart", notStart);
    q.exec(qf::core::Exception::Throw);
}

void OFeedClient::processNoteChange(int runs_id, const QString &new_value) {
    qDebug() << "Processing note change for runsId:" << runs_id << "with new note:" << new_value;
	int competitor_id = getPlugin<RunsPlugin>()->competitorForRun(runs_id);

    qf::core::sql::Query q;
    q.prepare("UPDATE competitors SET note = CASE WHEN note IS NULL OR note = '' THEN :newNote ELSE note || ', ' || :newNote END WHERE id = :competitorId", qf::core::Exception::Throw);
    q.bindValue(":competitorId", competitor_id);
    q.bindValue(":newNote", new_value);
    q.exec(qf::core::Exception::Throw);
}

void OFeedClient::storeChange(const QJsonObject &change) {
    qDebug() << "Storing processed change to the database table:" << change;

	int current_stage = getPlugin<EventPlugin>()->currentStageId();
	int ofeed_competitor_id = change["competitorId"].toInt();
	int runs_id = getCompetitorExternalId(ofeed_competitor_id);
	int competitor_id = getPlugin<RunsPlugin>()->competitorForRun(runs_id);

	QString previous_value = change["previousValue"].isString() ? change["previousValue"].toString() : QString();
	QString new_value = change["newValue"].isString() ? change["newValue"].toString() : QString();
	qf::core::sql::Query q;
	q.prepare("INSERT INTO qxchanges (data_type, data, source, user_id, stage_id, change_id, created)"
			" VALUES (:data_type, :data, :source, :user_id, :stage_id, :change_id, :created)");
	q.bindValue(":data_type", change["type"]);
	q.bindValue(":data", "previousValue: " + previous_value + ", newValue: " + new_value);
	q.bindValue(":source", "OFeed");
	q.bindValue(":user_id", competitor_id);
	q.bindValue(":stage_id", current_stage);
	q.bindValue(":change_id", change["id"]);
	q.bindValue(":created", change["createdAt"]);
	q.exec(qf::core::Exception::Throw);
}

static QString getIofResultStatus(
	int time,
	bool is_disq,
	bool is_disq_by_organizer,
	bool is_miss_punch,
	bool is_bad_check,
	bool is_did_not_start,
	bool is_did_not_finish,
	bool is_not_competing)
{
	// Handle time initial value
	if (time == -1)
	{
		return "Inactive";
	}
	// IOF xml 3.0 statuses:
	// OK (finished and validated)
	// Finished (finished but not yet validated.)
	// MissingPunch
	// Disqualified (for some other reason than a missing punch)
	// DidNotFinish
	// Active
	// Inactive
	// OverTime
	// SportingWithdrawal
	// NotCompeting
	// DidNotStart
	if (is_not_competing)
		return "NotCompeting";
	if (is_miss_punch)
		return "MissingPunch";
	if (is_did_not_finish)
		return "DidNotFinish";
	if (is_did_not_start)
		return "DidNotStart";
	if (is_bad_check || is_disq_by_organizer || is_disq)
		return "Disqualified";
	if (time)
		return "OK";   // OK
	return "Inactive"; // Inactive as default status
}

static QString datetime_to_string(const QDateTime &dt)
{
	return quickevent::core::Utils::dateTimeToIsoStringWithUtcOffset(dt);
}

void OFeedClient::onCompetitorAdded(int competitor_id)
{
	if (competitor_id == 0)
	{
		return;
	}

	int INT_INITIAL_VALUE = -1;

	int stage_id = getPlugin<EventPlugin>()->currentStageId();
	QDateTime stage_start_date_time = getPlugin<EventPlugin>()->stageStartDateTime(stage_id);
	qf::core::sql::Query q;
	q.exec("SELECT competitors.registration, "
		   "competitors.startNumber, "
		   "competitors.firstName, "
		   "competitors.lastName, "
		   "clubs.name AS organisationName, "
		   "clubs.abbr AS organisationAbbr, "
		   "classes.id AS classId, "
		   "runs.id AS runId, "
		   "runs.siId, "
		   "runs.disqualified, "
		   "runs.disqualifiedByOrganizer, "
		   "runs.misPunch, "
		   "runs.badCheck, "
		   "runs.notStart, "
		   "runs.notFinish, "
		   "runs.notCompeting, "
		   "runs.startTimeMs, "
		   "runs.finishTimeMs, "
		   "runs.timeMs "
		   "FROM runs "
		   "INNER JOIN competitors ON competitors.id = runs.competitorId "
		   "LEFT JOIN relays ON relays.id = runs.relayId  "
		   "LEFT JOIN clubs ON substr(competitors.registration, 1, 3) = clubs.abbr "
		   "INNER JOIN classes ON classes.id = competitors.classId OR classes.id = relays.classId  "
		   "WHERE competitors.id=" QF_IARG(competitor_id) " AND runs.stageId=" QF_IARG(stage_id),
		   qf::core::Exception::Throw);
	if (q.next())
	{
		int run_id = q.value("runId").toInt();
		QString registration = q.value(QStringLiteral("registration")).toString();
		QString first_name = q.value(QStringLiteral("firstName")).toString();
		QString last_name = q.value(QStringLiteral("lastName")).toString();
		int card_number = q.value(QStringLiteral("siId")).toInt();
		QString organisation_name = q.value(QStringLiteral("organisationName")).toString();
		QString organisation_abbr = q.value(QStringLiteral("organisationAbbr")).toString();
		QString organisation = !organisation_abbr.isEmpty() ? organisation_name : registration.left(3);
		int class_id = q.value(QStringLiteral("classId")).toInt();
		QString nationality = "";
		// int run_id = q.value("runId").toInt();
		QString origin = "IT";
		QString note = "Edited from Quickevent";

		// Start bib
		int start_bib = INT_INITIAL_VALUE;
		QVariant start_bib_variant = q.value(QStringLiteral("startNumber"));
		if (!start_bib_variant.isNull())
		{
			start_bib = start_bib_variant.toInt();
		}

		// Start time
		int start_time = INT_INITIAL_VALUE;
		QVariant start_time_variant = q.value(QStringLiteral("startTimeMs"));
		if (!start_time_variant.isNull())
		{
			start_time = start_time_variant.toInt();
		}

		// Finish time
		int finish_time = INT_INITIAL_VALUE;
		QVariant finish_time_variant = q.value(QStringLiteral("finishTimeMs"));
		if (!finish_time_variant.isNull())
		{
			finish_time = finish_time_variant.toInt();
		}

		// Time
		int running_time = INT_INITIAL_VALUE;
		QVariant running_time_variant = q.value(QStringLiteral("timeMs"));
		if (!running_time_variant.isNull())
		{
			running_time = running_time_variant.toInt();
		}

		// Status
		bool is_disq = q.value(QStringLiteral("disqualified")).toBool();
		bool is_disq_by_organizer = q.value(QStringLiteral("disqualifiedByOrganizer")).toBool();
		bool is_miss_punch = q.value(QStringLiteral("misPunch")).toBool();
		bool is_bad_check = q.value(QStringLiteral("badCheck")).toBool();
		bool is_did_not_start = q.value(QStringLiteral("notStart")).toBool();
		bool is_did_not_finish = q.value(QStringLiteral("notFinish")).toBool();
		bool is_not_competing = q.value(QStringLiteral("notCompeting")).toBool();
		QString status = getIofResultStatus(running_time, is_disq, is_disq_by_organizer, is_miss_punch, is_bad_check, is_did_not_start, is_did_not_finish, is_not_competing);

		// Use std::stringstream to build the JSON string
		std::stringstream json_payload;

		// Setup common values
		json_payload << "{"
					 << R"("origin":")" << origin.toStdString() << R"(",)"
					 << R"("firstname":")" << first_name.toStdString() << R"(",)"
					 << R"("lastname":")" << last_name.toStdString() << R"(",)"
					 << R"("registration":")" << registration.toStdString() << R"(",)"
					 << R"("organisation":")" << organisation.toStdString() << R"(",)"
					 << R"("status":")" << status.toStdString() << R"(",)"
					 << R"("note":")" << note.toStdString() << R"(",)";

		if (nationality != "")
		{
			json_payload << R"("nationality":")" << nationality.toStdString() << R"(",)";
		}

		// External ids
		json_payload << R"("classExternalId":")" << class_id << R"(",)"
						 << R"("externalId":")" << run_id << R"(",)";

		// Card number - QE saves 0 for empty si card
		if (card_number != 0)
		{
			json_payload << R"("card":)" << card_number << ",";
		}

		// Finish time
		if (finish_time != INT_INITIAL_VALUE)
		{
			json_payload << R"("finishTime":")" << datetime_to_string(stage_start_date_time.addMSecs(finish_time)).toStdString() << R"(",)";
		}

		// Star time
		if (start_time != INT_INITIAL_VALUE)
		{
			json_payload << R"("startTime":")" << datetime_to_string(stage_start_date_time.addMSecs(start_time)).toStdString() << R"(",)";
		}

		// Start bib
		if (start_bib != INT_INITIAL_VALUE)
		{
			json_payload << R"("bibNumber":)" << start_bib << ",";
		}

		//  Competitor's time
		if (running_time != INT_INITIAL_VALUE)
		{
			json_payload << R"("time":)" << running_time << ",";
		}

		// Get the final JSON string
		std::string json_str = json_payload.str();

		// Remove the trailing comma if necessary
		if (json_str.back() == ',')
		{
			json_str.pop_back();
		}

		json_str += "}";

		// Output the JSON for debugging
		std::cout << "OFeed - competitor added - json: " << json_str << std::endl;

		// Convert std::string to QString
		QString json_qstr = QString::fromStdString(json_str);

		// Send
		sendNewCompetitor(json_qstr);
	}
}

void OFeedClient::onCompetitorEdited(int competitor_id)
{
	if (competitor_id == 0)
	{
		return;
	}

	int INT_INITIAL_VALUE = -1;

	int stage_id = getPlugin<EventPlugin>()->currentStageId();
	QDateTime stage_start_date_time = getPlugin<EventPlugin>()->stageStartDateTime(stage_id);
	qf::core::sql::Query q;
	q.exec("SELECT competitors.registration, "
		   "competitors.startNumber, "
		   "competitors.firstName, "
		   "competitors.lastName, "
		   "clubs.name AS organisationName, "
		   "clubs.abbr AS organisationAbbr, "
		   "classes.id AS classId, "
		   "runs.id AS runId, "
		   "runs.siId, "
		   "runs.disqualified, "
		   "runs.disqualifiedByOrganizer, "
		   "runs.misPunch, "
		   "runs.badCheck, "
		   "runs.notStart, "
		   "runs.notFinish, "
		   "runs.notCompeting, "
		   "runs.startTimeMs, "
		   "runs.finishTimeMs, "
		   "runs.timeMs "
		   "FROM runs "
		   "INNER JOIN competitors ON competitors.id = runs.competitorId "
		   "LEFT JOIN relays ON relays.id = runs.relayId  "
		   "LEFT JOIN clubs ON substr(competitors.registration, 1, 3) = clubs.abbr "
		   "INNER JOIN classes ON classes.id = competitors.classId OR classes.id = relays.classId  "
		   "WHERE competitors.id=" QF_IARG(competitor_id) " AND runs.stageId=" QF_IARG(stage_id),
		   qf::core::Exception::Throw);
	if (q.next())
	{
		int run_id = q.value("runId").toInt();
		QString registration = q.value(QStringLiteral("registration")).toString();
		QString first_name = q.value(QStringLiteral("firstName")).toString();
		QString last_name = q.value(QStringLiteral("lastName")).toString();
		int card_number = q.value(QStringLiteral("siId")).toInt();
		QString organisation_name = q.value(QStringLiteral("organisationName")).toString();
		QString organisation_abbr = q.value(QStringLiteral("organisationAbbr")).toString();
		QString organisation = !organisation_abbr.isEmpty() ? organisation_name : registration.left(3);
		int class_id = q.value(QStringLiteral("classId")).toInt();
		QString nationality = "";
		QString origin = "IT";
		QString note = "Edited from Quickevent";

		// Start bib
		int start_bib = INT_INITIAL_VALUE;
		QVariant start_bib_variant = q.value(QStringLiteral("startNumber"));
		if (!start_bib_variant.isNull())
		{
			start_bib = start_bib_variant.toInt();
		}

		// Start time
		int start_time = INT_INITIAL_VALUE;
		QVariant start_time_variant = q.value(QStringLiteral("startTimeMs"));
		if (!start_time_variant.isNull())
		{
			start_time = start_time_variant.toInt();
		}

		// Finish time
		int finish_time = INT_INITIAL_VALUE;
		QVariant finish_time_variant = q.value(QStringLiteral("finishTimeMs"));
		if (!finish_time_variant.isNull())
		{
			finish_time = finish_time_variant.toInt();
		}

		// Time
		int running_time = INT_INITIAL_VALUE;
		QVariant running_time_variant = q.value(QStringLiteral("timeMs"));
		if (!running_time_variant.isNull())
		{
			running_time = running_time_variant.toInt();
		}

		// Status
		bool is_disq = q.value(QStringLiteral("disqualified")).toBool();
		bool is_disq_by_organizer = q.value(QStringLiteral("disqualifiedByOrganizer")).toBool();
		bool is_miss_punch = q.value(QStringLiteral("misPunch")).toBool();
		bool is_bad_check = q.value(QStringLiteral("badCheck")).toBool();
		bool is_did_not_start = q.value(QStringLiteral("notStart")).toBool();
		bool is_did_not_finish = q.value(QStringLiteral("notFinish")).toBool();
		bool is_not_competing = q.value(QStringLiteral("notCompeting")).toBool();
		QString status = getIofResultStatus(running_time, is_disq, is_disq_by_organizer, is_miss_punch, is_bad_check, is_did_not_start, is_did_not_finish, is_not_competing);

		// Use std::stringstream to build the JSON string
		std::stringstream json_payload;

		// Setup common values
		json_payload << "{"
					 << R"("origin":")" << origin.toStdString() << R"(",)"
					 << R"("firstname":")" << first_name.toStdString() << R"(",)"
					 << R"("lastname":")" << last_name.toStdString() << R"(",)"
					 << R"("registration":")" << registration.toStdString() << R"(",)"
					 << R"("organisation":")" << organisation.toStdString() << R"(",)"
					 << R"("status":")" << status.toStdString() << R"(",)"
					 << R"("note":")" << note.toStdString() << R"(",)";

		if (nationality != "")
		{
			json_payload << R"("nationality":")" << nationality.toStdString() << R"(",)";
		}

		// External ids
		json_payload << R"("useExternalId":true,)"
						 << R"("classExternalId":")" << class_id << R"(",)";
				
		// Card number - QE saves 0 for empty si card
		if (card_number != 0)
		{
			json_payload << R"("card":)" << card_number << ",";
		}

		// Finish time
		if (finish_time != INT_INITIAL_VALUE)
		{
			json_payload << R"("finishTime":")" << datetime_to_string(stage_start_date_time.addMSecs(finish_time)).toStdString() << R"(",)";
		}

		// Star time
		if (start_time != INT_INITIAL_VALUE)
		{
			json_payload << R"("startTime":")" << datetime_to_string(stage_start_date_time.addMSecs(start_time)).toStdString() << R"(",)";
		}

		// Start bib
		if (start_bib != INT_INITIAL_VALUE)
		{
			json_payload << R"("bibNumber":)" << start_bib << ",";
		}

		//  Competitor's time
		if (running_time != INT_INITIAL_VALUE)
		{
			json_payload << R"("time":)" << running_time << ",";
		}

		// Get the final JSON string
		std::string json_str = json_payload.str();

		// Remove the trailing comma if necessary
		if (json_str.back() == ',')
		{
			json_str.pop_back();
		}

		json_str += "}";

		// Output the JSON for debugging
		std::cout << "OFeed - competitor edited - json: " << json_str << std::endl;

		// Convert std::string to QString
		QString json_qstr = QString::fromStdString(json_str);

		// Send
		sendCompetitorChange(json_qstr, run_id);
	}
}

void OFeedClient::onCompetitorReadOut(int competitor_id)
{
	if (competitor_id == 0)
		return;

	int stage_id = getPlugin<EventPlugin>()->currentStageId();
	QDateTime stage_start_date_time = getPlugin<EventPlugin>()->stageStartDateTime(stage_id);
	qf::core::sql::Query q;
	q.exec("SELECT runs.id AS runId, "
		   "runs.disqualified, "
		   "runs.disqualifiedByOrganizer, "
		   "runs.misPunch, "
		   "runs.badCheck, "
		   "runs.notStart, "
		   "runs.notFinish, "
		   "runs.notCompeting, "
		   "runs.startTimeMs, "
		   "runs.finishTimeMs, "
		   "runs.timeMs "
		   "FROM runs "
		   "INNER JOIN competitors ON competitors.id = runs.competitorId "
		   "LEFT JOIN relays ON relays.id = runs.relayId  "
		   "INNER JOIN classes ON classes.id = competitors.classId OR classes.id = relays.classId  "
		   "WHERE competitors.id=" QF_IARG(competitor_id) " AND runs.stageId=" QF_IARG(stage_id),
		   qf::core::Exception::Throw);
	if (q.next())
	{
		int run_id = q.value("runId").toInt();
		bool is_disq = q.value(QStringLiteral("disqualified")).toBool();
		bool is_disq_by_organizer = q.value(QStringLiteral("disqualifiedByOrganizer")).toBool();
		bool is_miss_punch = q.value(QStringLiteral("misPunch")).toBool();
		bool is_bad_check = q.value(QStringLiteral("badCheck")).toBool();
		bool is_did_not_start = q.value(QStringLiteral("notStart")).toBool();
		bool is_did_not_finish = q.value(QStringLiteral("notFinish")).toBool();
		bool is_not_competing = q.value(QStringLiteral("notCompeting")).toBool();
		int start_time = q.value(QStringLiteral("startTimeMs")).toInt();
		int finish_time = q.value(QStringLiteral("finishTimeMs")).toInt();
		int running_time = q.value(QStringLiteral("timeMs")).toInt();
		QString status = getIofResultStatus(running_time, is_disq, is_disq_by_organizer, is_miss_punch, is_bad_check, is_did_not_start, is_did_not_finish, is_not_competing);
		QString origin = "IT";
		QString note = "Quickevent read-out";

		// Use std::stringstream to build the JSON string
		std::stringstream json_payload;
		json_payload << "{"
					 << R"("useExternalId":true,)"
					 << R"("origin":")" << origin.toStdString() << R"(",)"
					 << R"("startTime":")" << datetime_to_string(stage_start_date_time.addMSecs(start_time)).toStdString() << R"(",)"
					 << R"("finishTime":")" << datetime_to_string(stage_start_date_time.addMSecs(finish_time)).toStdString() << R"(",)"
					 << R"("time":)" << running_time / 1000 << ","
					 << R"("status":")" << status.toStdString() << R"(",)"
					 << R"("note":")" << note.toStdString() << R"(")"
					 << "}";

		// Get the final JSON string
		std::string json_str = json_payload.str();

		// Output the JSON for debugging
		std::cout << "OFeed - competitor read-out - json: " << json_str << std::endl;

		// Convert std::string to QString
		QString json_qstr = QString::fromStdString(json_str);

		sendCompetitorChange(json_qstr, run_id);
	}
}
}