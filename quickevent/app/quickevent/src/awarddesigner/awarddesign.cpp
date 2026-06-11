#include "awarddesign.h"

#include <qf/core/sql/query.h>
#include <qf/core/log.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>

namespace AwardDesigner {

#define TR(s) QCoreApplication::translate("AwardDesigner", s)

QList<FieldDef> relayFields()
{
	return {
		{QStringLiteral("eventName"),        TR("Název závodu")},
		{QStringLiteral("date"),             TR("Datum")},
		{QStringLiteral("place"),            TR("Místo konání")},
		{QStringLiteral("positionCategory"), TR("Pořadí v kategorii")},
		{QStringLiteral("position"),         TR("Pořadí")},
		{QStringLiteral("category"),         TR("Kategorie")},
		{QStringLiteral("clubName"),         TR("Název štafety/klubu")},
		{QStringLiteral("runners"),          TR("Závodníci (seznam)")},
		{QStringLiteral("mainReferee"),      TR("Hlavní rozhodčí")},
		{QStringLiteral("director"),         TR("Ředitel závodu")},
		{QStringLiteral("customText"),       TR("Vlastní text")},
	};
}

QList<FieldDef> runsFields()
{
	return {
		{QStringLiteral("eventName"),        TR("Název závodu")},
		{QStringLiteral("date"),             TR("Datum")},
		{QStringLiteral("place"),            TR("Místo konání")},
		{QStringLiteral("positionCategory"), TR("Pořadí v kategorii")},
		{QStringLiteral("position"),         TR("Pořadí")},
		{QStringLiteral("category"),         TR("Kategorie")},
		{QStringLiteral("competitorName"),   TR("Jméno závodníka")},
		{QStringLiteral("clubName"),         TR("Klub")},
		{QStringLiteral("mainReferee"),      TR("Hlavní rozhodčí")},
		{QStringLiteral("director"),         TR("Ředitel závodu")},
		{QStringLiteral("customText"),       TR("Vlastní text")},
	};
}

static Item makeFieldItem(const QString &fieldId, qreal x, qreal y, qreal w, qreal h,
	const QString &fontFamily, int fontSize, bool bold,
	const QString &color = QStringLiteral("#000000"),
	int halign = Qt::AlignHCenter)
{
	Item it;
	it.kind = Item::Field;
	it.fieldId = fieldId;
	it.x = x; it.y = y; it.w = w; it.h = h;
	it.fontFamily = fontFamily; it.fontSize = fontSize; it.bold = bold;
	it.color = color; it.halign = halign;
	return it;
}

Design Design::defaultRelayDesign()
{
	Design d;
	d.type = QStringLiteral("relay");
	d.pageW = 210; d.pageH = 297;

	// Event name — large bold
	d.items << makeFieldItem(QStringLiteral("eventName"),
		15, 15, 180, 14, QStringLiteral("Arial"), 16, true);

	// Date — small red
	d.items << makeFieldItem(QStringLiteral("date"),
		15, 31, 180, 8, QStringLiteral("Arial"), 10, false, QStringLiteral("#cc0000"));

	// "Diplom" heading — huge Times Serif maroon
	Item diplom;
	diplom.kind = Item::Field;
	diplom.fieldId = QStringLiteral("customText");
	diplom.customText = QStringLiteral("Diplom");
	diplom.x = 15; diplom.y = 52; diplom.w = 180; diplom.h = 38;
	diplom.fontFamily = QStringLiteral("Times New Roman"); diplom.fontSize = 72;
	diplom.color = QStringLiteral("#800000"); diplom.halign = Qt::AlignHCenter;
	d.items << diplom;

	// Position + category combined
	d.items << makeFieldItem(QStringLiteral("positionCategory"),
		15, 100, 180, 12, QStringLiteral("Arial"), 16, true);

	// Club name
	d.items << makeFieldItem(QStringLiteral("clubName"),
		15, 117, 180, 12, QStringLiteral("Arial"), 16, true);

	// Runners list (multi-line, generous height)
	d.items << makeFieldItem(QStringLiteral("runners"),
		15, 133, 180, 50, QStringLiteral("Arial"), 13, false);

	// Main referee signature (left)
	d.items << makeFieldItem(QStringLiteral("mainReferee"),
		15, 265, 80, 8, QStringLiteral("Arial"), 10, false);

	// Director signature (right)
	d.items << makeFieldItem(QStringLiteral("director"),
		115, 265, 80, 8, QStringLiteral("Arial"), 10, false);

	return d;
}

Design Design::defaultRunsDesign()
{
	Design d;
	d.type = QStringLiteral("runs");
	d.pageW = 210; d.pageH = 297;

	d.items << makeFieldItem(QStringLiteral("eventName"),
		15, 15, 180, 14, QStringLiteral("Arial"), 16, true);

	d.items << makeFieldItem(QStringLiteral("date"),
		15, 31, 180, 8, QStringLiteral("Arial"), 10, false, QStringLiteral("#cc0000"));

	Item diplom;
	diplom.kind = Item::Field;
	diplom.fieldId = QStringLiteral("customText");
	diplom.customText = QStringLiteral("Diplom");
	diplom.x = 15; diplom.y = 52; diplom.w = 180; diplom.h = 38;
	diplom.fontFamily = QStringLiteral("Times New Roman"); diplom.fontSize = 72;
	diplom.color = QStringLiteral("#800000"); diplom.halign = Qt::AlignHCenter;
	d.items << diplom;

	d.items << makeFieldItem(QStringLiteral("positionCategory"),
		15, 100, 180, 12, QStringLiteral("Arial"), 16, true);

	d.items << makeFieldItem(QStringLiteral("competitorName"),
		15, 117, 180, 12, QStringLiteral("Arial"), 16, true);

	d.items << makeFieldItem(QStringLiteral("clubName"),
		15, 133, 180, 10, QStringLiteral("Arial"), 13, false);

	d.items << makeFieldItem(QStringLiteral("mainReferee"),
		15, 265, 80, 8, QStringLiteral("Arial"), 10, false);

	d.items << makeFieldItem(QStringLiteral("director"),
		115, 265, 80, 8, QStringLiteral("Arial"), 10, false);

	return d;
}

QJsonObject Item::toJson() const
{
	QJsonObject o;
	o[QStringLiteral("kind")] = kind;
	o[QStringLiteral("x")] = x;
	o[QStringLiteral("y")] = y;
	o[QStringLiteral("w")] = w;
	o[QStringLiteral("h")] = h;
	o[QStringLiteral("zOrder")] = zOrder;
	o[QStringLiteral("imagePath")] = imagePath;
	o[QStringLiteral("fieldId")] = fieldId;
	o[QStringLiteral("customText")] = customText;
	o[QStringLiteral("fontFamily")] = fontFamily;
	o[QStringLiteral("fontSize")] = fontSize;
	o[QStringLiteral("bold")] = bold;
	o[QStringLiteral("italic")] = italic;
	o[QStringLiteral("color")] = color;
	o[QStringLiteral("halign")] = halign;
	o[QStringLiteral("scaleProportional")] = scaleProportional;
	return o;
}

Item Item::fromJson(const QJsonObject &o)
{
	Item it;
	it.kind = static_cast<Kind>(o[QStringLiteral("kind")].toInt());
	it.x = o[QStringLiteral("x")].toDouble(20);
	it.y = o[QStringLiteral("y")].toDouble(20);
	it.w = o[QStringLiteral("w")].toDouble(170);
	it.h = o[QStringLiteral("h")].toDouble(15);
	it.zOrder = o[QStringLiteral("zOrder")].toInt();
	it.imagePath = o[QStringLiteral("imagePath")].toString();
	it.fieldId = o[QStringLiteral("fieldId")].toString(QStringLiteral("eventName"));
	it.customText = o[QStringLiteral("customText")].toString();
	it.fontFamily = o[QStringLiteral("fontFamily")].toString(QStringLiteral("Arial"));
	it.fontSize = o[QStringLiteral("fontSize")].toInt(14);
	it.bold = o[QStringLiteral("bold")].toBool();
	it.italic = o[QStringLiteral("italic")].toBool();
	it.color = o[QStringLiteral("color")].toString(QStringLiteral("#000000"));
	it.halign = o[QStringLiteral("halign")].toInt(Qt::AlignHCenter);
	it.scaleProportional = o[QStringLiteral("scaleProportional")].toBool(true);
	return it;
}

QJsonObject Design::toJson() const
{
	QJsonObject o;
	o[QStringLiteral("name")] = name;
	o[QStringLiteral("type")] = type;
	o[QStringLiteral("pageW")] = pageW;
	o[QStringLiteral("pageH")] = pageH;
	QJsonArray arr;
	for (const auto &item : items)
		arr.append(item.toJson());
	o[QStringLiteral("items")] = arr;
	return o;
}

Design Design::fromJson(const QJsonObject &o)
{
	Design d;
	d.name = o[QStringLiteral("name")].toString();
	// default "relay" for backward compat — old designs had no type field
	d.type = o[QStringLiteral("type")].toString(QStringLiteral("relay"));
	d.pageW = o[QStringLiteral("pageW")].toDouble(210);
	d.pageH = o[QStringLiteral("pageH")].toDouble(297);
	for (const auto &v : o[QStringLiteral("items")].toArray())
		d.items.append(Item::fromJson(v.toObject()));
	return d;
}

bool Design::saveToDb() const
{
	if (name.isEmpty()) {
		qfWarning() << "Design name is empty, cannot save to DB";
		return false;
	}
	QString key = dbKey(name);
	QString json = QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
	qf::core::sql::Query q_up;
	q_up.prepare(QStringLiteral("UPDATE config SET cvalue=:val WHERE ckey=:key"));
	q_up.bindValue(QStringLiteral(":key"), key);
	q_up.bindValue(QStringLiteral(":val"), json);
	q_up.exec();
	if (q_up.numRowsAffected() < 1) {
		qf::core::sql::Query q_ins;
		q_ins.prepare(QStringLiteral("INSERT INTO config(ckey, cname, cvalue, ctype) VALUES(:key, :cname, :val, 'QString')"));
		q_ins.bindValue(QStringLiteral(":key"), key);
		q_ins.bindValue(QStringLiteral(":cname"), QStringLiteral("Award design: ") + name);
		q_ins.bindValue(QStringLiteral(":val"), json);
		q_ins.exec();
	}
	return true;
}

Design Design::loadFromDb(const QString &name)
{
	qf::core::sql::Query q;
	q.prepare(QStringLiteral("SELECT cvalue FROM config WHERE ckey=:key"));
	q.bindValue(QStringLiteral(":key"), dbKey(name));
	q.exec();
	if (q.next()) {
		QJsonParseError err;
		QJsonDocument doc = QJsonDocument::fromJson(q.value(0).toString().toUtf8(), &err);
		if (err.error == QJsonParseError::NoError) {
			Design d = fromJson(doc.object());
			d.name = name;
			return d;
		}
		qfWarning() << "JSON parse error for design" << name << ":" << err.errorString();
	}
	return Design{};
}

QStringList Design::listFromDb(const QString &type)
{
	qf::core::sql::Query q;
	q.exec(QStringLiteral("SELECT ckey, cvalue FROM config WHERE ckey LIKE 'awards.design.%' ORDER BY ckey"));
	QStringList names;
	const int prefixLen = QStringLiteral("awards.design.").length();
	while (q.next()) {
		if (!type.isEmpty()) {
			QJsonParseError err;
			auto doc = QJsonDocument::fromJson(q.value(1).toString().toUtf8(), &err);
			if (err.error != QJsonParseError::NoError)
				continue;
			// default "relay" for backward compat
			QString t = doc.object().value(QStringLiteral("type")).toString(QStringLiteral("relay"));
			if (t != type)
				continue;
		}
		names << q.value(0).toString().mid(prefixLen);
	}
	return names;
}

bool Design::deleteFromDb(const QString &name)
{
	qf::core::sql::Query q;
	q.prepare(QStringLiteral("DELETE FROM config WHERE ckey=:key"));
	q.bindValue(QStringLiteral(":key"), dbKey(name));
	q.exec();
	return q.numRowsAffected() > 0;
}

} // namespace AwardDesigner
