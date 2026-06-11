#include "printrelayawardsoptionsdialogwidget.h"
#include "ui_printrelayawardsoptionsdialogwidget.h"
#include "relaysplugin.h"

#include <qf/gui/framework/mainwindow.h>
#include <qf/core/log.h>

PrintRelayAwardsOptionsDialogWidget::PrintRelayAwardsOptionsDialogWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::PrintRelayAwardsOptionsDialogWidget)
{
	setPersistentSettingsId(objectName());
	ui->setupUi(this);

	auto *relays_plugin = qf::gui::framework::getPlugin<Relays::RelaysPlugin>();
	for(const auto &i : relays_plugin->listReportFiles("awards")) {
		qfDebug() << i.reportName << i.reportFilePath;
		ui->edReportPath->addItem(i.reportName, i.reportFilePath);
	}
}

PrintRelayAwardsOptionsDialogWidget::~PrintRelayAwardsOptionsDialogWidget()
{
	delete ui;
}

QVariantMap PrintRelayAwardsOptionsDialogWidget::printOptions() const
{
	QVariantMap ret;
	if(ui->edReportPath->currentIndex() >= 0) {
		ret["numPlaces"] = ui->edNumPlaces->value();
		ret["reportPath"] = ui->edReportPath->currentData().toString();
	}
	return ret;
}

void PrintRelayAwardsOptionsDialogWidget::setPrintOptions(const QVariantMap &opts)
{
	ui->edNumPlaces->setValue(opts.value("numPlaces", 3).toInt());
	QString report_path = opts.value("reportPath").toString();
	if(!report_path.isEmpty()) {
		int ix = ui->edReportPath->findData(report_path);
		if(ix >= 0)
			ui->edReportPath->setCurrentIndex(ix);
	}
}
