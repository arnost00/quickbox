#ifndef EVENTDIALOGWIDGET_H
#define EVENTDIALOGWIDGET_H

#include "stage.h"

#include <qf/gui/framework/dialogwidget.h>

#include <QDate>
#include <QMap>
#include <QString>
#include <QTime>
#include <QVariantMap>

namespace Event {

struct EventConfigData
{
	static EventConfigData fromVariantMap(const QVariantMap &values);
	QVariantMap toVariantMap() const;

	int stageCount = 1;
	QMap<int, StageData> stages;
	QString name;
	QDate date;
	QTime time;
	QString description;
	QString place;
	QString mainReferee;
	QString director;
	int handicapLength = 0;
	int sportId = 0;
	int disciplineId = 0;
	int importId = 0;
	QString orisEventKey;
	int cardCheckTimeSec = 0;
	int oneTenthSecResults = 0;
	bool iofRace = false;
	int iofXmlRaceNumber = 0;
	int currentStageId = 1;
};

}

namespace Ui {
class EventDialogWidget;
}

class EventDialogWidget : public qf::gui::framework::DialogWidget
{
	Q_OBJECT
private:
	typedef qf::gui::framework::DialogWidget Super;
public:
	explicit EventDialogWidget(QWidget *parent = nullptr);
	~EventDialogWidget() Q_DECL_OVERRIDE;

	void setEventId(const QString &event_id);
	QString eventId() const;
	void setEventIdEditable(bool b);

	void loadParams(const Event::EventConfigData &params);
	Event::EventConfigData saveParams();

	static QString disciplineName(int disc_id);
	static QString sportName(int sport_id);
private:
	void updateStageStartTimeEditors(int stage_count);
	void updateStageStartTimesTableHeight();

	Event::EventConfigData m_data;
	Ui::EventDialogWidget *ui;
};

#endif // EVENTDIALOGWIDGET_H
