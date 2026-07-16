#pragma once

#include "stagedata.h"

#include <QDate>
#include <QMap>
#include <QString>
#include <QTime>
#include <QVariantMap>

namespace Event {

struct EventConfig
{
	static EventConfig fromVariantMap(const QVariantMap &values);
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
