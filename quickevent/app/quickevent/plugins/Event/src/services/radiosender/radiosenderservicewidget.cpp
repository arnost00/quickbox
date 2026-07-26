#include "radiosenderservicewidget.h"
#include "ui_radiosenderservicewidget.h"
#include "radiosenderservice.h"

#include "../../eventplugin.h"

#include <qf/core/assert.h>
#include <qf/gui/framework/mainwindow.h>

#include <QDialog>

using qf::gui::framework::getPlugin;

namespace Event::services {

RadioSenderServiceWidget::RadioSenderServiceWidget(RadioSenderService *service, QWidget *parent)
	: Super(parent)
	, ui(new Ui::RadioSenderServiceWidget)
	, m_service(service)
{
	Q_ASSERT(m_service);
	setPersistentSettingsId(QStringLiteral("RadioSenderServiceWidget"));
	ui->setupUi(this);
	auto *event_plugin = getPlugin<EventPlugin>();
	const auto config = event_plugin->appDbConfig().radioSenderConfig();
	ui->edListenAddress->setText(config.listenAddress);
	ui->edPort->setValue(config.port);
	ui->edStartControl->setValue(config.startControl);
	ui->edFinishControl->setValue(config.finishControl);
	ui->edStartTolerance->setValue(config.startToleranceMs);
	ui->edFinishTolerance->setValue(config.finishToleranceMs);

	connect(m_service, &RadioSenderService::receivedLineLogged,
		this, &RadioSenderServiceWidget::updateReceivedLineLog);
	connect(m_service, &RadioSenderService::statusChanged,
		this, &RadioSenderServiceWidget::updateServiceControls);
	connect(ui->btStart, &QPushButton::clicked, this, [this] { m_service->setRunning(true); });
	connect(ui->btStop, &QPushButton::clicked, this, [this] { m_service->setRunning(false); });
	updateReceivedLineLog();
	updateServiceControls();
}

RadioSenderServiceWidget::~RadioSenderServiceWidget()
{
	delete ui;
}

bool RadioSenderServiceWidget::acceptDialogDone(int result)
{
	if (result == QDialog::Accepted)
		saveConfig();
	return true;
}

void RadioSenderServiceWidget::updateServiceControls()
{
	const bool running = m_service->isRunning();
	ui->btStart->setEnabled(!running);
	ui->btStop->setEnabled(running);
}

void RadioSenderServiceWidget::updateReceivedLineLog()
{
	ui->edReceivedLines->setPlainText(m_service->receivedLineLog().join(QStringLiteral("\n\n")));
}

void RadioSenderServiceWidget::saveConfig()
{
	auto *event_plugin = getPlugin<EventPlugin>();
	auto config = event_plugin->appDbConfig().radioSenderConfig();
	config.listenAddress = ui->edListenAddress->text().trimmed();
	config.port = ui->edPort->value();
	config.startControl = ui->edStartControl->value();
	config.finishControl = ui->edFinishControl->value();
	config.startToleranceMs = ui->edStartTolerance->value();
	config.finishToleranceMs = ui->edFinishTolerance->value();
	event_plugin->appDbConfig().setRadioSenderConfig(config);

	if (m_service->isRunning()) {
		m_service->stop();
		m_service->run();
	}
}

} // namespace Event::services
