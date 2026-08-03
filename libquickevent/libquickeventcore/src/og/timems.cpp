#include "timems.h"

#include <qf/core/log.h>

#include <QString>
#include <QRegularExpression>



namespace quickevent::core::og {

//bool TimeMs::m_oneTenthSecPrecision = false;

TimeMs::TimeMs()
	: m_msec(0), m_isValid(false)
{

}

TimeMs::TimeMs(int msec)
	: m_msec(msec), m_isValid(true)
{

}

TimeMs TimeMs::fromVariant(const QVariant &time_v)
{
	if(time_v.userType() == QMetaType::Int)
		return TimeMs(time_v.toInt());
	return TimeMs();
}

QString TimeMs::toString(QChar sec_sep, QChar msec_sep) const
{
	if(!isValid())
		return QString();

	int msec = m_msec;
	bool is_neg = false;
	if(msec < 0) {
		msec = -msec;
		is_neg = true;
	}
	int ms = msec % 1000;
	int sec = (msec / 1000) % 60;
	int min = msec / (1000 * 60);
	QString ret = QString::number(min) + sec_sep;
	if(sec < 10)
		ret += '0';
	ret += QString::number(sec);
	if(!msec_sep.isNull()) {
		ret += msec_sep;
		if(ms < 100)
			ret += '0';
		if(ms < 10)
			ret += '0';
		ret += QString::number(ms);
	}
	if(is_neg)
		ret = '-' + ret;
	return ret;
}

QString TimeMs::toString() const
{
	if(msec() % 1000) {
		return toString('.', '/');
	}
	return toString('.', {});
}



TimeMs TimeMs::fromString(const QString &time_str)
{
	if(time_str.isEmpty())
		return TimeMs();

	static const QRegularExpression re(R"(^(\d+)(?:[.:\-,](\d+)(?:\/(\d+))?)?$)");
	auto m = re.match(time_str.trimmed());
	if(!m.hasMatch()) {
		qfWarning() << "Invalid OGTime string" << time_str;
		return TimeMs();
	}
	int min  = m.captured(1).toInt();
	int sec  = m.captured(2).toInt();
	int msec = m.captured(3).toInt();

	return TimeMs(msec + ((sec + (min * 60)) * 1000));
}

int TimeMs::fixTimeWrapAM(int time1_msec, int time2_msec)
{
	constexpr int HR_12_MSEC = 12 * 60 * 60 * 1000;
	while(time2_msec < time1_msec)
		time2_msec += HR_12_MSEC;
	while(time1_msec <= time2_msec - HR_12_MSEC)
		time2_msec -= HR_12_MSEC;
	return time2_msec;
}

int TimeMs::msecIntervalAM(int from_time_msec, int to_time_msec)
{
	return fixTimeWrapAM(from_time_msec, to_time_msec) - from_time_msec;
}

void TimeMs::registerQVariantFunctions()
{
	static bool registered = false;
	if(!registered) {
		registered = true;
#if QT_VERSION_MAJOR < 6
		{
			bool ok = QMetaType::registerComparators<TimeMs>();
			if(!ok)
				qfError() << "Error registering comparators for quickevent::core::og::TimeMs!";
		}
#endif
		{
			bool ok = QMetaType::registerConverter<TimeMs, int>([](const TimeMs &t) -> int {return t.msec();});
			if(!ok)
				qfError() << "Error registering converter for quickevent::core::og::TimeMs!";
		}
	}
}

}
