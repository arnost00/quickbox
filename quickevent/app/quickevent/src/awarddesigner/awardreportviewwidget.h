#pragma once
#include "awarddesign.h"

#include <QDialog>
#include <QList>
#include <QSharedPointer>
#include <QVariantMap>

class QPrinter;
class QSvgRenderer;

// Preview/print dialog for award documents rendered natively by Typst.
// Pages are shown and printed as vector SVG; PDF export uses Typst's own PDF output.
class AwardReportViewWidget : public QDialog
{
	Q_OBJECT
	using Super = QDialog;
public:
	explicit AwardReportViewWidget(const AwardDesigner::Design &design,
		const QList<QVariantMap> &pages, QWidget *parent = nullptr);
	~AwardReportViewWidget() override;

	// True when at least one page was rendered successfully.
	bool hasPages() const { return !m_renderers.isEmpty(); }

private:
	void print(QPrinter &printer);
	void onPrint();
	void onPrintPreview();
	void onExportPdf();

	AwardDesigner::Design m_design;
	QList<QVariantMap> m_pages;
	QList<QSharedPointer<QSvgRenderer>> m_renderers;
};
