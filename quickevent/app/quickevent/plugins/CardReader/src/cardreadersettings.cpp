#include "cardreadersettings.h"

CardReaderSettings::ReaderMode CardReaderSettings::readerModeEnum() const
{
	if(readerMode() == "EditOnPunch")
		return ReaderMode::EditOnPunch;
	return ReaderMode::Readout;
}

CardReaderSettings::ReaderType CardReaderSettings::readerTypeEnum() const
{
	if(readerType() == "BTSIReader")
		return ReaderType::BTSIReader;
	return ReaderType::Serial;
}
