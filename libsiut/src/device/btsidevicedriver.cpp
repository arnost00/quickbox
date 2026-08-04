//
// BtSiDeviceDriver — Bluetooth LE SI reader device driver (Qt Bluetooth)
//
// Copyright: See COPYING file that comes with this distribution
//

#include "btsidevicedriver.h"
#include "sitask.h"

#include <qf/core/log.h>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyDescriptor>
#include <QVariantList>

#include <ios>

namespace siut {

// ===========================================================================
// Constants
// ===========================================================================

namespace {

// SI BLE characteristic UUIDs
constexpr const char *CARD_STATE_UUID		= "bd510012-6aec-4628-a146-f3e95bc49e62";
constexpr const char *CARD_DATA_UUID		= "bd510013-6aec-4628-a146-f3e95bc49e62";

// Message IDs (little-endian u16, bytes 0-1 of a reassembled message)
constexpr quint16 MSG_CARD_STATE		= 0x1101;
constexpr quint16 MSG_CARD_MINIMAL		= 0x1102;
constexpr quint16 MSG_CARD_COMPLETE		= 0x1103;

// Wrapper chunk flags (payload byte 0)
constexpr quint8 WRAP_FLAG_FIRST		= 0x01;
constexpr quint8 WRAP_FLAG_MIDDLE		= 0x00;
constexpr quint8 WRAP_FLAG_LAST			= 0x02;

// Punch type codes
constexpr quint8 PUNCH_TYPE_CLEAR		= 1;
constexpr quint8 PUNCH_TYPE_CHECK		= 2;
constexpr quint8 PUNCH_TYPE_START		= 3;
constexpr quint8 PUNCH_TYPE_FINISH		= 5;
constexpr quint8 PUNCH_TYPE_CONTROL		= 7;

} // anonymous namespace

// ===========================================================================
// BtSiReassembler
// ===========================================================================

void BtSiReassembler::reset()
{
	m_buf.clear();
	m_expectedLen = 0;
	m_started = false;
}

QByteArray BtSiReassembler::feed(const QByteArray &chunk)
{
	if (chunk.size() < 2)
		return {};

	quint16 msgId = static_cast<quint16>(static_cast<quint8>(chunk[0]))
	              | (static_cast<quint16>(static_cast<quint8>(chunk[1])) << 8);

	if (msgId != WRAPPER_MSG_ID) {
		// Non-wrapper: complete message as-is
		reset();
		return chunk;
	}

	// Wrapper packet: bytes 2-3 = payload_len, bytes 4+ = payload
	if (chunk.size() < 4)
		return {};

	quint16 payloadLen = static_cast<quint16>(static_cast<quint8>(chunk[2]))
	                   | (static_cast<quint16>(static_cast<quint8>(chunk[3])) << 8);

	if (chunk.size() < 4 + payloadLen || payloadLen < 1)
		return {};

	auto flag = static_cast<quint8>(chunk[4]);

	if (flag == WRAP_FLAG_FIRST) {
		// payload[1..5] = total_len (u32le), payload[5..] = data
		if (payloadLen < 5)
			return {};
		quint32 totalLen =
			  static_cast<quint32>(static_cast<quint8>(chunk[5]))
			| (static_cast<quint32>(static_cast<quint8>(chunk[6])) << 8)
			| (static_cast<quint32>(static_cast<quint8>(chunk[7])) << 16)
			| (static_cast<quint32>(static_cast<quint8>(chunk[8])) << 24);
		reset();
		m_expectedLen = static_cast<int>(totalLen);
		m_started = true;
		// data starts at chunk[9] (4 header bytes + 1 flag + 4 total_len bytes)
		if (chunk.size() > 9)
			m_buf.append(chunk.constData() + 9, chunk.size() - 9);

	} else if (flag == WRAP_FLAG_MIDDLE) {
		if (!m_started)
			return {};
		// data = payload[1..] → chunk[5..]
		if (chunk.size() > 5)
			m_buf.append(chunk.constData() + 5, chunk.size() - 5);

	} else if (flag == WRAP_FLAG_LAST) {
		if (!m_started)
			return {};
		// data = payload[1..] → chunk[5..]
		if (chunk.size() > 5)
			m_buf.append(chunk.constData() + 5, chunk.size() - 5);

		if (m_buf.size() == m_expectedLen) {
			QByteArray result = m_buf;
			reset();
			return result;
		}
		// Size mismatch — discard
		reset();
	}

	return {};
}

// ===========================================================================
// BtSiDeviceDriver — utility helpers
// ===========================================================================

quint16 BtSiDeviceDriver::readU16LE(const QByteArray &ba, int offset)
{
	if (offset + 1 >= ba.size())
		return 0;
	return static_cast<quint16>(static_cast<quint8>(ba[offset]))
	     | (static_cast<quint16>(static_cast<quint8>(ba[offset + 1])) << 8);
}

quint32 BtSiDeviceDriver::readU32LE(const QByteArray &ba, int offset)
{
	if (offset + 3 >= ba.size())
		return 0;
	return  static_cast<quint32>(static_cast<quint8>(ba[offset]))
	     | (static_cast<quint32>(static_cast<quint8>(ba[offset + 1])) << 8)
	     | (static_cast<quint32>(static_cast<quint8>(ba[offset + 2])) << 16)
	     | (static_cast<quint32>(static_cast<quint8>(ba[offset + 3])) << 24);
}

void BtSiDeviceDriver::emitInfo(NecroLog::Level level, const QString &msg)
{
	emit driverInfo(level, msg);
}

// ===========================================================================
// BtSiDeviceDriver — construction / destruction
// ===========================================================================

BtSiDeviceDriver::BtSiDeviceDriver(QObject *parent)
	: Super(parent)
	, m_cardStateUuid(QString(CARD_STATE_UUID))
	, m_cardDataUuid(QString(CARD_DATA_UUID))
{
	// m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
	// m_discoveryAgent->setLowEnergyDiscoveryTimeout(10000);

	// connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BtSiDeviceDriver::onDeviceDiscovered);
	// connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BtSiDeviceDriver::onScanFinished);
	// connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &BtSiDeviceDriver::onScanError);
}

BtSiDeviceDriver::~BtSiDeviceDriver()
{
	// Disconnect without emitting signals from a partially-destroyed object
	// if (m_discoveryAgent->isActive())
	// 	m_discoveryAgent->stop();

	for (auto *svc : m_services)
		svc->deleteLater();
	m_services.clear();

	if (m_controller) {
		m_controller->disconnectFromDevice();
		delete m_controller;
		m_controller = nullptr;
	}
}

// ===========================================================================
// BtSiDeviceDriver — public API
// ===========================================================================

bool BtSiDeviceDriver::isConnected() const
{
	return m_connected;
}

// void BtSiDeviceDriver::connectToDevice(const QString &address)
// {
// 	if (address.isEmpty()) {
// 		emitInfo(NecroLog::Level::Error, tr("connectToDevice: empty address."));
// 		return;
// 	}
// 	m_targetAddress = address.toUpper().trimmed();
// 	emitInfo(NecroLog::Level::Info,
// 	         tr("Starting BLE scan for device %1 ...").arg(m_targetAddress));
// 	startScan();
// }

void BtSiDeviceDriver::connectToDevice(const QBluetoothDeviceInfo &info)
{
	if (!info.isValid()) {
		emitInfo(NecroLog::Level::Error, tr("connectToDevice: invalid QBluetoothDeviceInfo."));
		return;
	}
	m_deviceInfo = info;
	emitInfo(NecroLog::Level::Info,
	         tr("Connecting to BLE device %1 %2 ...").arg(info.address().toString().toUpper()).arg(info.name()));
	createController(info);
}

void BtSiDeviceDriver::disconnectFromDevice()
{
	// if (m_discoveryAgent->isActive())
	// 	m_discoveryAgent->stop();

	for (auto *svc : m_services)
		svc->deleteLater();
	m_services.clear();

	if (m_controller) {
		m_controller->disconnectFromDevice();
		m_controller->deleteLater();
		m_controller = nullptr;
	}

	m_cardStateSubscribed = false;
	m_cardDataSubscribed = false;
	m_pendingServices = 0;
	m_cardStateReassembler.reset();
	m_cardDataReassembler.reset();

	if (m_connected) {
		m_connected = false;
		emit connectionStateChanged(false);
	}
}

// ===========================================================================
// BtSiDeviceDriver — scan
// ===========================================================================

// void BtSiDeviceDriver::startScan()
// {
// 	if (m_discoveryAgent->isActive())
// 		m_discoveryAgent->stop();
// 	m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
// }

// void BtSiDeviceDriver::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
// {
// 	if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
// 		return;

// 	bool matches = false;
// 	qfInfo() << "BT device discovered:" << info.address().toString().toUpper() << info.name();
// 	// MAC address format (Linux, Windows)
// 	if (m_targetAddress.contains(':')) {
// 		matches = info.address().toString().toUpper() == m_targetAddress;
// 		// macOS: address() is always empty, fall back to name matching
// 		if (!matches && info.address().isNull())
// 			matches = !info.name().isEmpty() &&
// 			          info.name().compare(m_targetAddress, Qt::CaseInsensitive) == 0;
// 	} else {
// 		// User entered a device name (useful on macOS where addresses are unavailable)
// 		matches = info.name().compare(m_targetAddress, Qt::CaseInsensitive) == 0;
// 	}

// 	if (matches) {
// 		emitInfo(NecroLog::Level::Info, tr("Found BLE device: %1 (%2)").arg(info.name(), info.address().toString()));
// 		m_discoveryAgent->stop();
// 		createController(info);
// 	}
// }

// void BtSiDeviceDriver::onScanFinished()
// {
// 	// Scan ended without finding the device (it would have been stopped early on match)
// 	if (!m_controller) {
// 		emitInfo(NecroLog::Level::Warning, tr("BLE scan finished — device %1 not found.").arg(m_targetAddress));
// 	}
// }

// void BtSiDeviceDriver::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
// {
// 	Q_UNUSED(error)
// 	emitInfo(NecroLog::Level::Error, tr("BLE scan error: %1").arg(m_discoveryAgent->errorString()));
// }

// ===========================================================================
// BtSiDeviceDriver — GATT controller
// ===========================================================================

void BtSiDeviceDriver::createController(const QBluetoothDeviceInfo &info)
{
	m_controller = QLowEnergyController::createCentral(info, this);

	connect(m_controller, &QLowEnergyController::connected, this, &BtSiDeviceDriver::onControllerConnected);
	connect(m_controller, &QLowEnergyController::disconnected, this, &BtSiDeviceDriver::onControllerDisconnected);
	connect(m_controller, &QLowEnergyController::errorOccurred, this, &BtSiDeviceDriver::onControllerError);
	connect(m_controller, &QLowEnergyController::discoveryFinished, this, &BtSiDeviceDriver::onServiceDiscoveryFinished);

	m_controller->connectToDevice();
}

void BtSiDeviceDriver::onControllerConnected()
{
	emitInfo(NecroLog::Level::Info, tr("Connected, discovering services..."));
	m_controller->discoverServices();
}

void BtSiDeviceDriver::onControllerDisconnected()
{
	emitInfo(NecroLog::Level::Warning, tr("BT SI device disconnected."));
	bool wasConnected = m_connected;
	m_connected = false;
	m_cardStateSubscribed = false;
	m_cardDataSubscribed = false;
	for (auto *svc : m_services)
		svc->deleteLater();
	m_services.clear();
	if (wasConnected)
		emit connectionStateChanged(false);
}

void BtSiDeviceDriver::onControllerError(QLowEnergyController::Error error)
{
	emitInfo(NecroLog::Level::Error,
	         tr("BT SI Reader controller error: %1").arg(m_controller->errorString()));
	if (error != QLowEnergyController::RemoteHostClosedError) {
		bool wasConnected = m_connected;
		m_connected = false;
		if (wasConnected)
			emit connectionStateChanged(false);
	}
}

// ===========================================================================
// BtSiDeviceDriver — service discovery and characteristic subscription
// ===========================================================================

void BtSiDeviceDriver::onServiceDiscoveryFinished()
{
	const auto uuids = m_controller->services();
	m_pendingServices = uuids.count();

	for (const QBluetoothUuid &uuid : uuids) {
		QLowEnergyService *svc = m_controller->createServiceObject(uuid, this);
		if (!svc) {
			--m_pendingServices;
			continue;
		}
		m_services.append(svc);
		connect(svc, &QLowEnergyService::stateChanged, this, &BtSiDeviceDriver::onServiceStateChanged);
		svc->discoverDetails();
	}

	if (m_pendingServices == 0)
		checkAllServicesReady();
}

void BtSiDeviceDriver::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
	if (state == QLowEnergyService::RemoteServiceDiscovered) {
		subscribeCharacteristicsFrom(qobject_cast<QLowEnergyService *>(sender()));
		if (--m_pendingServices == 0)
			checkAllServicesReady();
	} else if (state == QLowEnergyService::InvalidService) {
		if (--m_pendingServices == 0)
			checkAllServicesReady();
	}
}

void BtSiDeviceDriver::subscribeCharacteristicsFrom(QLowEnergyService *svc)
{
	if (!svc)
		return;

	bool connectedSignal = false;
	for (const QLowEnergyCharacteristic &ch : svc->characteristics()) {
		if (ch.uuid() == m_cardStateUuid || ch.uuid() == m_cardDataUuid) {
			if (!connectedSignal) {
				connect(svc, &QLowEnergyService::characteristicChanged,
				        this, &BtSiDeviceDriver::onCharacteristicChanged,
				        Qt::UniqueConnection);
				connectedSignal = true;
			}
			// Enable notifications via Client Characteristic Configuration Descriptor
			const QLowEnergyDescriptor cccd = ch.descriptor(
				QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
			if (cccd.isValid())
				svc->writeDescriptor(cccd, QByteArray::fromHex("0100"));

			if (ch.uuid() == m_cardStateUuid)
				m_cardStateSubscribed = true;
			else
				m_cardDataSubscribed = true;
		}
	}
}

void BtSiDeviceDriver::checkAllServicesReady()
{
	if (m_pendingServices > 0)
		return;

	if (m_cardStateSubscribed && m_cardDataSubscribed) {
		m_connected = true;
		emit connectionStateChanged(true);
		emitInfo(NecroLog::Level::Info, tr("BT SI Reader ready."));
	} else {
		emitInfo(NecroLog::Level::Warning,
		         tr("BT SI Reader: SI characteristics not found on device %1.")
		             .arg(deviceInfo().address().toString()));
	}
}

void BtSiDeviceDriver::onCharacteristicChanged(const QLowEnergyCharacteristic &ch, const QByteArray &value)
{
	if (ch.uuid() == m_cardStateUuid) {
		QByteArray msg = m_cardStateReassembler.feed(value);
		if (!msg.isEmpty())
			handleCardStateMessage(msg);
	} else if (ch.uuid() == m_cardDataUuid) {
		QByteArray msg = m_cardDataReassembler.feed(value);
		if (!msg.isEmpty())
			handleCardDataMessage(msg);
	}
}

// ===========================================================================
// BtSiDeviceDriver — message handlers
// ===========================================================================

void BtSiDeviceDriver::handleCardStateMessage(const QByteArray &message)
{
	// message layout: msg_id(u16le) | payload_len(u16le) | payload
	// payload (7 bytes): card_number(u32le) | state(u8) | code_number(u16le)
	if (message.size() < 4)
		return;

	quint16 msgId = readU16LE(message, 0);
	if (msgId != MSG_CARD_STATE) {
		qfDebug() << "BtSiDeviceDriver: unexpected msgId in cardState notification:"
		          << std::hex << msgId;
		return;
	}

	// payload starts at byte 4 (after 2-byte msg_id and 2-byte payload_len)
	if (message.size() < 4 + 7) {
		emitInfo(NecroLog::Level::Warning,
		         tr("CardState message too short (%1 bytes).").arg(message.size()));
		return;
	}

	auto cardNumber  = readU32LE(message, 4);
	auto state       = static_cast<quint8>(message[8]);
	auto codeNumber  = readU16LE(message, 9);

	m_lastStationNumber = static_cast<int>(codeNumber);

	qfDebug() << "BtSiDeviceDriver: CardState card=" << cardNumber
	          << "state=" << state << "station=" << codeNumber;

	if (state == 0) {
		emitInfo(NecroLog::Level::Info, tr("SI card %1 removed from station %2.").arg(cardNumber).arg(codeNumber));
	} else {
		emitInfo(NecroLog::Level::Info, tr("SI card %1 inserted at station %2.").arg(cardNumber).arg(codeNumber));
	}
}

void BtSiDeviceDriver::handleCardDataMessage(const QByteArray &message)
{
	// message layout: msg_id(u16le) | payload_len(u16le) | payload
	if (message.size() < 4)
		return;

	quint16 msgId = readU16LE(message, 0);
	if (msgId != MSG_CARD_MINIMAL && msgId != MSG_CARD_COMPLETE) {
		qfDebug() << "BtSiDeviceDriver: unexpected msgId in cardData notification:"
		           << std::hex << msgId;
		return;
	}

	// payload starts at byte 4 (after 2-byte msg_id and 2-byte payload_len)
	QByteArray payload = message.mid(4);
	SICard card = buildSICard(payload);

	if (card.cardNumber <= 0) {
		emitInfo(NecroLog::Level::Warning, tr("Received card readout with invalid card number."));
		return;
	}

	card.stationNumber = m_lastStationNumber;

	qfDebug() << "BtSiDeviceDriver: card readout complete for card" << card.cardNumber;
	emitInfo(NecroLog::Level::Info,
	         tr("SI card %1 readout complete (%2 punches).")
	             .arg(card.cardNumber)
	             .arg(card.punchCount()));

	emit siTaskFinished(static_cast<int>(SiTask::Type::CardRead), QVariant(card.toVariantMap()));
}

// ===========================================================================
// BtSiDeviceDriver — SI card building
// ===========================================================================

SICard BtSiDeviceDriver::buildSICard(const QByteArray &payload)
{
	// payload layout:
	//   card_number:  u32le  [0..3]
	//   card_family:  u8     [4]
	//   punch_count:  u16le  [5..6]
	//   punches:      punch_count * 8 bytes starting at [7]
	//     control_info: u8      [+0]
	//     punch_type:   u8      [+1]  1=Clear 2=Check 3=Start 5=Finish 7=Control
	//     control_code: u16le   [+2..3]
	//     time_ms:      u32le   [+4..7]  ms since Sunday 00:00:00.000

	SICard card;

	if (payload.size() < 7)
		return card;

	quint32 cardNumber  = readU32LE(payload, 0);
	// quint8 cardFamily  = static_cast<quint8>(payload[4]);  // unused for now
	quint16 punchCount  = readU16LE(payload, 5);

	card.cardNumber = static_cast<int>(cardNumber);

	// Defaults — mark times as invalid
	card.checkTime    = SICard::INVALID_SI_TIME;
	card.startTime    = SICard::INVALID_SI_TIME;
	card.finishTime   = SICard::INVALID_SI_TIME;
	card.finishTimeMs = 0;

	SICard::PunchList controlPunches;

	const int punchBase = 7;
	for (int i = 0; i < static_cast<int>(punchCount); ++i) {
		const int offset = punchBase + i * 8;
		if (offset + 8 > payload.size())
			break;

		// quint8 controlInfo = static_cast<quint8>(payload[offset]);  // unused
		auto punchType   = static_cast<quint8>(payload[offset + 1]);
		auto controlCode = readU16LE(payload, offset + 2);
		auto timeMs      = readU32LE(payload, offset + 4);

		// Time conversion: ms since Sunday 00:00:00.000
		int todMs      = static_cast<int>(timeMs % 86400000U);
		int t12Ms      = todMs % 43200000;
		int t12Sec     = t12Ms / 1000;
		int msecPart   = static_cast<int>(timeMs % 1000U);
		bool pmFlag    = todMs >= 43200000;
		int dayOfWeek  = static_cast<int>((timeMs / 86400000U) % 7U);

		switch (punchType) {
		case PUNCH_TYPE_CHECK:
			card.checkTime = t12Sec;
			break;

		case PUNCH_TYPE_START:
			card.startTime = t12Sec;
			break;

		case PUNCH_TYPE_FINISH:
			card.finishTime   = t12Sec;
			card.finishTimeMs = msecPart;
			break;

		case PUNCH_TYPE_CONTROL: {
			SIPunch punch;
			punch.code      = static_cast<int>(controlCode);
			punch.time      = t12Sec;
			punch.msec      = msecPart;
			punch.pmFlag    = pmFlag;
			punch.dayOfWeek = dayOfWeek;
			controlPunches.append(punch);
			break;
		}

		case PUNCH_TYPE_CLEAR:
			// Clear punch — no useful data to store
			break;

		default:
			qfDebug() << "BtSiDeviceDriver: unknown punch type" << punchType << "at index" << i;
			break;
		}
	}

	card.punches = controlPunches;
	return card;
}

} // namespace siut
