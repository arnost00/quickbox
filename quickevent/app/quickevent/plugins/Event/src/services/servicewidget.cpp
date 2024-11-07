#include "servicewidget.h"
#include "ui_servicewidget.h"

namespace Event {
namespace services {

ServiceWidget::ServiceWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ServiceWidget)
{
	ui->setupUi(this);

	connect(ui->btPlay, &QPushButton::clicked, this, &ServiceWidget::onBtPlayClicked);
	connect(ui->btShowDetail, &QPushButton::clicked, this, &ServiceWidget::showDetail);
}

ServiceWidget::~ServiceWidget()
{
	delete ui;
}

void ServiceWidget::setStatus(Service::Status st)
{
	m_isRunning = (st == Service::Status::Running);
	static QIcon ico_play(":/qf/qmlwidgets/images/flat/media-play");
	static QIcon ico_stop(":/qf/qmlwidgets/images/flat/media-stop");
	ui->btPlay->setIcon(m_isRunning? ico_stop: ico_play);
	switch (st) {
	case Service::Status::Running:
		ui->lblStatus->setPixmap(QPixmap(":/qf/qmlwidgets/images/light-green"));
		break;
	case Service::Status::Stopped:
		ui->lblStatus->setPixmap(QPixmap(":/qf/qmlwidgets/images/light-red"));
		break;
	default:
		ui->lblStatus->setPixmap(QPixmap(":/qf/qmlwidgets/images/light-yellow"));
		break;
	}
}

void ServiceWidget::setServiceId(const QString &id, const QString &display_name)
{
	m_serviceId = id;
	ui->lblServiceName->setText(display_name.isEmpty()? id: display_name);
}

QString ServiceWidget::serviceId() const
{
	return m_serviceId;
}

void ServiceWidget::setMessage(const QString &m)
{
	ui->lblServiceMessage->setText(m);
}

void ServiceWidget::onBtPlayClicked()
{
	emit setRunningRequest(!m_isRunning);
}

void ServiceWidget::showDetail()
{
	Service *svc = Service::serviceByName(serviceId());
	if(svc) {
		svc->showDetail(this);
	}
}

}}
