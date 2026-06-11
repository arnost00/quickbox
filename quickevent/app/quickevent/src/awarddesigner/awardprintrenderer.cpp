#include "awardprintrenderer.h"

#include <plugins/Event/src/eventconfig.h>

#include <qf/core/utils/treetable.h>
#include <qf/core/log.h>

#include <QDate>
#include <QFont>
#include <QPainter>
#include <QPrinter>

AwardPrintRenderer::AwardPrintRenderer(const AwardDesigner::Design &design)
	: m_design(design)
{
}

QList<QVariantMap> AwardPrintRenderer::collectPages(const qf::core::utils::TreeTable &tt,
	Event::EventConfig *eventConfig) const
{
	QList<QVariantMap> pages;

	QVariantMap eventMap = tt.value(QStringLiteral("event")).toMap();
	QString eventName  = eventMap.value(QStringLiteral("name")).toString();
	QString eventPlace = eventMap.value(QStringLiteral("place")).toString();

	QString dateStr;
	{
		QDateTime dt;
		QVariant stageStart = tt.value(QStringLiteral("stageStart"));
		if (stageStart.isValid())
			dt = stageStart.toDateTime();
		if (!dt.isValid()) {
			QVariant evDt = eventMap.value(QStringLiteral("dateTime"));
			if (!evDt.isValid())
				evDt = eventMap.value(QStringLiteral("date"));
			dt = evDt.toDateTime();
			if (!dt.isValid())
				dt = QDateTime(evDt.toDate(), QTime());
		}
		if (dt.isValid())
			dateStr = dt.toString(QStringLiteral("dd.MM.yyyy"));
	}

	QString mainReferee;
	QString director;
	if (eventConfig) {
		mainReferee = eventConfig->mainReferee();
		director    = eventConfig->director();
	}

	for (int ci = 0; ci < tt.rowCount(); ++ci) {
		auto classRow = tt.row(ci);
		QString className = classRow.value(QStringLiteral("className")).toString();
		auto relayTable = classRow.table(0);

		for (int ri = 0; ri < relayTable.rowCount(); ++ri) {
			auto relayRow = relayTable.row(ri);
			int pos = relayRow.value(QStringLiteral("pos")).toInt();
			QString orgName = relayRow.value(QStringLiteral("orgName")).toString();
			if (orgName.isEmpty())
				orgName = relayRow.value(QStringLiteral("name")).toString();

			QStringList runnerNames;
			auto runnerTable = relayRow.table(0);
			for (int ki = 0; ki < runnerTable.rowCount(); ++ki) {
				auto runnerRow = runnerTable.row(ki);
				QString name = runnerRow.value(QStringLiteral("competitorName")).toString();
				if (!name.isEmpty())
					runnerNames << name;
			}

			const QString posStr = (pos > 0)
				? QString::number(pos) + QStringLiteral(". místo")
				: QStringLiteral("místo");

			QVariantMap data;
			data[QStringLiteral("pos")]              = pos;   // raw integer for keying
			data[QStringLiteral("eventName")]       = eventName;
			data[QStringLiteral("date")]             = dateStr;
			data[QStringLiteral("place")]            = eventPlace;
			data[QStringLiteral("position")]         = posStr;
			data[QStringLiteral("category")]         = className;
			data[QStringLiteral("positionCategory")] = posStr + QStringLiteral(" v kategorii ") + className;
			data[QStringLiteral("clubName")]         = orgName;
			data[QStringLiteral("runners")]          = runnerNames.join(QStringLiteral("\n"));
			data[QStringLiteral("mainReferee")]      = mainReferee;
			data[QStringLiteral("director")]         = director;
			pages.append(data);
		}
	}
	return pages;
}

QList<QVariantMap> AwardPrintRenderer::collectRunsPages(const qf::core::utils::TreeTable &tt,
	Event::EventConfig *eventConfig) const
{
	QList<QVariantMap> pages;

	QVariantMap eventMap = tt.value(QStringLiteral("event")).toMap();
	QString eventName  = eventMap.value(QStringLiteral("name")).toString();
	QString eventPlace = eventMap.value(QStringLiteral("place")).toString();

	QString dateStr;
	{
		QDateTime dt;
		QVariant stageStart = tt.value(QStringLiteral("stageStart"));
		if (stageStart.isValid())
			dt = stageStart.toDateTime();
		if (!dt.isValid()) {
			QVariant evDt = eventMap.value(QStringLiteral("dateTime"));
			if (!evDt.isValid())
				evDt = eventMap.value(QStringLiteral("date"));
			dt = evDt.toDateTime();
			if (!dt.isValid())
				dt = QDateTime(evDt.toDate(), QTime());
		}
		if (dt.isValid())
			dateStr = dt.toString(QStringLiteral("dd.MM.yyyy"));
	}

	QString mainReferee;
	QString director;
	if (eventConfig) {
		mainReferee = eventConfig->mainReferee();
		director    = eventConfig->director();
	}

	for (int ci = 0; ci < tt.rowCount(); ++ci) {
		auto classRow = tt.row(ci);
		QString className = classRow.value(QStringLiteral("name")).toString();
		auto runnerTable = classRow.table(0);

		for (int ri = 0; ri < runnerTable.rowCount(); ++ri) {
			auto runnerRow = runnerTable.row(ri);

			// stageResultsTable has integer "npos"; nstagesResultsTable has text "pos" like "1."
			int pos = runnerRow.value(QStringLiteral("npos")).toInt();
			if (pos <= 0) {
				QString posText = runnerRow.value(QStringLiteral("pos")).toString();
				if (posText.endsWith(QLatin1Char('.')))
					pos = posText.chopped(1).toInt();
			}

			QString competitorName = runnerRow.value(QStringLiteral("competitorName")).toString();
			QString orgName = runnerRow.value(QStringLiteral("clubs.name")).toString();
			if (orgName.isEmpty())
				orgName = runnerRow.value(QStringLiteral("club")).toString();

			const QString posStr = (pos > 0)
				? QString::number(pos) + QStringLiteral(". místo")
				: QString();

			QVariantMap data;
			data[QStringLiteral("_classIdx")]        = ci;
			data[QStringLiteral("_runnerIdx")]       = ri;
			data[QStringLiteral("pos")]              = pos;
			data[QStringLiteral("eventName")]        = eventName;
			data[QStringLiteral("date")]             = dateStr;
			data[QStringLiteral("place")]            = eventPlace;
			data[QStringLiteral("position")]         = posStr;
			data[QStringLiteral("category")]         = className;
			data[QStringLiteral("positionCategory")] = posStr.isEmpty()
				? QStringLiteral("v kategorii ") + className
				: posStr + QStringLiteral(" v kategorii ") + className;
			data[QStringLiteral("competitorName")]   = competitorName;
			data[QStringLiteral("clubName")]         = orgName;
			data[QStringLiteral("mainReferee")]      = mainReferee;
			data[QStringLiteral("director")]         = director;
			pages.append(data);
		}
	}
	return pages;
}

QList<QImage> AwardPrintRenderer::renderToImages(const QList<QVariantMap> &pages, int dpi) const
{
	const qreal mmToPx = dpi / 25.4;
	const int pw = qRound(m_design.pageW * mmToPx);
	const int ph = qRound(m_design.pageH * mmToPx);

	QList<QImage> images;
	images.reserve(pages.size());

	for (const auto &data : pages) {
		QImage img(pw, ph, QImage::Format_RGB32);
		img.fill(Qt::white);
		img.setDotsPerMeterX(qRound(dpi / 25.4 * 1000.0));
		img.setDotsPerMeterY(qRound(dpi / 25.4 * 1000.0));

		QPainter painter(&img);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		renderPage(painter, data, QRectF(0, 0, pw, ph), mmToPx);
		painter.end();
		images.append(std::move(img));
	}
	return images;
}

void AwardPrintRenderer::renderToPrinter(QPrinter &printer, const QList<QVariantMap> &pages) const
{
	QPainter painter(&printer);
	const QRectF pageRect = printer.pageLayout().paintRectPixels(printer.resolution());
	const qreal mmToPx = printer.resolution() / 25.4;

	for (int i = 0; i < pages.count(); ++i) {
		if (i > 0)
			printer.newPage();
		renderPage(painter, pages.at(i), pageRect, mmToPx);
	}
	painter.end();
}

void AwardPrintRenderer::renderPage(QPainter &painter, const QVariantMap &data,
	const QRectF &pageRect, qreal mmToPx) const
{
	QList<AwardDesigner::Item> sorted = m_design.items;
	std::stable_sort(sorted.begin(), sorted.end(),
		[](const AwardDesigner::Item &a, const AwardDesigner::Item &b) {
			return a.zOrder < b.zOrder;
		});
	for (const auto &item : sorted)
		renderItem(painter, item, data, pageRect, mmToPx);
}

void AwardPrintRenderer::renderItem(QPainter &painter, const AwardDesigner::Item &item,
	const QVariantMap &data, const QRectF &pageRect, qreal mmToPx) const
{
	const qreal x = pageRect.left() + item.x * mmToPx;
	const qreal y = pageRect.top()  + item.y * mmToPx;
	const qreal w = item.w * mmToPx;
	const qreal h = item.h * mmToPx;
	const QRectF r(x, y, w, h);

	if (item.kind == AwardDesigner::Item::Image) {
		if (!item.imagePath.isEmpty()) {
			QPixmap pm;
			if (pm.load(item.imagePath)) {
				if (item.scaleProportional)
					painter.drawPixmap(r.toRect(),
						pm.scaled(r.toRect().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
				else
					painter.drawPixmap(r.toRect(), pm);
			}
		}
	} else {
		QString text;
		if (item.fieldId == QLatin1String("customText"))
			text = item.customText;
		else
			text = resolveField(item.fieldId, data);
		if (text.isEmpty())
			return;

		QFont font(item.fontFamily, -1);
		font.setPixelSize(qRound(item.fontSize * mmToPx * 25.4 / 72.0));
		font.setBold(item.bold);
		font.setItalic(item.italic);
		painter.setFont(font);
		painter.setPen(QColor(item.color));

		Qt::Alignment align = static_cast<Qt::Alignment>(item.halign) | Qt::AlignVCenter;
		painter.drawText(r, align | Qt::TextWordWrap, text);
	}
}

QString AwardPrintRenderer::resolveField(const QString &fieldId, const QVariantMap &data)
{
	return data.value(fieldId).toString();
}
