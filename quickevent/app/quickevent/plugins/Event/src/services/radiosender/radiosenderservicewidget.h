#pragma once

#include <qf/gui/framework/dialogwidget.h>

namespace Event::services {

namespace Ui { class RadioSenderServiceWidget; }

class RadioSenderService;

class RadioSenderServiceWidget : public qf::gui::framework::DialogWidget
{
	Q_OBJECT
	using Super = qf::gui::framework::DialogWidget;
public:
	explicit RadioSenderServiceWidget(RadioSenderService *service, QWidget *parent = nullptr);
	~RadioSenderServiceWidget() override;

	bool acceptDialogDone(int result) override;
private:
	void saveConfig();
	void updateReceivedLineLog();
	void updateServiceControls();

	void onTestPunch(const QString &line);

	Ui::RadioSenderServiceWidget *ui;
	RadioSenderService *m_service = nullptr;
};

} // namespace Event::services
