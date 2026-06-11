#include "awardqmlrenderer.h"
#include "awardprintrenderer.h"

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
		const int     pos = data.value(QStringLiteral("pos")).toInt();
		m_pageMap.insert(pageKey(cls, pos), data);
	}
}

QString AwardQmlRenderer::renderPageBase64(const QString &className, int pos)
{
	const QString key = pageKey(className, pos);
	if (!m_pageMap.contains(key)) {
		qfWarning() << "Award page not found for key:" << key;
		return {};
	}

	AwardPrintRenderer renderer(m_design);
	auto images = renderer.renderToImages({m_pageMap.value(key)}, 200);
	if (images.isEmpty())
		return {};

	QByteArray png;
	QBuffer buf(&png);
	buf.open(QIODevice::WriteOnly);
	images.first().save(&buf, "PNG");
	return QString::fromLatin1(png.toBase64());
}

QString AwardQmlRenderer::pageKey(const QString &className, int pos)
{
	return className + QLatin1Char('|') + QString::number(pos);
}
