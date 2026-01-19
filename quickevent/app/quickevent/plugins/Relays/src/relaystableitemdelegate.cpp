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
	m_classNameCol = class_col;
	m_legsCol = legs_col;
}

void RelaysTableItemDelegate::addClassLegs(QString class_name, int legs)
{
	m_legsCount[class_name] = legs;
}

void RelaysTableItemDelegate::paintBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Super::paintBackground(painter, option, index);

	qf::gui::TableView *v = view();
	if(!v)
		return;

	auto *m = v->model();
	auto *tm = qobject_cast< qf::gui::model::SqlTableModel*>(v->tableModel());
	if(!(m && tm && m_classNameCol.has_value() && m_legsCol.has_value()))
		return;

	if(index.column() == m_legsCol.value()) {
		auto legs_cnt = m_legsCount[m->data(index.sibling(index.row(),m_classNameCol.value()), Qt::EditRole).toString()];
		auto legs = m->data(index.sibling(index.row(),m_legsCol.value()), Qt::EditRole).toInt();
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
