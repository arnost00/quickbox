#pragma once

#include <quickevent/gui/og/itemdelegate.h>

class RelaysTableItemDelegate : public quickevent::gui::og::ItemDelegate
{
	Q_OBJECT
private:
	typedef quickevent::gui::og::ItemDelegate Super;
public:
	RelaysTableItemDelegate(qf::gui::TableView * parent = nullptr);

	void setColumns(int class_col, int legs_col);
	void addClassLegs(QString class_name, int legs);
	void resetClassLegs() { m_legs_count.clear(); }
protected:
	void paintBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
private:
	int m_class_name_col = -1;
	int m_legs_col = -1;
	QMap <QString, int> m_legs_count;
};
