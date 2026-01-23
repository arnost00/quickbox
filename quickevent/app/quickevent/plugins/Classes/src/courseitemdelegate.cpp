#include "courseitemdelegate.h"

#include <qf/core/assert.h>

#include <QComboBox>

CourseItemDelegate::CourseItemDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{}

QWidget *CourseItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)
	auto *editor = new QComboBox(parent);
	QMap<QString, int> name_to_id;
	if (!m_nullText.isEmpty()) {
		editor->addItem(m_nullText, {});
	}
	for (const auto &[id, name] : m_idToCourseName.asKeyValueRange()) {
		name_to_id[name] = id;
	}
	for (const auto &[name, id] : name_to_id.asKeyValueRange()) {
		editor->addItem(name, id);
	}
	return editor;
}
void CourseItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	auto *cbx = qobject_cast<QComboBox *>(editor);
	QF_ASSERT(cbx != nullptr, "Bad combo!", return);
	QString id = index.data(Qt::EditRole).toString();
	int ix = cbx->findData(id);
	cbx->setCurrentIndex(ix);
}
void CourseItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	qfLogFuncFrame();
	auto *cbx = qobject_cast<QComboBox *>(editor);
	QF_ASSERT(cbx != nullptr, "Bad combo!", return);
	qfDebug() << "setting model data:" << cbx->currentText() << cbx->currentData();
	model->setData(index, cbx->currentData(), Qt::EditRole);
	emit const_cast<CourseItemDelegate*>(this)->courseIdChanged(); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

QString CourseItemDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
	Q_UNUSED(locale)
	return m_idToCourseName.value(value.toInt(), "???");
}

void CourseItemDelegate::setCourses(const QMap<int, QString> &courses)
{
	m_idToCourseName = courses;
}
