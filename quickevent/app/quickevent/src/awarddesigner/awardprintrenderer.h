#pragma once
#include "awarddesign.h"

#include <QImage>
#include <QList>
#include <QVariantMap>

namespace qf::core::utils { class TreeTable; }
namespace Event { class EventConfig; }
class QPainter;
class QPrinter;

class AwardPrintRenderer
{
public:
	explicit AwardPrintRenderer(const AwardDesigner::Design &design);

	// Collect per-relay data maps from the relay results TreeTable
	QList<QVariantMap> collectPages(const qf::core::utils::TreeTable &tt,
		Event::EventConfig *eventConfig) const;

	// Collect per-runner data maps from the stage results TreeTable
	QList<QVariantMap> collectRunsPages(const qf::core::utils::TreeTable &tt,
		Event::EventConfig *eventConfig) const;

	// Render pages to QImage list at given DPI
	QList<QImage> renderToImages(const QList<QVariantMap> &pages, int dpi = 96) const;

	// Render all pages to an already-configured QPrinter (for print/PDF)
	void renderToPrinter(QPrinter &printer, const QList<QVariantMap> &pages) const;

private:
	AwardDesigner::Design m_design;

	void renderPage(QPainter &painter, const QVariantMap &data,
		const QRectF &pageRect, qreal mmToPx) const;
	void renderItem(QPainter &painter, const AwardDesigner::Item &item,
		const QVariantMap &data, const QRectF &pageRect, qreal mmToPx) const;
	static QString resolveField(const QString &fieldId, const QVariantMap &data);
};
