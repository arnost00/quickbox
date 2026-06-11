#include "printrelayawardsoptionsdialogwidget.h"
#include "ui_printrelayawardsoptionsdialogwidget.h"
#include "relaysplugin.h"

#include <awarddesigner/awarddesign.h>
#include <awarddesigner/awarddesignerdialog.h>

#include <qf/gui/framework/mainwindow.h>
#include <qf/core/log.h>

static const QLatin1String DB_PREFIX("db:");

PrintRelayAwardsOptionsDialogWidget::PrintRelayAwardsOptionsDialogWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::PrintRelayAwardsOptionsDialogWidget)
{
	setPersistentSettingsId(objectName());
	ui->setupUi(this);

	refreshTemplateList();

	connect(ui->btnDesigner, &QPushButton::clicked, this, &PrintRelayAwardsOptionsDialogWidget::onDesignerClicked);
}

PrintRelayAwardsOptionsDialogWidget::~PrintRelayAwardsOptionsDialogWidget()
{
	delete ui;
}

void PrintRelayAwardsOptionsDialogWidget::refreshTemplateList()
{
	QString currentData = ui->edReportPath->currentData().toString();
	ui->edReportPath->clear();

	// QML templates
	auto *relays_plugin = qf::gui::framework::getPlugin<Relays::RelaysPlugin>();
	for (const auto &i : relays_plugin->listReportFiles("awards")) {
		qfDebug() << i.reportName << i.reportFilePath;
		ui->edReportPath->addItem(i.reportName, i.reportFilePath);
	}

	// DB-stored designer templates
	for (const QString &name : AwardDesigner::Design::listFromDb(QStringLiteral("relay"))) {
		ui->edReportPath->addItem(QStringLiteral("★ ") + name,
			QString(DB_PREFIX) + name);
	}

	// Restore previous selection
	if (!currentData.isEmpty()) {
		int ix = ui->edReportPath->findData(currentData);
		if (ix >= 0)
			ui->edReportPath->setCurrentIndex(ix);
	}
}

QVariantMap PrintRelayAwardsOptionsDialogWidget::printOptions() const
{
	QVariantMap ret;
	if (ui->edReportPath->currentIndex() >= 0) {
		ret["numPlaces"] = ui->edNumPlaces->value();
		ret["reportPath"] = ui->edReportPath->currentData().toString();
	}
	return ret;
}

void PrintRelayAwardsOptionsDialogWidget::setPrintOptions(const QVariantMap &opts)
{
	ui->edNumPlaces->setValue(opts.value("numPlaces", 3).toInt());
	QString report_path = opts.value("reportPath").toString();
	if (!report_path.isEmpty()) {
		int ix = ui->edReportPath->findData(report_path);
		if (ix >= 0)
			ui->edReportPath->setCurrentIndex(ix);
	}
}

void PrintRelayAwardsOptionsDialogWidget::onDesignerClicked()
{
	// Load currently selected design if it is a DB design
	AwardDesigner::Design design;
	QString currentData = ui->edReportPath->currentData().toString();
	if (currentData.startsWith(DB_PREFIX)) {
		QString name = currentData.mid(DB_PREFIX.size());
		design = AwardDesigner::Design::loadFromDb(name);
	}

	AwardDesignerDialog dlg(AwardDesigner::relayFields(), AwardDesigner::Design::defaultRelayDesign(), this);
	if (design.isValid())
		dlg.loadDesign(design);
	dlg.exec();

	// Refresh dropdown so any newly saved designs appear
	refreshTemplateList();

	// Try to select the design that was just edited/created
	QString savedName = dlg.designName();
	if (!savedName.isEmpty()) {
		int ix = ui->edReportPath->findData(QString(DB_PREFIX) + savedName);
		if (ix >= 0)
			ui->edReportPath->setCurrentIndex(ix);
	}
}
