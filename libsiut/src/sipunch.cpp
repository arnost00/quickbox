#include "sipunch.h"

#include <qf/core/log.h>

namespace siut {

SIPunch::SIPunch(int code, int time)
	: code(code), time(time)
{
}

SIPunch::SIPunch(const QByteArray &card_data, int ix)
{
	/*
	record structure: PTD - CN - PTH - PTL
	CN - control station code number, 0...255 or subsecond value1
	PTD - day of week / halfday
	bit 0 - am/pm
	bit 3...1 - day of week, 000 = Sunday, 110 = Saturday
	bit 5...4 - week counter 0...3, relative
	bit 7...6 - control station code number high
	(...511)
	punching time PTH, PTL - 12h binary
	*/
	time = static_cast<int>(getUnsigned(card_data, ix + 2, 2));
	auto pdt = static_cast<uint8_t>(card_data[ix]);
	uint16_t code_complete = static_cast<uint16_t>(((pdt & 0x60) >> 6) << 8);
	code_complete += static_cast<uint8_t>(card_data[ix + 1]);
	code     = code_complete;
	pmFlag   = pdt & 1;
	dayOfWeek = (pdt & 0x0e) >> 1;
	weekCnt   = (pdt & 0x30) >> 4;
}

SIPunch SIPunch::fromVariantMap(const QVariantMap &m)
{
	SIPunch p;
	p.cardNumber = m.value(QStringLiteral("cardNumber")).toInt();
	p.code       = m.value(QStringLiteral("code")).toInt();
	p.time       = m.value(QStringLiteral("time")).toInt();
	p.msec       = m.value(QStringLiteral("msec")).toInt();
	p.pmFlag     = m.value(QStringLiteral("pmFlag")).toBool();
	p.dayOfWeek  = m.value(QStringLiteral("dayOfWeek")).toInt();
	p.weekCnt    = m.value(QStringLiteral("weekCnt")).toInt();
	return p;
}

QVariantMap SIPunch::toVariantMap() const
{
	QVariantMap m;
	m[QStringLiteral("cardNumber")] = cardNumber;
	m[QStringLiteral("code")]       = code;
	m[QStringLiteral("time")]       = time;
	m[QStringLiteral("msec")]       = msec;
	m[QStringLiteral("pmFlag")]     = pmFlag;
	m[QStringLiteral("dayOfWeek")]  = dayOfWeek;
	m[QStringLiteral("weekCnt")]    = weekCnt;
	return m;
}

bool SIPunch::operator==(const SIPunch &other) const
{
	return cardNumber == other.cardNumber
		&& code      == other.code
		&& time      == other.time
		&& msec      == other.msec
		&& pmFlag    == other.pmFlag
		&& dayOfWeek == other.dayOfWeek
		&& weekCnt   == other.weekCnt;
}

unsigned SIPunch::getUnsigned(const QByteArray &ba, int ix, int byte_cnt)
{
	unsigned ret = 0;
	if(ix + byte_cnt <= ba.size()) {
		for (int i = 0; i < byte_cnt; ++i) {
			ret <<= 8;
			ret += static_cast<uint8_t>(ba[ix + i]);
		}
	}
	else {
		qfError() << "array too short";
	}
	return ret;
}

} // namespace siut
