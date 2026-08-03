#pragma once

//
// BtSiDeviceDriver — Bluetooth LE SI reader device driver (Qt Bluetooth)
//
// Copyright: See COPYING file that comes with this distribution
//

#include "../sicard.h"
#include "../siutglobal.h"

#include <necrolog.h>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QObject>
#include <QVariantMap>

namespace siut {

// ---------------------------------------------------------------------------
// BtSiReassembler
//
// Reassembles multi-chunk BLE wrapper messages (msgId == 0xA101).
// Non-wrapper chunks are returned immediately as-is.
// ---------------------------------------------------------------------------
class BtSiReassembler
{
public:
	BtSiReassembler() = default;

	/// Feed one BLE notification chunk.
	/// Returns the complete reassembled message when done, or an empty
	/// QByteArray when more chunks are still needed.
	QByteArray feed(const QByteArray &chunk);

	void reset();

private:
	static constexpr quint16 WRAPPER_MSG_ID = 0xA101;

	QByteArray m_buf;
	int m_expectedLen = 0;
	bool m_started = false;
};

// ---------------------------------------------------------------------------
// BtSiDeviceDriver
// ---------------------------------------------------------------------------
class SIUT_DECL_EXPORT BtSiDeviceDriver : public QObject
{
	Q_OBJECT
	using Super = QObject;

public:
	explicit BtSiDeviceDriver(QObject *parent = nullptr);
	~BtSiDeviceDriver() override;

	void connectToDevice(const QString &address);
	void disconnectFromDevice();
	bool isConnected() const;

	Q_SIGNAL void driverInfo(NecroLog::Level level, const QString &msg);
	Q_SIGNAL void siTaskFinished(int task_type, QVariant result);
	Q_SIGNAL void connectionStateChanged(bool connected);

private Q_SLOTS:
	void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
	void onScanFinished();
	void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);
	void onControllerConnected();
	void onControllerDisconnected();
	void onControllerError(QLowEnergyController::Error error);
	void onServiceDiscoveryFinished();
	void onServiceStateChanged(QLowEnergyService::ServiceState state);
	void onCharacteristicChanged(const QLowEnergyCharacteristic &ch, const QByteArray &value);

private:
	void startScan();
	void createController(const QBluetoothDeviceInfo &info);
	void subscribeCharacteristicsFrom(QLowEnergyService *svc);
	void checkAllServicesReady();

	// Message handlers
	void handleCardStateMessage(const QByteArray &message);
	void handleCardDataMessage(const QByteArray &message);

	// SI card building
	SICard buildSICard(const QByteArray &payload);

	// Utility
	static quint16 readU16LE(const QByteArray &ba, int offset);
	static quint32 readU32LE(const QByteArray &ba, int offset);

	void emitInfo(NecroLog::Level level, const QString &msg);

	// Target device
	QString m_targetAddress;

	// Qt Bluetooth objects
	QBluetoothDeviceDiscoveryAgent	*m_discoveryAgent = nullptr;
	QLowEnergyController		*m_controller = nullptr;
	QList<QLowEnergyService *>	m_services;

	// Known characteristic UUIDs
	QBluetoothUuid	m_cardStateUuid;
	QBluetoothUuid	m_cardDataUuid;

	// State
	bool	m_connected = false;
	bool	m_cardStateSubscribed = false;
	bool	m_cardDataSubscribed = false;
	int	m_pendingServices = 0;
	int	m_lastStationNumber = 0;

	// Reassemblers — one per characteristic
	BtSiReassembler m_cardStateReassembler;
	BtSiReassembler m_cardDataReassembler;
};

} // namespace siut
