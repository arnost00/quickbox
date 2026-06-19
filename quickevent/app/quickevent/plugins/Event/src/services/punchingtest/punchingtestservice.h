#pragma once

#include "../service.h"

class QTimer;

namespace Event {
namespace services {

class PunchingTestServiceSettings : public ServiceSettings
{
	using Super = ServiceSettings;

	QF_VARIANTMAP_FIELD2(int, p, setP, unchInterval, 10)

public:
	PunchingTestServiceSettings(const QVariantMap &o = QVariantMap()) : Super(o) {}
};

class PunchingTestService : public Service
{
	Q_OBJECT
	using Super = Service;
public:
	explicit PunchingTestService(QObject *parent);

	void run() override;
	void stop() override;

	PunchingTestServiceSettings settings() const { return PunchingTestServiceSettings(m_settings); }
	static QString serviceName();
	QString serviceDisplayName() const override;

private:
	void onTimerTick();
	qf::gui::framework::DialogWidget *createDetailWidget() override;

private:
	QTimer *m_timer = nullptr;
};

}} // namespace Event::services
