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

	// Called from QML dataFn for Relays — key is "className|pos"
	Q_INVOKABLE QString renderPageBase64(const QString &class_name, int pos);
	// Called from QML dataFn for Runs — key is "classIdx|runnerIdx" (Detail.currentIndex)
	Q_INVOKABLE QString renderRunPageBase64(int class_idx, int runner_idx);

private:
	AwardDesigner::Design m_design;
	QMap<QString, QVariantMap> m_pageMap; // "className|pos" → data (Relays)
	QMap<QString, QVariantMap> m_runPageMap; // "ci|ri" → data (Runs)

	static QString pageKey(const QString &class_name, int pos);
	static QString runPageKey(int ci, int ri);
};
