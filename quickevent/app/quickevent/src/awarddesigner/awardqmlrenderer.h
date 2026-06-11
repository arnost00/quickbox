#pragma once
#include "awarddesign.h"

#include <QObject>
#include <QVariantMap>

class AwardQmlRenderer : public QObject
{
	Q_OBJECT
public:
	explicit AwardQmlRenderer(const AwardDesigner::Design &design,
		const QList<QVariantMap> &pages,
		QObject *parent = nullptr);

	// Called from QML dataFn — key identifies the relay page
	Q_INVOKABLE QString renderPageBase64(const QString &className, int pos);

private:
	AwardDesigner::Design m_design;
	QMap<QString, QVariantMap> m_pageMap; // "className|pos" → data

	static QString pageKey(const QString &className, int pos);
};
