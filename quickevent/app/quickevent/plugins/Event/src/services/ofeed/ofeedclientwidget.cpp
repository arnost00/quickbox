#include "ofeedclientwidget.h"
#include "ui_ofeedclientwidget.h"
#include "ofeedclient.h"

#include <qf/gui/framework/mainwindow.h>
#include <qf/gui/dialogs/messagebox.h>

#include <qf/core/assert.h>
#include <qf/core/log.h>

#include <QFileDialog>
#include <QPointer>

#include <plugins/Event/src/eventplugin.h>

namespace Event::services {

OFeedClientWidget::OFeedClientWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::OFeedClientWidget)
{
	setPersistentSettingsId("OFeedClientWidget");
	ui->setupUi(this);

	OFeedClient *svc = service();
	if(svc) {
		OFeedClientSettings ss = svc->settings();
		ui->edExportInterval->setValue(ss.exportIntervalSec());
		ui->edHostUrl->setText(svc->hostUrl());
		ui->edEventId->setText(svc->eventId());
		ui->edEventPassword->setText(svc->eventPassword());
		ui->edChangelogOrigin->setText(svc->changelogOrigin());
		ui->additionalSettingsRunXmlValidation->setChecked(svc->runXmlValidation());
		ui->additionalSettingsPrintEventImageOnReceipt->setChecked(svc->printEventImageOnReceipt());
		ui->edReceiptImageHeight->setValue(svc->receiptImageHeightMm());
		ui->lbReceiptImageHeight->setEnabled(svc->printEventImageOnReceipt());
		ui->edReceiptImageHeight->setEnabled(svc->printEventImageOnReceipt());
		ui->lbEventImageCacheStatus->setText(svc->hasCachedEventImage() ? tr("Cached image is available") : tr("No cached image"));
		ui->processChangesOnOffButton->setText(svc->runChangesProcessing() ? tr("ON") : tr("OFF"));
		ui->processChangesOnOffButton->setChecked(svc->runChangesProcessing());
		ui->processChangesOnOffButton->setStyleSheet(
		"QPushButton {"
		"  padding: 5px;"
		"  border-radius: 4px;"
		"  border: 2px solid gray;"
		"}"
		"QPushButton:checked {"
		"  border: 2px solid green;"
		"  color: green;"
		"}"
		"QPushButton:!checked {"
		"  border: 2px solid red;"
		"  color: red;"
		"}"
		);
		ui->processChangesOnOffLabel->setText(svc->runChangesProcessing() ? tr("Changes are automatically processed") : tr("Processing changes is deactivated"));
	}

	connect(ui->btExportResultsXml30, &QPushButton::clicked, this, &OFeedClientWidget::onBtExportResultsXml30Clicked);
	connect(ui->btExportStartListXml30, &QPushButton::clicked, this, &OFeedClientWidget::onBtExportStartListXml30Clicked);
	connect(ui->processChangesOnOffButton, &QPushButton::clicked,this, &OFeedClientWidget::onProcessChangesOnOffButtonClicked);
	connect(ui->btTestConnection, &QPushButton::clicked, this, &OFeedClientWidget::onBtTestConnectionClicked);
	connect(ui->btRefreshEventImage, &QPushButton::clicked, this, &OFeedClientWidget::onBtRefreshEventImageClicked);
	connect(ui->additionalSettingsPrintEventImageOnReceipt, &QCheckBox::toggled, this, [this](bool on) {
		ui->lbReceiptImageHeight->setEnabled(on);
		ui->edReceiptImageHeight->setEnabled(on);
	});
	connect(ui->edHostUrl, &QLineEdit::textChanged, this, &OFeedClientWidget::updateTestConnectionState);
	connect(ui->edEventId, &QLineEdit::textChanged, this, &OFeedClientWidget::updateTestConnectionState);
	connect(ui->edEventPassword, &QLineEdit::textChanged, this, &OFeedClientWidget::updateTestConnectionState);
	updateTestConnectionState();
}

OFeedClientWidget::~OFeedClientWidget()
{
	delete ui;
}

bool OFeedClientWidget::acceptDialogDone(int result)
{
	if(result == QDialog::Accepted) {
		if(!saveSettings()) {
			return false;
		}
	}
	return true;
}

OFeedClient *OFeedClientWidget::service()
{
	auto *svc = qobject_cast<OFeedClient*>(Service::serviceByName(OFeedClient::serviceName()));
	QF_ASSERT(svc, OFeedClient::serviceName() + " doesn't exist", return nullptr);
	return svc;
}

bool OFeedClientWidget::saveSettings()
{
	OFeedClient *svc = service();
	if(svc) {
		OFeedClientSettings ss = svc->settings();
		ss.setExportIntervalSec(ui->edExportInterval->value());
		svc->setHostUrl(ui->edHostUrl->text().trimmed());
		svc->setEventId(ui->edEventId->text().trimmed());
		svc->setEventPassword(ui->edEventPassword->text().trimmed());
		svc->setChangelogOrigin(ui->edChangelogOrigin->text().trimmed());
		svc->setRunXmlValidation(ui->additionalSettingsRunXmlValidation->isChecked());
		svc->setPrintEventImageOnReceipt(ui->additionalSettingsPrintEventImageOnReceipt->isChecked());
		svc->setReceiptImageHeightMm(ui->edReceiptImageHeight->value());
		svc->setSettings(ss);
	}
	return true;
}

void OFeedClientWidget::onBtExportResultsXml30Clicked()
{
	OFeedClient *svc = service();
	if(svc) {
		saveSettings();
		qfInfo() << OFeedClient::serviceName() + " [results - manual upload]";
		svc->exportResultsIofXml3();
	}
}

void OFeedClientWidget::onBtExportStartListXml30Clicked()
{
	OFeedClient *svc = service();
	if(svc) {
		saveSettings();
		qfInfo() << OFeedClient::serviceName() + " [startlist - manual upload]";
		svc->exportStartListIofXml3();
	}
}

void OFeedClientWidget::onProcessChangesOnOffButtonClicked()
{
	OFeedClient *svc = service();
    if (!svc)
        return;

    bool newState = !svc->runChangesProcessing();
    svc->setRunChangesProcessing(newState);

    // Update button text or icon
    ui->processChangesOnOffButton->setText(newState ? tr("ON") : tr("OFF"));
	ui->processChangesOnOffButton->setChecked(svc->runChangesProcessing());
	ui->processChangesOnOffLabel->setText(svc->runChangesProcessing() ? tr("Changes are automatically processed") : tr("Processing changes is deactivated"));
}

void OFeedClientWidget::onBtTestConnectionClicked()
{
	OFeedClient *svc = service();
	if(!svc) {
		return;
	}

	m_isTestConnectionRunning = true;
	ui->btTestConnection->setText(tr("Testing..."));
	ui->lbConnectionTestResult->setStyleSheet("color:#666;");
	ui->lbConnectionTestResult->setText(tr("Testing connection..."));
	updateTestConnectionState();

	const QString host_url = ui->edHostUrl->text().trimmed();
	const QString event_id = ui->edEventId->text().trimmed();
	const QString event_password = ui->edEventPassword->text().trimmed();
	QPointer<OFeedClientWidget> widget_guard(this);

	svc->testConnection(host_url, event_id, event_password, [widget_guard](bool success, const QString &message) {
		if(!widget_guard) {
			return;
		}
		widget_guard->m_isTestConnectionRunning = false;
		widget_guard->ui->btTestConnection->setText(widget_guard->tr("Test connection"));
		widget_guard->ui->lbConnectionTestResult->setStyleSheet(success ? "color:#0a7a2f;" : "color:#b00020;");
		widget_guard->ui->lbConnectionTestResult->setText(message);
		widget_guard->updateTestConnectionState();
	});
}

void OFeedClientWidget::onBtRefreshEventImageClicked()
{
	OFeedClient *svc = service();
	if(!svc)
		return;

	saveSettings();
	m_isImageRefreshRunning = true;
	ui->lbEventImageCacheStatus->setStyleSheet("color:#666;");
	ui->lbEventImageCacheStatus->setText(tr("Refreshing image cache..."));
	updateTestConnectionState();

	QPointer<OFeedClientWidget> widget_guard(this);
	svc->refreshEventImageCache([widget_guard](bool success, const QString &message) {
		if(!widget_guard)
			return;
		widget_guard->m_isImageRefreshRunning = false;
		widget_guard->ui->lbEventImageCacheStatus->setStyleSheet(success ? "color:#0a7a2f;" : "color:#b00020;");
		widget_guard->ui->lbEventImageCacheStatus->setText(message);
		widget_guard->updateTestConnectionState();
	});
}

void OFeedClientWidget::updateTestConnectionState()
{
	const bool has_required_credentials = !ui->edHostUrl->text().trimmed().isEmpty()
		&& !ui->edEventId->text().trimmed().isEmpty()
		&& !ui->edEventPassword->text().trimmed().isEmpty();
	ui->btTestConnection->setEnabled(has_required_credentials && !m_isTestConnectionRunning);
	ui->btRefreshEventImage->setEnabled(has_required_credentials && !m_isImageRefreshRunning);
}
}
