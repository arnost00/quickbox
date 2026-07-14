#include "awardreportviewwidget.h"
#include "awardtypstrenderer.h"

#include <qf/gui/dialogs/filedialog.h>
#include <qf/core/log.h>

#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPageSize>
#include <QPainter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QScrollArea>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// On-screen preview width, in device-independent pixels.
constexpr qreal PREVIEW_WIDTH_PX = 780.0;

void configurePrinter(QPrinter &printer, const AwardDesigner::Design &design)
{
	printer.setPageSize(QPageSize(QSizeF(design.pageW, design.pageH), QPageSize::Millimeter));
	printer.setFullPage(true);
	printer.setPageMargins(QMarginsF(0, 0, 0, 0));
}
}

AwardReportViewWidget::AwardReportViewWidget(const AwardDesigner::Design &design,
	const QList<QVariantMap> &pages, QWidget *parent)
	: Super(parent)
	, m_design(design)
	, m_pages(pages)
{
	setWindowTitle(tr("Awards"));
	resize(900, 700);

	AwardTypstRenderer renderer(m_design);
	const QList<QByteArray> svgs = renderer.renderToSvgs(m_pages);
	for (const QByteArray &svg : svgs) {
		auto r = QSharedPointer<QSvgRenderer>::create();
		if (r->load(svg))
			m_renderers.append(r);
		else
			qfWarning() << "Cannot load a rendered award SVG page";
	}

	auto *layout = new QVBoxLayout(this);

	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setBackgroundRole(QPalette::Mid);
	auto *pages_container = new QWidget(scroll);
	auto *pages_layout = new QVBoxLayout(pages_container);
	pages_layout->setSpacing(12);

	const qreal scale = PREVIEW_WIDTH_PX / m_design.pageW; // px per mm
	const int w = qRound(m_design.pageW * scale);
	const int h = qRound(m_design.pageH * scale);
	for (const auto &r : m_renderers) {
		QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
		img.fill(Qt::white);
		QPainter p(&img);
		r->render(&p, QRectF(0, 0, w, h));
		p.end();

		auto *label = new QLabel(pages_container);
		label->setPixmap(QPixmap::fromImage(img));
		label->setFixedSize(w, h);
		pages_layout->addWidget(label, 0, Qt::AlignHCenter);
	}
	pages_layout->addStretch(1);
	scroll->setWidget(pages_container);
	layout->addWidget(scroll, 1);

	auto *buttons = new QDialogButtonBox(this);
	auto *bt_print = buttons->addButton(tr("&Print"), QDialogButtonBox::ActionRole);
	auto *bt_preview = buttons->addButton(tr("Print pre&view"), QDialogButtonBox::ActionRole);
	auto *bt_pdf = buttons->addButton(tr("Export P&DF"), QDialogButtonBox::ActionRole);
	buttons->addButton(QDialogButtonBox::Close);
	layout->addWidget(buttons);

	connect(bt_print, &QPushButton::clicked, this, &AwardReportViewWidget::onPrint);
	connect(bt_preview, &QPushButton::clicked, this, &AwardReportViewWidget::onPrintPreview);
	connect(bt_pdf, &QPushButton::clicked, this, &AwardReportViewWidget::onExportPdf);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	const bool has_pages = hasPages();
	bt_print->setEnabled(has_pages);
	bt_preview->setEnabled(has_pages);
	bt_pdf->setEnabled(has_pages);
}

AwardReportViewWidget::~AwardReportViewWidget() = default;

void AwardReportViewWidget::print(QPrinter &printer)
{
	QPainter painter;
	if (!painter.begin(&printer)) {
		qfWarning() << "Cannot start printing the award document";
		return;
	}
	const QRectF target = printer.pageRect(QPrinter::DevicePixel);
	for (int i = 0; i < m_renderers.size(); ++i) {
		if (i > 0)
			printer.newPage();
		m_renderers.at(i)->render(&painter, target);
	}
	painter.end();
}

void AwardReportViewWidget::onPrint()
{
	QPrinter printer(QPrinter::HighResolution);
	printer.setOutputFormat(QPrinter::NativeFormat);
	configurePrinter(printer, m_design);

	QPrintDialog dlg(&printer, this);
	if (dlg.exec() != QDialog::Accepted)
		return;
	print(printer);
}

void AwardReportViewWidget::onPrintPreview()
{
	QPrinter printer(QPrinter::HighResolution);
	printer.setOutputFormat(QPrinter::NativeFormat);
	configurePrinter(printer, m_design);

	QPrintPreviewDialog preview(&printer, this);
	connect(&preview, &QPrintPreviewDialog::paintRequested, this, [this](QPrinter *p) { print(*p); });
	preview.exec();
}

void AwardReportViewWidget::onExportPdf()
{
	QString fn = qf::gui::dialogs::FileDialog::getSaveFileName(this,
		tr("Export PDF"), QString(), QStringLiteral("*.pdf"));
	if (fn.isEmpty())
		return;
	if (!fn.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
		fn += QStringLiteral(".pdf");

	// Use Typst's native PDF output for best (vector) quality.
	AwardTypstRenderer renderer(m_design);
	QTemporaryDir temp_dir;
	const QString pdf_path = renderer.renderToPdf(m_pages, temp_dir);
	if (pdf_path.isEmpty()) {
		qfWarning() << "Cannot render the award PDF document";
		return;
	}
	QFile::remove(fn);
	if (!QFile::copy(pdf_path, fn))
		qfWarning() << "Cannot write the award PDF to" << fn;
}
