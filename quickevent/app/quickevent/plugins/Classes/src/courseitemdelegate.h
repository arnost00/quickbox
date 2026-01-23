#pragma once

#include <QStyledItemDelegate>

class CourseItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	CourseItemDelegate(QObject *parent);

	Q_SIGNAL void courseIdChanged();

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

	QString displayText(const QVariant &value, const QLocale &locale) const override;
	void setNullText(const QString &text) { m_nullText = text; }

	void setCourses(const QMap<int, QString> &courses);
private:
	QMap<int, QString> m_idToCourseName;
	QString m_nullText;
};


