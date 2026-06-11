#include "printawardsoptionsdialogwidget.h"
#include "ui_printawardsoptionsdialogwidget.h"
#include "runsplugin.h"

#include <awarddesigner/awarddesign.h>
#include <awarddesigner/awarddesignerdialog.h>

#include <qf/gui/framework/mainwindow.h>
#include <qf/core/log.h>

static const QLatin1String DB_PREFIX("db:");

PrintAwardsOptionsDialogWidget::PrintAwardsOptionsDialogWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::PrintAwardsOptionsDialogWidget)
{
	setPersistentSettingsId(objectName());
	ui->setupUi(this);
	ui->edNumPlaces->setValue(3);

	refreshTemplateList();

	connect(ui->btnDesigner, &QPushButton::clicked, this, &PrintAwardsOptionsDialogWidget::onDesignerClicked);
}

PrintAwardsOptionsDialogWidget::~PrintAwardsOptionsDialogWidget()
{
	delete ui;
}

void PrintAwardsOptionsDialogWidget::refreshTemplateList()
{
	QString currentData = ui->edReportPath->currentData().toString();
	ui->edReportPath->clear();

	auto *runs_plugin = qf::gui::framework::getPlugin<Runs::RunsPlugin>();
	for (const auto &i : runs_plugin->listReportFiles("awards")) {
		qfDebug() << i.reportName << i.reportFilePath;
		ui->edReportPath->addItem(i.reportName, i.reportFilePath);
	}

	for (const QString &name : AwardDesigner::Design::listFromDb(QStringLiteral("runs"))) {
		ui->edReportPath->addItem(QStringLiteral("★ ") + name,
			QString(DB_PREFIX) + name);
	}

	if (!currentData.isEmpty()) {
		int ix = ui->edReportPath->findData(currentData);
		if (ix >= 0)
			ui->edReportPath->setCurrentIndex(ix);
	}
}

QVariantMap PrintAwardsOptionsDialogWidget::printOptions() const
{
	QVariantMap ret;
	if (ui->edReportPath->currentIndex() >= 0) {
		ret["numPlaces"] = ui->edNumPlaces->value();
		ret["stageId"]   = ui->edStageNumber->value();
		ret["reportPath"] = ui->edReportPath->currentData().toString();
	}
	return ret;
}

void PrintAwardsOptionsDialogWidget::setPrintOptions(const QVariantMap &opts)
{
	ui->edNumPlaces->setValue(opts.value("numPlaces", 3).toInt());
	ui->edStageNumber->setValue(opts.value("stageId").toInt());
	QString report_path = opts.value("reportPath").toString();
	if (!report_path.isEmpty()) {
		int ix = ui->edReportPath->findData(report_path);
		if (ix >= 0)
			ui->edReportPath->setCurrentIndex(ix);
	}
}

void PrintAwardsOptionsDialogWidget::onDesignerClicked()
{
	AwardDesigner::Design design;
	QString currentData = ui->edReportPath->currentData().toString();
	if (currentData.startsWith(DB_PREFIX)) {
		QString name = currentData.mid(DB_PREFIX.size());
		design = AwardDesigner::Design::loadFromDb(name);
	}

	AwardDesignerDialog dlg(AwardDesigner::runsFields(), AwardDesigner::Design::defaultRunsDesign(), this);
	if (design.isValid())
		dlg.loadDesign(design);
	dlg.exec();

	refreshTemplateList();

	QString savedName = dlg.designName();
	if (!savedName.isEmpty()) {
		int ix = ui->edReportPath->findData(QString(DB_PREFIX) + savedName);
		if (ix >= 0)
			ui->edReportPath->setCurrentIndex(ix);
	}
}
