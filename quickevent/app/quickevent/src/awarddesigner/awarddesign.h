#pragma once
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>
#include <Qt>

namespace AwardDesigner {

struct FieldDef {
	QString id;
	QString label;
};

QList<FieldDef> relayFields();
QList<FieldDef> runsFields();

struct Item {
	enum Kind { Image = 0, Field = 1 };
	Kind kind = Field;

	// Geometry in mm
	qreal x = 20, y = 20, w = 170, h = 15;
	int zOrder = 0;

	// Image
	QString imagePath;

	// Field
	QString fieldId = QStringLiteral("eventName");
	QString customText;

	// Text formatting (used for Field items)
	QString fontFamily = QStringLiteral("Arial");
	int fontSize = 14;
	bool bold = false;
	bool italic = false;
	QString color = QStringLiteral("#000000");
	int halign = Qt::AlignHCenter; // stored as int

	// Image sizing
	bool scaleProportional = true;

	QJsonObject toJson() const;
	static Item fromJson(const QJsonObject &obj);
};

struct Design {
	QString name;
	QString type; // "relay" or "runs"; empty = treat as "relay" (backward compat)
	qreal pageW = 210;
	qreal pageH = 297;
	QList<Item> items;

	QJsonObject toJson() const;
	static Design fromJson(const QJsonObject &obj);

	bool saveToDb() const;
	static Design loadFromDb(const QString &name);
	// type filter: "relay", "runs", or QString() for all
	static QStringList listFromDb(const QString &type = QString());
	static bool deleteFromDb(const QString &name);

	static QString dbKey(const QString &name)
	{
		return QLatin1String("awards.design.") + name;
	}

	bool isValid() const { return !name.isEmpty(); }

	static Design defaultRelayDesign();
	static Design defaultRunsDesign();
};

} // namespace AwardDesigner
