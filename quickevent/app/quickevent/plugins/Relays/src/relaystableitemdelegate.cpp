#include "relaystableitemdelegate.h"

#include <qf/gui/tableview.h>
#include <qf/gui/model/sqltablemodel.h>

#include <QPainter>

RelaysTableItemDelegate::RelaysTableItemDelegate(qf::gui::TableView * parent)
	: Super(parent)
{
}

void RelaysTableItemDelegate::setColumns(int class_col, int legs_col)
{
	m_class_name_col = class_col;
	m_legs_col = legs_col;
}

void RelaysTableItemDelegate::addClassLegs(QString class_name, int legs)
{
	m_legs_count[class_name] = legs;
}

void RelaysTableItemDelegate::paintBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Super::paintBackground(painter, option, index);

	qf::gui::TableView *v = view();
	if(!v)
		return;

	auto *m = v->model();
	auto *tm = qobject_cast< qf::gui::model::SqlTableModel*>(v->tableModel());
	if(!(m && tm))
		return;

	if(index.column() == m_legs_col) {
		auto legs_cnt = m_legs_count[m->data(index.sibling(index.row(),m_class_name_col), Qt::EditRole).toString()];
		auto legs = m->data(index.sibling(index.row(),m_legs_col), Qt::EditRole).toInt();
		if(legs == 0) {
			QColor c = Qt::red;
			c.setAlphaF(0.3);
			painter->fillRect(option.rect, c);
		}
		else if (legs > 0 && legs < legs_cnt) {
			QColor c = Qt::magenta;
			c.setAlphaF(0.3);
			painter->fillRect(option.rect, c);
		}
		else if (legs > legs_cnt) {
			QColor c = Qt::yellow;
			c.setAlphaF(0.3);
			painter->fillRect(option.rect, c);
		}
	}
}
