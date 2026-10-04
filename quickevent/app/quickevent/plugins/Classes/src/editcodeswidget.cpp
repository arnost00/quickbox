#include "editcodeswidget.h"
#include "ui_editcodeswidget.h"

#include <qf/gui/model/sqltablemodel.h>
#include <qf/core/sql/query.h>
#include <qf/core/exception.h>
#include <qf/core/log.h>

namespace qfm = qf::gui::model;
namespace qfs = qf::core::sql;

namespace {
class CodesTableModel : public qfm::SqlTableModel
{
	Q_OBJECT
private:
	using Super = qfm::SqlTableModel;
public:
	CodesTableModel(QObject *parent) : Super(parent) {}

	bool dropRows(int row_ix, int count, bool throw_exc) override
	{
		/// check before rows are removed from the model, failure in removeTableRow() leaves the view inconsistent
		for(int i=row_ix; i<row_ix+count; i++) {
			QString err = deleteCodeError(i);
			if(!err.isEmpty()) {
				if(throw_exc)
					QF_EXCEPTION(err);
				qfError() << err;
				return false;
			}
		}
		return Super::dropRows(row_ix, count, throw_exc);
	}
private:
	QString deleteCodeError(int row_no) const
	{
		static constexpr int MAX_LISTED_COURSES = 10;
		const auto row = m_table.row(row_no);
		int code_id = row.value("codes.id").toInt();
		if(code_id <= 0)
			return {};
		qfs::Query q(sqlConnection());
		q.exec("SELECT DISTINCT courses.name FROM coursecodes"
			   " INNER JOIN courses ON courses.id=coursecodes.courseId"
			   " WHERE coursecodes.codeId=" QF_IARG(code_id)
			   " ORDER BY courses.name", qf::core::Exception::Throw);
		QStringList courses;
		while(q.next())
			courses << q.value(0).toString();
		if(courses.isEmpty())
			return {};
		QString course_list = QStringList(courses.mid(0, MAX_LISTED_COURSES)).join(", ");
		if(courses.count() > MAX_LISTED_COURSES)
			course_list += ", " + tr("and %n more", nullptr, static_cast<int>(courses.count()) - MAX_LISTED_COURSES);
		return tr("Cannot delete code %1, it is used in %n course(s): %2.", nullptr, static_cast<int>(courses.count()))
				.arg(row.value("codes.code").toInt())
				.arg(course_list);
	}
};
}

EditCodesWidget::EditCodesWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::EditCodesWidget)
{
	setTitle(tr("Codes"));
	setPersistentSettingsId("EditCodesWidget");
	ui->setupUi(this);
	{
		ui->tableView->setPersistentSettingsId("tableView");
		//ui->tableView->setEditRowsSectionEnabled(false);
		ui->tableView->setDirtyRowsMenuSectionEnabled(false);
		ui->tableViewTB->setTableView(ui->tableView);
		ui->tableView->setRemoveRowsQuestion([](int row_count) {
			return tr("Do you really want to delete %n code(s)? This cannot be undone.", nullptr, row_count);
		});
		auto *m = new CodesTableModel(this);
		//m->setObjectName("classes.classesModel");
		m->addColumn("id").setReadOnly(true);
		//m->addColumn("codes.type", tr("Type", "control type")).setToolTip(tr("Control type"));
		m->addColumn("codes.code", tr("Code"));
		m->addColumn("codes.altCode", tr("Alt")).setToolTip(tr("Code alternative"));
		m->addColumn("codes.note", tr("Note"));
		m->addColumn("codes.outOfOrder", tr("Out of order")).setToolTip(tr("Out of order"));
		m->addColumn("codes.radio", tr("Radio"));
		m->addColumn("codes.longitude", tr("Long")).setToolTip(tr("Longitude"));
		m->addColumn("codes.latitude", tr("Lat")).setToolTip(tr("Latitude"));
		ui->tableView->setTableModel(m);
		m_tableModel = m;
	}
	{
		qfs::QueryBuilder qb;
		qb.select2("codes", "*")
				.from("codes")
				.orderBy("codes.code");//.limit(10);
		m_tableModel->setQueryBuilder(qb, false);
		m_tableModel->reload();
	}
}

EditCodesWidget::~EditCodesWidget()
{
	delete ui;
}

#include "editcodeswidget.moc"
