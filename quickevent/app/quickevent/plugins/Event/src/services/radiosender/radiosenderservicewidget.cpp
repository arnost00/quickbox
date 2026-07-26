#include "radiosenderservicewidget.h"
#include "ui_radiosenderservicewidget.h"
#include "radiosenderservice.h"

#include "../../eventplugin.h"

#include <qf/gui/framework/mainwindow.h>

#include <QDialog>

using qf::gui::framework::getPlugin;

namespace Event::services {

RadioSenderServiceWidget::RadioSenderServiceWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::RadioSenderServiceWidget)
{
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

	m_service = qobject_cast<RadioSenderService*>(Service::serviceByName(RadioSenderService::serviceName()));
	if (m_service) {
		connect(m_service, &RadioSenderService::receivedLineLogged,
			this, &RadioSenderServiceWidget::updateReceivedLineLog);
		updateReceivedLineLog();
	}
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

void RadioSenderServiceWidget::updateReceivedLineLog()
{
	if (m_service)
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

	auto *service = qobject_cast<RadioSenderService*>(Service::serviceByName(RadioSenderService::serviceName()));
	if (service && service->isRunning()) {
		service->stop();
		service->run();
	}
}

} // namespace Event::services
