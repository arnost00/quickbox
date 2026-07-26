#pragma once

#include <qf/gui/framework/dialogwidget.h>

namespace Event::services {

namespace Ui { class RadioSenderServiceWidget; }

class RadioSenderServiceWidget : public qf::gui::framework::DialogWidget
{
	Q_OBJECT
	using Super = qf::gui::framework::DialogWidget;
public:
	explicit RadioSenderServiceWidget(QWidget *parent = nullptr);
	~RadioSenderServiceWidget() override;

private:
	bool acceptDialogDone(int result) override;
	void saveConfig();
	void updateReceivedLineLog();

	Ui::RadioSenderServiceWidget *ui;
	class RadioSenderService *m_service = nullptr;
};

} // namespace Event::services
