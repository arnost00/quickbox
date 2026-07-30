#include "radiosenderservice.h"
#include "radiosenderservicewidget.h"
#include "radiosenderconfig.h"

#include "../../eventplugin.h"

#include <quickevent/core/si/checkedcard.h>

#include <qf/core/log.h>
#include <qf/core/sql/query.h>
#include <qf/gui/framework/application.h>
#include <qf/gui/framework/mainwindow.h>

#include <QTcpServer>
#include <QTcpSocket>

#include <cmath>

using qf::gui::framework::getPlugin;

namespace Event::services {

RadioSenderService::RadioSenderService(QObject *parent)
	: Super(serviceName(), parent)
{
	m_server = new QTcpServer(this);
	connect(m_server, &QTcpServer::newConnection, this, &RadioSenderService::onNewConnection);
	connect(getPlugin<EventPlugin>(), &EventPlugin::dbEventNotify,
		this, &RadioSenderService::onDbEventNotify, Qt::QueuedConnection);
}

QString RadioSenderService::serviceName()
{
	return QStringLiteral("RadioSender");
}

QString RadioSenderService::serviceDisplayName() const
{
	return tr("Radio Sender");
}

void RadioSenderService::run()
{
	startServer();
	if (m_server->isListening())
		Super::run();
}

void RadioSenderService::stop()
{
	m_server->close();
	for (QTcpSocket *socket : m_clients.keys()) {
		socket->disconnectFromHost();
		socket->deleteLater();
	}
	m_clients.clear();
	Super::stop();
}

qf::gui::framework::DialogWidget *RadioSenderService::createDetailWidget()
{
	return new RadioSenderServiceWidget(this);
}

void RadioSenderService::startServer()
{
	const auto config = getPlugin<EventPlugin>()->appDbConfig().radioSenderConfig();
	const QHostAddress address(config.listenAddress);
	if (address.isNull() || config.port <= 0 || config.port > 65535) {
		setStatusMessage(tr("Invalid listen address or port"));
		return;
	}
	if (!m_server->listen(address, static_cast<quint16>(config.port))) {
		setStatusMessage(tr("Cannot listen on %1:%2: %3")
			.arg(config.listenAddress).arg(config.port).arg(m_server->errorString()));
		return;
	}
	setStatusMessage(tr("Listening on %1:%2").arg(config.listenAddress).arg(config.port));
}

void RadioSenderService::onNewConnection()
{
	while (m_server->hasPendingConnections()) {
		QTcpSocket *socket = m_server->nextPendingConnection();
		m_clients.insert(socket, {});
		connect(socket, &QTcpSocket::readyRead, this, &RadioSenderService::onReadyRead);
		connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
			m_clients.remove(socket);
			socket->deleteLater();
			setStatusMessage(tr("Listening, %1 sender(s) connected").arg(m_clients.size()));
		});
		connect(socket, &QTcpSocket::errorOccurred, this, [socket](QAbstractSocket::SocketError) {
			qfWarning() << "RadioSender socket error:" << socket->errorString();
		});
		setStatusMessage(tr("Listening, %1 sender(s) connected").arg(m_clients.size()));
	}
}

void RadioSenderService::onReadyRead()
{
	auto *socket = qobject_cast<QTcpSocket*>(sender());
	if (!socket || !m_clients.contains(socket))
		return;
	QByteArray &buffer = m_clients[socket];
	buffer += socket->readAll();
	while (true) {
		const qsizetype eol = buffer.indexOf('\n');
		if (eol < 0)
			break;
		const QByteArray line = buffer.left(eol).trimmed();
		buffer.remove(0, eol + 1);
		if (!line.isEmpty())
			processLine(line);
	}
}

void RadioSenderService::processLine(const QByteArray &line)
{
    // {Control};{Type};{Bib};{Time:HH:mm:ss.fff};{Status};{Cancellation}
    enum { ColControl = 0, ColBib, ColTime, ColStatus, ColCancellation };
	const QList<QByteArray> fields = line.split(';');
	bool id_ok = false;
	bool control_ok = false;
	const int start_number = fields.value(ColBib).trimmed().toInt(&id_ok);
	const int control = fields.value(ColControl).trimmed().toInt(&control_ok);
	const QTime time = QTime::fromString(QString::fromLatin1(fields.value(ColTime).trimmed()), QStringLiteral("HH:mm:ss.zzz"));
	bool is_dns = fields.value(ColStatus).trimmed() == "DNS";
	bool is_cancellation = fields.value(ColCancellation).trimmed() == "ANN";
	const auto config = getPlugin<EventPlugin>()->appDbConfig().radioSenderConfig();
	const bool is_valid = (control == config.startControl || control == config.finishControl)
		&& (time.isValid() || is_cancellation || is_dns);
	QString parsed_data;
	if (is_valid) {
		parsed_data = QStringLiteral("%1, startNumber: %2, time=%3")
			.arg(control == config.startControl ? QStringLiteral("STA") : QStringLiteral("FIN"))
			.arg(start_number)
			.arg(time.toString(QStringLiteral("HH:mm:ss.zzz")));
	} else {
		parsed_data = QStringLiteral("invalid message");
	}
	m_receivedLineLog.append(QStringLiteral("%1\n  %2")
		.arg(QString::fromUtf8(line), parsed_data));
	while (m_receivedLineLog.size() > 20) {
		m_receivedLineLog.removeFirst();
	}
	emit receivedLineLogged();
	if (!is_valid) {
		qfWarning() << "RadioSender: invalid message:" << line;
		return;
	}

	auto *event_plugin = getPlugin<EventPlugin>();
	const int stage_id = event_plugin->currentStageId();
	qf::core::sql::Query q;
	q.prepare("SELECT runs.id FROM runs JOIN competitors ON competitors.id=runs.competitorId"
		" WHERE runs.stageId=:stageId"
		" AND runs.isRunning"
		" AND competitors.startNumber=:startNumber");
	q.bindValue(QStringLiteral(":stageId"), stage_id);
	q.bindValue(QStringLiteral(":startNumber"), start_number);
	q.exec(qf::core::Exception::Throw);
	if (!q.next()) {
		qfWarning() << "RadioSender: competitor not found:" << start_number;
		return;
	}
	const int run_id = q.value(0).toInt();
	if (q.next()) {
		qfWarning() << "RadioSender: ambiguous competitor id:" << start_number;
		return;
	}

	QVariantMap record;
	const QString field = control == config.startControl
				? QStringLiteral("startGateTime")
				: QStringLiteral("finishGateTime");
	if (time.isValid()) {
	    const QDateTime stage_start = event_plugin->stageStartDateTime(stage_id);
	    QDateTime gate_time(stage_start.date(), time);
	    record[field] = gate_time;
	}
	if (is_dns) {
		record[QStringLiteral("notstart")] = true;
	}
	if (is_cancellation) {
	    record[field] = {};
	}

	qf::gui::framework::Application::instance()->updateDbRecord( QStringLiteral("runs"), run_id, record, this);
	updateRunTime(run_id);
}

void RadioSenderService::updateRunTime(int run_id)
{
	auto *event_plugin = getPlugin<EventPlugin>();
	qf::core::sql::Query q;
	q.prepare(QStringLiteral("SELECT stageId, startTimeMs, finishTimeMs, startGateTime, finishGateTime FROM runs WHERE id=:id"));
	q.bindValue(QStringLiteral(":id"), run_id);
	q.exec(qf::core::Exception::Throw);
	if (!q.next() || q.value(QStringLiteral("finishTimeMs")).isNull())
		return;

	const int stage_id = q.value(QStringLiteral("stageId")).toInt();
	const QDateTime stage_start = event_plugin->stageStartDateTime(stage_id);
	const qint64 start_time = q.value(QStringLiteral("startTimeMs")).toLongLong();
	const qint64 finish_time = q.value(QStringLiteral("finishTimeMs")).toLongLong();
	const QVariant start_gate_value = q.value(QStringLiteral("startGateTime"));
	const QVariant finish_gate_value = q.value(QStringLiteral("finishGateTime"));
	const qint64 start_gate = stage_start.msecsTo(start_gate_value.toDateTime());
	const qint64 finish_gate = stage_start.msecsTo(finish_gate_value.toDateTime());
	const auto &config = event_plugin->appDbConfig().radioSenderConfig();
	const bool use_start_gate = !start_gate_value.isNull()
		&& std::abs(start_gate - start_time) <= config.startToleranceMs;
	const bool use_finish_gate = !finish_gate_value.isNull()
		&& std::abs(finish_gate - finish_time) <= config.finishToleranceMs;
	const qint64 result_time = (use_finish_gate ? finish_gate : finish_time)
		- (use_start_gate ? start_gate : start_time);
	if (result_time <= 0)
		return;

	qf::gui::framework::Application::instance()->updateDbRecord(
		QStringLiteral("runs"), run_id, {{QStringLiteral("timeMs"), result_time}}, this);
}

void RadioSenderService::onDbEventNotify(const QString &domain, int connection_id, const QVariant &data)
{
	Q_UNUSED(connection_id)
	if (domain != QLatin1String(EventPlugin::DBEVENT_CARD_PROCESSED_AND_ASSIGNED))
		return;
	const quickevent::core::si::CheckedCard card(data.toMap());
	if (card.runId() > 0)
		updateRunTime(card.runId());
}

} // namespace Event::services
