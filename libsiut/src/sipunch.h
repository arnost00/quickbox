#ifndef SIUT_SIPUNCH_H
#define SIUT_SIPUNCH_H

#include <siut/siutglobal.h>

#include <QByteArray>
#include <QVariantMap>

namespace siut {

struct SIUT_DECL_EXPORT SIPunch
{
	enum DayOfWeek {Sunday = 0, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday};

	int cardNumber = 0;
	int code = 0;
	int time = 0;
	int msec = 0;
	bool pmFlag = false;
	int dayOfWeek = 0;
	int weekCnt = 0;

	SIPunch() = default;
	SIPunch(int code, int time);
	SIPunch(const QByteArray &card_data, int ix);

	static SIPunch fromVariantMap(const QVariantMap &m);
	QVariantMap toVariantMap() const;

	bool operator==(const SIPunch &other) const;

	static unsigned getUnsigned(const QByteArray &ba, int ix, int byte_cnt = 2);
};

} // namespace siut

#endif // SIPUNCH_H
