#include "utils.h"

#include <QDateTime>
#include <QTimeZone>


namespace quickevent::core {

 QString Utils::dateTimeToIsoStringWithUtcOffset(QDateTime dt)
{
	// Qt emits no UTC offset for Qt::LocalTime, which means "zone unspecified",
	// see https://bugreports.qt.io/browse/QTBUG-26161?focusedCommentId=554227
	// Other time representations (UTC, fixed offset, named zone) already print the offset.
	if (dt.timeSpec() == Qt::LocalTime) {
		dt.setTimeZone(QTimeZone::systemTimeZone());
	}
	return dt.toString(Qt::ISODateWithMs);
}

} // namespace quickevent::core

