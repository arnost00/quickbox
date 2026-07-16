#include "eventdialogwidget.h"
#include "ui_eventdialogwidget.h"

#include "eventconfig.h"

#include <qf/core/collator.h>
#include <qf/core/utils.h>

#include <QDateTimeEdit>
#include <QHeaderView>
#include <QTableWidget>

Event::EventConfigData Event::EventConfigData::fromVariantMap(const QVariantMap &values)
{
	EventConfigData data;
	data.stageCount = values.value("stageCount", 1).toInt();
	const QVariantMap stages = values.value("stage").toMap();
	for(auto it = stages.cbegin(); it != stages.cend(); ++it) {
		const int stage_id = it.key().toInt();
		if(stage_id <= 0)
			continue;
		const QVariantMap stage_values = it.value().toMap();
		StageData stage;
		stage.startDateTime = stage_values.value("startDateTime").toDateTime();
		stage.useAllMaps = stage_values.value("useAllMaps").toBool();
		const QVariant drawing_config = stage_values.value("drawingConfig");
		stage.drawingConfig = drawing_config.userType() == qMetaTypeId<QString>()
			? qf::core::Utils::jsonToQVariant(drawing_config.toString()).toMap()
			: drawing_config.toMap();
		stage.qxApiToken = stage_values.value("qxApiToken").toString();
		data.stages.insert(stage_id, stage);
	}
	data.name = values.value("name").toString();
	data.date = values.value("date").toDate();
	data.time = values.value("time").toTime();
	data.description = values.value("description").toString();
	data.place = values.value("place").toString();
	data.mainReferee = values.value("mainReferee").toString();
	data.director = values.value("director").toString();
	data.handicapLength = values.value("handicapLength").toInt();
	data.sportId = values.value("sportId").toInt();
	data.disciplineId = values.value("disciplineId").toInt();
	data.importId = values.value("importId").toInt();
	data.orisEventKey = values.value("orisEventKey").toString();
	data.cardCheckTimeSec = values.value("cardChechCheckTimeSec").toInt();
	data.oneTenthSecResults = values.value("oneTenthSecResults").toInt();
	data.iofRace = values.value("iofRace").toBool();
	data.iofXmlRaceNumber = values.value("iofXmlRaceNumber").toInt();
	data.currentStageId = values.value("currentStageId", 1).toInt();
	return data;
}

QVariantMap Event::EventConfigData::toVariantMap() const
{
	QVariantMap values;
	values.insert("stageCount", stageCount);
	QVariantMap stage_values;
	for(auto it = stages.cbegin(); it != stages.cend(); ++it) {
		QVariantMap stage;
		stage.insert("startDateTime", it.value().startDateTime);
		stage.insert("useAllMaps", it.value().useAllMaps);
		stage.insert("drawingConfig", qf::core::Utils::qvariantToJson(it.value().drawingConfig));
		stage.insert("qxApiToken", it.value().qxApiToken);
		stage_values.insert(QString::number(it.key()), stage);
	}
	values.insert("stage", stage_values);
	values.insert("name", name);
	values.insert("date", date);
	values.insert("time", time);
	values.insert("description", description);
	values.insert("place", place);
	values.insert("mainReferee", mainReferee);
	values.insert("director", director);
	values.insert("handicapLength", handicapLength);
	values.insert("sportId", sportId);
	values.insert("disciplineId", disciplineId);
	values.insert("importId", importId);
	values.insert("orisEventKey", orisEventKey);
	values.insert("cardChechCheckTimeSec", cardCheckTimeSec);
	values.insert("oneTenthSecResults", oneTenthSecResults);
	values.insert("iofRace", iofRace);
	values.insert("iofXmlRaceNumber", iofXmlRaceNumber);
	values.insert("currentStageId", currentStageId);
	return values;
}

EventDialogWidget::EventDialogWidget(QWidget *parent) :
	Super(parent),
	ui(new Ui::EventDialogWidget)
{
	setPersistentSettingsId("EventDialogWidget");
	ui->setupUi(this);

	ui->stageStartTimesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->stageStartTimesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	ui->stageStartTimesTable->verticalHeader()->hide();
	connect(ui->ed_stageCount, QOverload<int>::of(&QSpinBox::valueChanged), this, &EventDialogWidget::updateStageStartTimeEditors);
	updateStageStartTimeEditors(ui->ed_stageCount->value());

	connect(ui->ed_iofRace, &QAbstractButton::toggled, ui->frameIofRace, &QWidget::setVisible);
	ui->frameIofRace->hide();

	connect(ui->ed_orisRace, &QAbstractButton::toggled, ui->frameOrisRace, &QWidget::setVisible);
	ui->frameOrisRace->hide();

	using S = Event::EventConfig::Sport;
	for (S sport : {S::OB, S::LOB, S::MTBO, S::TRAIL})
		ui->cbxSportId->addItem(sportName(static_cast<int>(sport)), static_cast<int>(sport));

	using D = Event::EventConfig::Discipline;
	for (D disc : {D::LongDistance, D::ShortDistance, D::UltralongDistance, D::Sprint,
	               D::Relays, D::Teams, D::FreeOrder, D::NightRace, D::SprintRelays,
	               D::KnocOutSprint, D::TempO, D::MultiStages, D::Indoor, D::MassStart}) {
		ui->cbxDisciplineId->addItem(disciplineName(static_cast<int>(disc)), static_cast<int>(disc));
	}


	ui->ed_oneTenthSecResults->setDisabled(true);

	QRegularExpression rx("[a-z][a-z0-9_]*"); // PostgreSQL schema must start with small letter and it may contain small letters, digits and underscores only.
	QValidator *validator = new QRegularExpressionValidator(rx, this);
	ui->ed_eventId->setValidator(validator);
}

EventDialogWidget::~EventDialogWidget()
{
	delete ui;
}

void EventDialogWidget::setEventId(const QString &event_id)
{
	QByteArray la = qf::core::Collator::toAscii7(QLocale::Czech, event_id, true);
	ui->ed_eventId->setText(QString::fromUtf8(la));
}

QString EventDialogWidget::eventId() const
{
	QString event_id = ui->ed_eventId->text();
	QByteArray la = qf::core::Collator::toAscii7(QLocale::Czech, event_id, true);
	return QString::fromUtf8(la);
}

void EventDialogWidget::setEventIdEditable(bool b)
{
	ui->ed_eventId->setReadOnly(!b);
}

void EventDialogWidget::updateStageStartTimeEditors(int stage_count)
{
	QTableWidget *table = ui->stageStartTimesTable;
	const int old_count = table->rowCount();
	if(stage_count < old_count) {
		table->setRowCount(stage_count);
		updateStageStartTimesTableHeight();
		return;
	}

	QDateTime next_start(ui->ed_date->date(), ui->ed_time->time());
	if(old_count > 0) {
		if(auto *last_editor = qobject_cast<QDateTimeEdit *>(table->cellWidget(old_count - 1, 1)))
			next_start = last_editor->dateTime().addDays(1);
	}

	table->setRowCount(stage_count);
	for(int row = old_count; row < stage_count; ++row) {
		auto *stage_item = new QTableWidgetItem(QString::number(row + 1));
		stage_item->setTextAlignment(Qt::AlignCenter);
		table->setItem(row, 0, stage_item);

		auto *editor = new QDateTimeEdit(next_start, table);
		editor->setCalendarPopup(true);
		editor->setDisplayFormat(QStringLiteral("dd.MM.yyyy HH:mm:ss"));
		table->setCellWidget(row, 1, editor);
		next_start = next_start.addDays(1);
	}
	updateStageStartTimesTableHeight();
}

void EventDialogWidget::updateStageStartTimesTableHeight()
{
	QTableWidget *table = ui->stageStartTimesTable;
	table->resizeRowsToContents();

	const int visible_row_count = qMin(table->rowCount(), 5);
	int height = table->horizontalHeader()->height() + 2 * table->frameWidth();
	for(int row = 0; row < visible_row_count; ++row)
		height += table->rowHeight(row);
	table->setFixedHeight(height);
}

void EventDialogWidget::loadParams(const Event::EventConfigData &params)
{
	m_data = params;
	ui->ed_name->setText(params.name);
	ui->ed_date->setDate(params.date.isValid() ? params.date : QDate::currentDate());
	if(params.time.isValid())
		ui->ed_time->setTime(params.time);

	ui->stageStartTimesTable->setRowCount(0);
	ui->ed_stageCount->setValue(params.stageCount);
	updateStageStartTimeEditors(ui->ed_stageCount->value());
	for(int row = 0; row < ui->stageStartTimesTable->rowCount(); ++row) {
		const QDateTime start_time = params.stages.value(row + 1).startDateTime;
		if(start_time.isValid()) {
			if(auto *editor = qobject_cast<QDateTimeEdit *>(ui->stageStartTimesTable->cellWidget(row, 1)))
				editor->setDateTime(start_time);
		}
	}
	ui->ed_description->setText(params.description);
	ui->ed_place->setText(params.place);
	ui->ed_mainReferee->setText(params.mainReferee);
	ui->ed_director->setText(params.director);
	ui->ed_handicapLength->setValue(params.handicapLength);
	if(auto ix = ui->cbxSportId->findData(params.sportId); ix < 0)
		ui->cbxSportId->setCurrentIndex(0);
	else
		ui->cbxSportId->setCurrentIndex(ix);
	if(auto ix = ui->cbxDisciplineId->findData(params.disciplineId); ix < 0)
		ui->cbxDisciplineId->setCurrentIndex(0);
	else
		ui->cbxDisciplineId->setCurrentIndex(ix);
	ui->ed_orisImportId->setText(params.importId > 0 ? QString::number(params.importId) : QString());
	ui->ed_orisRace->setChecked(params.importId > 0);
	ui->ed_orisEventKey->setText(params.orisEventKey);
	ui->ed_cardChecCheckTimeSec->setValue(params.cardCheckTimeSec);
	ui->ed_oneTenthSecResults->setCurrentIndex(params.oneTenthSecResults);
	ui->ed_iofRace->setChecked(params.iofRace);
	ui->ed_xmlRaceNumber->setValue(params.iofXmlRaceNumber);
}

Event::EventConfigData EventDialogWidget::saveParams()
{
	Event::EventConfigData data = m_data;
	data.stageCount = ui->ed_stageCount->value();
	for(int row = 0; row < ui->stageStartTimesTable->rowCount(); ++row) {
		if(auto *editor = qobject_cast<QDateTimeEdit *>(ui->stageStartTimesTable->cellWidget(row, 1)))
			data.stages[row + 1].startDateTime = editor->dateTime();
	}
	data.name = ui->ed_name->text();
	data.date = ui->ed_date->date();
	data.time = ui->ed_time->time();
	data.description = ui->ed_description->text();
	data.place = ui->ed_place->text();
	data.mainReferee = ui->ed_mainReferee->text();
	data.director = ui->ed_director->text();
	data.handicapLength = ui->ed_handicapLength->value();
	data.sportId = ui->cbxSportId->currentData().isNull()
		? static_cast<int>(Event::EventConfig::Sport::OB)
		: ui->cbxSportId->currentData().toInt();
	data.disciplineId = ui->cbxDisciplineId->currentIndex() <= 0
		? static_cast<int>(Event::EventConfig::Discipline::LongDistance)
		: ui->cbxDisciplineId->currentData().toInt();
	data.importId = ui->ed_orisImportId->text().toInt();
	data.orisEventKey = ui->ed_orisEventKey->text();
	data.cardCheckTimeSec = ui->ed_cardChecCheckTimeSec->value();
	data.oneTenthSecResults = ui->ed_oneTenthSecResults->currentIndex();
	data.iofRace = ui->ed_iofRace->isChecked();
	data.iofXmlRaceNumber = ui->ed_xmlRaceNumber->value();
	return data;
}

QString EventDialogWidget::disciplineName(int disc_id)
{
	using D = Event::EventConfig::Discipline;
	switch (static_cast<D>(disc_id)) {
	case D::LongDistance:      return tr("Long distance");
	case D::ShortDistance:     return tr("Middle distance");
	case D::UltralongDistance: return tr("Ultralong distance");
	case D::Sprint:            return tr("Sprint");
	case D::Relays:            return tr("Relays");
	case D::Teams:             return tr("Teams");
	case D::FreeOrder:         return tr("Free order");
	case D::NightRace:         return tr("Night");
	case D::SprintRelays:      return tr("Sprint relays");
	case D::KnocOutSprint:     return tr("Knock-out sprint");
	case D::TempO:             return tr("TempO");
	case D::MultiStages:       return tr("Multi stages");
	case D::MassStart:         return tr("Mass start");
	case D::Indoor:            return tr("Indoor");
	}
	return {};
}

QString EventDialogWidget::sportName(int sport_id)
{
	using S = Event::EventConfig::Sport;
	switch (static_cast<S>(sport_id)) {
	case S::OB:    return QStringLiteral("OB");
	case S::LOB:   return QStringLiteral("LOB");
	case S::MTBO:  return QStringLiteral("MTBO");
	case S::TRAIL: return QStringLiteral("TRAIL");
	}
	return {};
}
