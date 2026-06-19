#include "punchingtestservicewidget.h"
#include "ui_punchingtestservicewidget.h"
#include "punchingtestservice.h"

#include "../service.h"

#include <qf/core/assert.h>

#include <QDialog>

namespace Event::services {

PunchingTestServiceWidget::PunchingTestServiceWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::PunchingTestServiceWidget)
{
	setPersistentSettingsId("PunchingTestServiceWidget");
	ui->setupUi(this);

	PunchingTestService *svc = service();
	if (svc) {
		PunchingTestServiceSettings ss = svc->settings();
		ui->edPunchInterval->setValue(ss.punchInterval());
	}
}

PunchingTestServiceWidget::~PunchingTestServiceWidget()
{
	delete ui;
}

bool PunchingTestServiceWidget::acceptDialogDone(int result)
{
	if (result == QDialog::Accepted)
		saveSettings();
	return true;
}

PunchingTestService *PunchingTestServiceWidget::service()
{
	auto *svc = qobject_cast<PunchingTestService*>(
		Service::serviceByName(PunchingTestService::serviceName()));
	QF_ASSERT(svc, PunchingTestService::serviceName() + " doesn't exist", return nullptr);
	return svc;
}

void PunchingTestServiceWidget::saveSettings()
{
	PunchingTestService *svc = service();
	if (svc) {
		PunchingTestServiceSettings ss = svc->settings();
		ss.setPunchInterval(ui->edPunchInterval->value());
		svc->setSettings(ss);
	}
}

} // namespace Event::services
