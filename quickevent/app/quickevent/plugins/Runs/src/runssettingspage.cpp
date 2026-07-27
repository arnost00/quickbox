#include "runssettingspage.h"
#include "ui_runssettingspage.h"

#include "runstablemodel.h"
#include "runstablewidget.h"

#include <plugins/Event/src/eventplugin.h>

#include <qf/core/sql/connection.h>
#include <qf/core/sql/query.h>
#include <qf/gui/framework/mainwindow.h>
#include <qf/gui/framework/plugin.h>
#include <qf/gui/tableview.h>

#include <QCheckBox>

namespace Runs {
namespace {

const auto config_key = QStringLiteral("runs.hiddenColumns");

RunsTableWidget *runsTableWidget()
{
	return qf::gui::framework::MainWindow::frameWork()->findChild<RunsTableWidget*>();
}

}

RunsSettingsPage::RunsSettingsPage(QWidget *parent)
	: Super(parent)
{
	m_caption = tr("Runs");
	ui = new ::Ui::RunsSettingsPage;
	ui->setupUi(this);

	auto *model = new RunsTableModel(this);
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

	RunsTableModel model;
	QStringList hidden_columns;
	auto *runs_table = runsTableWidget();
	for(auto it = m_columnCheckBoxes.cbegin(); it != m_columnCheckBoxes.cend(); ++it) {
		if(!it.value()->isChecked()) {
			hidden_columns << model.columnDefinition(it.key()).fieldName();
		}
		if(runs_table) {
			runs_table->setColumnVisible(it.key(), it.value()->isChecked());
		}
	}

	using namespace qf::core::sql;
	Query update_query(Connection::forName());
	update_query.prepare("UPDATE config SET cvalue=:value, ctype='QString' WHERE ckey=:key", qf::core::Exception::Throw);
	update_query.bindValue(":key", config_key);
	update_query.bindValue(":value", hidden_columns.join(','));
	update_query.exec(qf::core::Exception::Throw);
	if(update_query.numRowsAffected() < 1) {
		Query insert_query(Connection::forName());
		insert_query.prepare("INSERT INTO config (ckey, cvalue, ctype) VALUES (:key, :value, 'QString')", qf::core::Exception::Throw);
		insert_query.bindValue(":key", config_key);
		insert_query.bindValue(":value", hidden_columns.join(','));
		insert_query.exec(qf::core::Exception::Throw);
	}

	Query delete_legacy_query(Connection::forName());
	delete_legacy_query.exec("DELETE FROM config WHERE ckey LIKE 'runs.tableColumn.%' OR ckey='runs.visibleColumns'", qf::core::Exception::Throw);
}

} // namespace Runs
