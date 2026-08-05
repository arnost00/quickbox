#pragma once

#include <QBluetoothAddress>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QVariantMap>

namespace CardReader {

/// Serialize a QBluetoothDeviceInfo to a QVariantMap suitable for QSettings storage.
inline QVariantMap btDeviceInfoToMap(const QBluetoothDeviceInfo &info)
{
	QVariantMap m;
	m[QStringLiteral("name")] = info.name();
	m[QStringLiteral("address")] = info.address().toString();
	m[QStringLiteral("deviceUuid")] = info.deviceUuid().toString();
	m[QStringLiteral("coreConfigurations")] = static_cast<int>(info.coreConfigurations());
	return m;
}

/// Reconstruct a QBluetoothDeviceInfo from a QVariantMap produced by btDeviceInfoToMap().
/// Returns an invalid QBluetoothDeviceInfo if the map is empty or lacks both address and UUID.
inline QBluetoothDeviceInfo btDeviceInfoFromMap(const QVariantMap &m)
{
	if (m.isEmpty())
		return {};
	const QString name = m.value(QStringLiteral("name")).toString();
	const QString address = m.value(QStringLiteral("address")).toString();
	const QString uuid = m.value(QStringLiteral("deviceUuid")).toString();
	const auto configs = QBluetoothDeviceInfo::CoreConfigurations(m.value(QStringLiteral("coreConfigurations")).toInt());

	QBluetoothDeviceInfo info;
	if (!address.isEmpty()) {
		info = QBluetoothDeviceInfo(QBluetoothAddress(address), name, 0);
	} else if (!uuid.isEmpty()) {
		info = QBluetoothDeviceInfo(QBluetoothUuid(uuid), name, 0);
	}
	if (info.isValid() && configs) {
		info.setCoreConfigurations(configs);
	};
	return info;
}

} // namespace CardReader
