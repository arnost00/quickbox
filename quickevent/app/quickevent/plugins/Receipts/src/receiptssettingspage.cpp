#include "receiptssettingspage.h"
#include "ui_receiptssettingspage.h"
#include "receiptsplugin.h"
#include "receiptssettings.h"
#include "receiptsprinteroptionsdialog.h"
#include <plugins/Event/src/eventplugin.h>

#include <qf/gui/framework/mainwindow.h>
#include <qf/core/log.h>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QTimer>

namespace Receipts {
namespace {
QString storedReceiptImageLabel(const QString &format)
{
	const QString normalized_format = format.trimmed().isEmpty() ? QStringLiteral("PNG") : format.trimmed().toUpper();
	return QObject::tr("Stored image (%1)").arg(normalized_format);
}

QString defaultReceiptQrCodeCaption()
{
	return QStringLiteral("Live Results");
}

bool loadReceiptImagePayload(const QString &file_path, QString &image_base64, QString &image_format, QString &error_message)
{
	QFile file(file_path);
	if(!file.open(QIODevice::ReadOnly)) {
		error_message = QObject::tr("Cannot open image file '%1'.").arg(QDir::toNativeSeparators(file_path));
		return false;
	}

	const QByteArray file_data = file.readAll();
	if(file_data.isEmpty()) {
		error_message = QObject::tr("Selected image file is empty.");
		return false;
	}

	const bool is_svg = file_data.startsWith("<svg") || file_data.contains("<svg");
	if(is_svg) {
		image_base64 = QString::fromLatin1(file_data.toBase64());
		image_format = QStringLiteral("svg");
		return true;
	}

	QBuffer input_buffer;
	input_buffer.setData(file_data);
	input_buffer.open(QIODevice::ReadOnly);
	QImageReader reader(&input_buffer);
	reader.setAutoTransform(true);
	QImage image = reader.read();
	if(image.isNull()) {
		error_message = QObject::tr("Selected file is not a supported image.");
		return false;
	}

	QByteArray png_data;
	QBuffer output_buffer(&png_data);
	if(!output_buffer.open(QIODevice::WriteOnly) || !image.save(&output_buffer, "PNG") || png_data.isEmpty()) {
		error_message = QObject::tr("Cannot prepare selected image for printing.");
		return false;
	}

	image_base64 = QString::fromLatin1(png_data.toBase64());
	image_format = QStringLiteral("png");
	return true;
}
}

ReceiptsSettingsPage::ReceiptsSettingsPage(QWidget *parent)
	: Super(parent)
{
	ui = new Ui::ReceiptsSettingsPage;
	ui->setupUi(this);
	m_caption = tr("Receipts");

	ui->cbxWhenRunnerNotFound->addItem(tr("Error info"), "ErrorInfo");
	ui->cbxWhenRunnerNotFound->addItem(tr("Error info with picture"), "ErrorInfoLong");
	ui->cbxWhenRunnerNotFound->addItem(tr("Receipt without name"), "ReceiptWithoutName");
	ui->cbxWhenRunnerNotFound->setCurrentIndex(0);

	connect(ui->btPrinterOptions, &QAbstractButton::clicked, this, &ReceiptsSettingsPage::onPrinterOptionsClicked);
	connect(ui->chkPrintReceiptQrCode, &QCheckBox::toggled, this, &ReceiptsSettingsPage::updateReceiptMediaControls);
	connect(ui->chkPrintReceiptImage, &QCheckBox::toggled, this, &ReceiptsSettingsPage::updateReceiptMediaControls);
	connect(ui->btSelectReceiptImage, &QAbstractButton::clicked, this, &ReceiptsSettingsPage::onSelectReceiptImageClicked);
	connect(ui->btClearReceiptImage, &QAbstractButton::clicked, this, &ReceiptsSettingsPage::onClearReceiptImageClicked);
	ui->edReceiptQrCodeCaption->setPlaceholderText(defaultReceiptQrCodeCaption());
}

ReceiptsSettingsPage::~ReceiptsSettingsPage()
{
	delete ui;
}

void ReceiptsSettingsPage::load()
{
	loadReceptList();
	ReceiptsSettings settings;
	ui->chkAutoPrint->setChecked(settings.isAutoPrint());
	ui->chkThisReaderOnly->setChecked(settings.isThisReaderOnly());
	{
		auto *cbx = ui->cbxReceipt;
		QString path = settings.receiptPath();
		qfInfo() << "current receipt path:" << path;
		for (int i = 0; i < cbx->count(); ++i) {
			if(cbx->itemData(i).toString() == path) {
				cbx->setCurrentIndex(i);
				break;
			}
		}
		if(cbx->currentIndex() < 0) {
			cbx->setCurrentIndex(0);
			settings.setReceiptPath(cbx->currentData().toString());
		}
	}
	{
		auto *cbx = ui->cbxWhenRunnerNotFound;
		for (int i = 0; i < cbx->count(); ++i) {
			if(cbx->itemData(i).toString() == settings.whenRunnerNotFoundPrint()) {
				cbx->setCurrentIndex(i);
				break;
			}
		}
		if(cbx->currentIndex() < 0) {
			cbx->setCurrentIndex(0);
			settings.setWhenRunnerNotFoundPrint(cbx->currentData().toString());
		}
	}
	auto *event_plugin = qf::gui::framework::getPlugin<Event::EventPlugin>();
	Q_ASSERT(event_plugin);
	auto *event_config = event_plugin->eventConfig();
	Q_ASSERT(event_config);
	m_stageId = qMax(event_config->currentStageId(), 1);
	ui->chkPrintReceiptQrCode->setChecked(event_config->value(eventConfigKey(QStringLiteral("receiptPrintEventQrCode")), false).toBool());
	ui->edReceiptQrCodeBaseUrl->setText(event_config->value(eventConfigKey(QStringLiteral("receiptEventLinkUrl"))).toString().trimmed());
	ui->edReceiptQrCodeCaption->setText(event_config->value(eventConfigKey(QStringLiteral("receiptPrintEventQrCodeCaption")), defaultReceiptQrCodeCaption()).toString().trimmed());
	ui->chkPrintReceiptImage->setChecked(event_config->value(eventConfigKey(QStringLiteral("receiptPrintEventImage")), false).toBool());
	int image_height_mm = event_config->value(eventConfigKey(QStringLiteral("receiptImageHeightMm")), 18).toInt();
	if(image_height_mm < 10)
		image_height_mm = 10;
	else if(image_height_mm > 60)
		image_height_mm = 60;
	ui->edReceiptImageHeight->setValue(image_height_mm);
	m_receiptImageBase64 = event_config->value(eventConfigKey(QStringLiteral("receiptImageDataBase64"))).toString();
	m_receiptImageFormat = event_config->value(eventConfigKey(QStringLiteral("receiptImageFormat"))).toString().trimmed().toLower();
	if(m_receiptImageBase64.isEmpty()) {
		m_receiptImageFormat.clear();
		ui->edReceiptImageFile->clear();
		ui->edReceiptImageFile->setToolTip(QString());
	}
	else {
		ui->edReceiptImageFile->setText(storedReceiptImageLabel(m_receiptImageFormat));
		ui->edReceiptImageFile->setToolTip(tr("Image payload is stored in the event configuration."));
	}
	updateReceiptsPrinterLabel();
	updateReceiptMediaControls();
}

void ReceiptsSettingsPage::save()
{
	ReceiptsSettings settings;
	settings.setAutoPrint(ui->chkAutoPrint->isChecked());
	settings.setThisReaderOnly(ui->chkThisReaderOnly->isChecked());
	settings.setReceiptPath(ui->cbxReceipt->currentData().toString());
	settings.setWhenRunnerNotFoundPrint(ui->cbxWhenRunnerNotFound->currentData().toString());

	auto *event_plugin = qf::gui::framework::getPlugin<Event::EventPlugin>();
	Q_ASSERT(event_plugin);
	auto *event_config = event_plugin->eventConfig();
	Q_ASSERT(event_config);
	event_config->setValue(eventConfigKey(QStringLiteral("receiptPrintEventQrCode")), ui->chkPrintReceiptQrCode->isChecked());
	event_config->setValue(eventConfigKey(QStringLiteral("receiptEventLinkUrl")), ui->edReceiptQrCodeBaseUrl->text().trimmed());
	event_config->setValue(eventConfigKey(QStringLiteral("receiptPrintEventQrCodeCaption")), ui->edReceiptQrCodeCaption->text().trimmed());
	event_config->setValue(eventConfigKey(QStringLiteral("receiptPrintEventImage")), ui->chkPrintReceiptImage->isChecked());
	event_config->setValue(eventConfigKey(QStringLiteral("receiptImageHeightMm")), ui->edReceiptImageHeight->value());
	event_config->setValue(eventConfigKey(QStringLiteral("receiptImageDataBase64")), m_receiptImageBase64);
	event_config->setValue(eventConfigKey(QStringLiteral("receiptImageFormat")), m_receiptImageFormat);
	event_config->save(QStringLiteral("event"));
}

void ReceiptsSettingsPage::loadReceptList()
{
	qfLogFuncFrame();
	ui->cbxReceipt->clear();
	auto *receipts_plugin = qf::gui::framework::getPlugin<ReceiptsPlugin>();
	for(const auto &i : receipts_plugin->listReportFiles("receipts")) {
		qfDebug() << i.reportName << i.reportFilePath;
		ui->cbxReceipt->addItem(i.reportName, i.reportFilePath);
	}
	ui->cbxReceipt->setCurrentIndex(-1);
}

void ReceiptsSettingsPage::updateReceiptsPrinterLabel()
{
	ReceiptsSettings settings;
	ui->btPrinterOptions->setText(settings.printerCaption());
	if(settings.printerTypeEnum() == ReceiptsSettings::PrinterType::GraphicPrinter)
		ui->btPrinterOptions->setIcon(QIcon(":/quickevent/Receipts/images/graphic-printer.svg"));
	else
		ui->btPrinterOptions->setIcon(QIcon(":/quickevent/Receipts/images/character-printer.svg"));
}

void ReceiptsSettingsPage::updateReceiptMediaControls()
{
	const bool qr_code_enabled = ui->chkPrintReceiptQrCode->isChecked();
	ui->edReceiptQrCodeBaseUrl->setEnabled(qr_code_enabled);
	ui->lbReceiptQrCodeCaption->setEnabled(qr_code_enabled);
	ui->edReceiptQrCodeCaption->setEnabled(qr_code_enabled);

	const bool image_enabled = ui->chkPrintReceiptImage->isChecked();
	ui->lbReceiptImageHeight->setEnabled(image_enabled);
	ui->edReceiptImageHeight->setEnabled(image_enabled);
	ui->edReceiptImageFile->setEnabled(image_enabled);
	ui->btSelectReceiptImage->setEnabled(image_enabled);
	ui->btClearReceiptImage->setEnabled(image_enabled && !m_receiptImageBase64.isEmpty());
}

void ReceiptsSettingsPage::onPrinterOptionsClicked()
{
	ReceiptsPrinterOptionsDialog dlg(this);
	//dlg.setPrinterOptions(getPlugin<ReceiptsPlugin>()->receiptsPrinter()->printerOptions());
	if(dlg.exec()) {
		//getPlugin<ReceiptsPlugin>()->setReceiptsPrinterOptions(dlg.printerOptions());
		updateReceiptsPrinterLabel();
	}
}

void ReceiptsSettingsPage::onSelectReceiptImageClicked()
{
	const QString file_path = QFileDialog::getOpenFileName(
		this,
		tr("Select receipt image"),
		QString(),
		tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.svg);;All files (*)"));
	if(file_path.isEmpty())
		return;

	QString image_base64;
	QString image_format;
	QString error_message;
	if(!loadReceiptImagePayload(file_path, image_base64, image_format, error_message)) {
		QMessageBox::warning(this, tr("Warning"), error_message);
		return;
	}

	m_receiptImageBase64 = image_base64;
	m_receiptImageFormat = image_format;
	ui->edReceiptImageFile->setText(QDir::toNativeSeparators(file_path));
	ui->edReceiptImageFile->setToolTip(QDir::toNativeSeparators(file_path));
	ui->chkPrintReceiptImage->setChecked(true);
	updateReceiptMediaControls();
}

void ReceiptsSettingsPage::onClearReceiptImageClicked()
{
	m_receiptImageBase64.clear();
	m_receiptImageFormat.clear();
	ui->edReceiptImageFile->clear();
	ui->edReceiptImageFile->setToolTip(QString());
	updateReceiptMediaControls();
}

QString ReceiptsSettingsPage::eventConfigKey(const QString &suffix) const
{
	return QStringLiteral("event.") + suffix + QStringLiteral(".E") + QString::number(qMax(m_stageId, 1));
}

}
