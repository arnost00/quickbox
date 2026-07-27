#pragma once

#include <plugins/Core/src/widgets/settingspage.h>

#include <QMap>

class QCheckBox;

namespace Ui {
class RunsSettingsPage;
}

namespace Runs {

class RunsSettingsPage : public Core::SettingsPage
{
	Q_OBJECT
private:
	using Super = Core::SettingsPage;

public:
	explicit RunsSettingsPage(QWidget *parent = nullptr);
	~RunsSettingsPage() override;

	void load() override;
	void save() override;

private:
	::Ui::RunsSettingsPage *ui;
	QMap<int, QCheckBox*> m_columnCheckBoxes;
};

} // namespace Runs
