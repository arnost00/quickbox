#pragma once

#include "../service.h"

#include <QStringList>

class QTcpServer;
class QTcpSocket;

namespace Event::services {

class RadioSenderService : public Service
{
	Q_OBJECT
	using Super = Service;
public:
	explicit RadioSenderService(QObject *parent);

	void run() override;
	void stop() override;

	static QString serviceName();
	QString serviceDisplayName() const override;
	const QStringList &receivedLineLog() const { return m_receivedLineLog; }

signals:
	void receivedLineLogged();

private:
	qf::gui::framework::DialogWidget *createDetailWidget() override;
	void startServer();
	void onNewConnection();
	void onReadyRead();
	void processLine(const QByteArray &line);
	void updateRunTime(int run_id);
	void onDbEventNotify(const QString &domain, int connection_id, const QVariant &data);

	QTcpServer *m_server = nullptr;
	QHash<QTcpSocket*, QByteArray> m_clients;
	QStringList m_receivedLineLog;
};

} // namespace Event::services
