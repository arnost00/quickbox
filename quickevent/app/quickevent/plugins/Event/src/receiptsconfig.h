#pragma once

#include <QString>

namespace Event {

struct ReceiptsConfig
{
	bool printQrCode = false;
	QString linkUrl;
	QString qrCodeCaption;   // Default "Live Results" applied by AppDbConfig::receiptsConfig()
	bool printImage = false;
	int imageHeightMm = 18;  // Clamped to [10, 60]
	QString imageBase64;
	QString imageFormat;     // Default "png" applied by AppDbConfig::receiptsConfig()
};

}
