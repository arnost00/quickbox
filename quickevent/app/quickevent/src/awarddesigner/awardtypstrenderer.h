#pragma once
#include "awarddesign.h"

#include <QByteArray>
#include <QHash>
#include <QImage>
#include <QList>
#include <QVariantMap>

class QTemporaryDir;

namespace qf::core::utils { class TreeTable; }
namespace Event { class EventConfig; }

class AwardTypstRenderer
{
public:
	explicit AwardTypstRenderer(const AwardDesigner::Design &design);

	// Collect per-relay data maps from the relay results TreeTable
	QList<QVariantMap> collectPages(const qf::core::utils::TreeTable &tt,
		Event::EventConfig *event_config) const;

	// Collect per-runner data maps from the stage results TreeTable
	QList<QVariantMap> collectRunsPages(const qf::core::utils::TreeTable &tt,
		Event::EventConfig *event_config) const;

	// Render pages to a QImage per page, via a generated Typst document rendered at the given PPI
	QList<QImage> renderToImages(const QList<QVariantMap> &pages, int dpi = 96) const;

	// Render the whole document to a single PDF file; returns the file path, empty on failure.
	// The file lives in a temporary directory whose lifetime is tied to the given holder.
	QString renderToPdf(const QList<QVariantMap> &pages, QTemporaryDir &out_dir) const;

	// Render the whole document to one SVG document per page; empty list on failure.
	QList<QByteArray> renderToSvgs(const QList<QVariantMap> &pages) const;

private:
	AwardDesigner::Design m_design;

	// Compile the document for all pages into out_dir/out-{p}.<ext>. Returns true on success.
	bool compile(const QList<QVariantMap> &pages, const QString &format,
		const QString &out_pattern, QTemporaryDir &out_dir, int dpi = 0) const;

	QHash<QString, QString> copyImagesToDir(const QString &dir_path) const;
	QString buildTypstSource(const QList<AwardDesigner::Item> &sorted_items,
		const QHash<QString, QString> &image_file_names) const;
	QString itemToTypstSnippet(const AwardDesigner::Item &item, int index,
		const QHash<QString, QString> &image_file_names) const;

	static QString escapeTypstString(const QString &s);
	static QString typstAlignment(int halign);
};
