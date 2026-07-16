#include "stagewidget.h"
#include "ui_stagewidget.h"

#include "stagedocument.h"
#include "eventconfig.h"
#include "eventplugin.h"

#include <qf/gui/framework/mainwindow.h>

#include <QTimeZone>

using namespace Event;
using qf::gui::framework::getPlugin;

StageWidget::StageWidget(QWidget *parent) :
	Super(parent),
	ui(new Ui::StageWidget)
{
	setPersistentSettingsId("StageWidget");
	ui->setupUi(this);

	setTitle(tr("Stage"));
	setWindowTitle(tr("Edit Stage"));
	dataController()->setDocument(new StageDocument(this));
}

StageWidget::~StageWidget()
{
	delete ui;
}

bool StageWidget::load(const QVariant &id, int mode)
{
	m_stageId = id.toInt();
	bool ok  = Super::load(id, mode);
	if(ok) {
		const QString config_key = QStringLiteral("event.stage.%1.startDateTime").arg(m_stageId);
		QDateTime dt = getPlugin<EventPlugin>()->eventConfig()->value(config_key).toDateTime();
		if(dt.isValid()) {
			dt = dt.toTimeZone(QTimeZone::systemTimeZone());
		}
		else {
			// Backward compatibility for events whose start time is still in stages.
			qf::gui::model::DataDocument *doc = dataDocument();
			dt = doc->value(QStringLiteral("startDateTime")).toDateTime().toLocalTime();
		}

		ui->dateEdit->setDate(dt.date());
		ui->timeEdit->setTime(dt.time());
	}
	return ok;
}

bool StageWidget::saveData()
{
	QDate d = ui->dateEdit->date();
	QTime t = ui->timeEdit->time();
	QDateTime dt(d, t, QTimeZone::systemTimeZone());
	if(!Super::saveData())
		return false;

	EventConfig *config = getPlugin<EventPlugin>()->eventConfig();
	const QString config_key = QStringLiteral("event.stage.%1.startDateTime").arg(m_stageId);
	config->setValue(config_key, dt);
	config->save(config_key);
	return true;
}
