#include "runssettingspage.h"
#include "ui_runssettingspage.h"

#include "runstablemodel.h"
#include "runstablewidget.h"
#include "runsplugin.h"
#include "runswidget.h"

#include <plugins/Event/src/eventplugin.h>

#include <qf/core/sql/connection.h>
#include <qf/core/sql/query.h>
#include <qf/gui/framework/mainwindow.h>
#include <qf/gui/framework/plugin.h>
#include <qf/gui/tableview.h>

#include <QCheckBox>

namespace Runs {

namespace {

RunsTableWidget *runsTableWidget()
{
    auto runs_plugin = qf::gui::framework::getPlugin<RunsPlugin>();
	auto *runs_widget = runs_plugin->runsWidget();
	Q_ASSERT(runs_widget);
	return runs_widget->findChild<RunsTableWidget*>();
}

}

RunsSettingsPage::RunsSettingsPage(QWidget *parent)
	: Super(parent)
{
	m_caption = tr("Runs");
	ui = new ::Ui::RunsSettingsPage;
	ui->setupUi(this);

	auto *model = runsTableWidget()->runsModel();
	for(int column = 0; column < model->columnCount({}); ++column) {
		auto *check_box = new QCheckBox(model->headerData(column, Qt::Horizontal).toString(), this);
		ui->columnsLayout->addWidget(check_box);
		m_columnCheckBoxes.insert(column, check_box);
	}
	ui->columnsLayout->addStretch();
}

RunsSettingsPage::~RunsSettingsPage()
{
	delete ui;
}

void RunsSettingsPage::load()
{
	for(auto *check_box : m_columnCheckBoxes) {
		check_box->setChecked(true);
	}

	if(!qf::gui::framework::getPlugin<Event::EventPlugin>()->isEventOpen()) {
		setEnabled(false);
		return;
	}
	setEnabled(true);

	if(auto *runs_table = runsTableWidget()) {
		for(auto it = m_columnCheckBoxes.cbegin(); it != m_columnCheckBoxes.cend(); ++it) {
			it.value()->setChecked(!runs_table->tableView()->isColumnHidden(it.key()));
		}
	}
}

void RunsSettingsPage::save()
{
	if(!qf::gui::framework::getPlugin<Event::EventPlugin>()->isEventOpen()) {
		return;
	}

	QStringList hidden_columns;
	auto *runs_table = runsTableWidget();
	for(auto it = m_columnCheckBoxes.cbegin(); it != m_columnCheckBoxes.cend(); ++it) {
		if(!it.value()->isChecked()) {
			hidden_columns << runs_table->runsModel()->columnDefinition(it.key()).fieldName();
		}
		if(runs_table) {
			runs_table->setColumnVisible(it.key(), it.value()->isChecked());
		}
	}
	RunsPlugin::saveRunsTableHiddenColumns(hidden_columns);
}

} // namespace Runs
