#include "awardqmlrenderer.h"
#include "awardtypstrenderer.h"

#include <qf/core/log.h>

#include <QBuffer>

AwardQmlRenderer::AwardQmlRenderer(const AwardDesigner::Design &design,
	const QList<QVariantMap> &pages,
	QObject *parent)
	: QObject(parent)
	, m_design(design)
{
	for (const auto &data : pages) {
		const QString cls = data.value(QStringLiteral("category")).toString();
		const int pos = data.value(QStringLiteral("pos")).toInt();
		m_pageMap.insert(pageKey(cls, pos), data);

		if (data.contains(QStringLiteral("_classIdx"))) {
			const int ci = data.value(QStringLiteral("_classIdx")).toInt();
			const int ri = data.value(QStringLiteral("_runnerIdx")).toInt();
			m_runPageMap.insert(runPageKey(ci, ri), data);
		}
	}
}

QString AwardQmlRenderer::renderPageBase64(const QString &class_name, int pos)
{
	const QString key = pageKey(class_name, pos);
	if (!m_pageMap.contains(key)) {
		qfWarning() << "Award page not found for key:" << key;
		return {};
	}

	AwardTypstRenderer renderer(m_design);
	auto images = renderer.renderToImages({m_pageMap.value(key)}, 200);
	if (images.isEmpty())
		return {};

	QByteArray png;
	QBuffer buf(&png);
	buf.open(QIODevice::WriteOnly);
	images.first().save(&buf, "PNG");
	return QString::fromLatin1(png.toBase64());
}

QString AwardQmlRenderer::renderRunPageBase64(int class_idx, int runner_idx)
{
	const QString key = runPageKey(class_idx, runner_idx);
	if (!m_runPageMap.contains(key))
		return {};
	const QVariantMap &data = m_runPageMap.value(key);
	if (data.value(QStringLiteral("pos")).toInt() <= 0)
		return {};

	AwardTypstRenderer renderer(m_design);
	auto images = renderer.renderToImages({data}, 200);
	if (images.isEmpty())
		return {};

	QByteArray png;
	QBuffer buf(&png);
	buf.open(QIODevice::WriteOnly);
	images.first().save(&buf, "PNG");
	return QString::fromLatin1(png.toBase64());
}

QString AwardQmlRenderer::pageKey(const QString &class_name, int pos)
{
	return class_name + QLatin1Char('|') + QString::number(pos);
}

QString AwardQmlRenderer::runPageKey(int ci, int ri)
{
	return QString::number(ci) + QLatin1Char('|') + QString::number(ri);
}
