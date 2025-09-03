<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="es">
<context>
    <name>SICard</name>
    <message>
        <location filename="src/sicard.cpp" line="42"/>
        <source>stationNumber: %1</source>
        <translation>Número de base: %1</translation>
    </message>
    <message>
        <location filename="src/sicard.cpp" line="43"/>
        <source>cardNumber: %1</source>
        <translation>Número de pinza: %1</translation>
    </message>
    <message>
        <location filename="src/sicard.cpp" line="47"/>
        <source>check: %1</source>
        <translation>Comprobación: %1</translation>
    </message>
    <message>
        <location filename="src/sicard.cpp" line="48"/>
        <source>start: %1</source>
        <translation>Salida: %1</translation>
    </message>
    <message>
        <location filename="src/sicard.cpp" line="49"/>
        <source>finish: %1</source>
        <translation>Meta: %1</translation>
    </message>
</context>
<context>
    <name>SiStationConfig</name>
    <message>
        <location filename="src/device/sitask.cpp" line="98"/>
        <source>Station number: {{StationNumber}}
Extended mode: {{ExtendedMode}}
Auto send: {{AutoSend}}
Handshake: {{HandShake}}
Password access: {{PasswordAccess}}
Read out after punch: {{ReadOutAfterPunch}}
</source>
        <translation>Número de estación: {{StationNumber}}
Protocolo extendido: {{ExtendedMode}}
Auto enviar: {{AutoSend}}
Handshake: {{HandShake}}
Contraseña: {{PasswordAccess}}
Lectura después de picar: {{ReadOutAfterPunch}}
</translation>
    </message>
    <message>
        <location filename="src/device/sitask.cpp" line="107"/>
        <location filename="src/device/sitask.cpp" line="108"/>
        <location filename="src/device/sitask.cpp" line="109"/>
        <location filename="src/device/sitask.cpp" line="110"/>
        <location filename="src/device/sitask.cpp" line="111"/>
        <source>True</source>
        <translation>Verdadero</translation>
    </message>
    <message>
        <location filename="src/device/sitask.cpp" line="107"/>
        <location filename="src/device/sitask.cpp" line="108"/>
        <location filename="src/device/sitask.cpp" line="109"/>
        <location filename="src/device/sitask.cpp" line="110"/>
        <location filename="src/device/sitask.cpp" line="111"/>
        <source>False</source>
        <translation>Falso</translation>
    </message>
</context>
<context>
    <name>siut::CommPort</name>
    <message>
        <location filename="src/device/commport.cpp" line="42"/>
        <source>Available ports: %1</source>
        <translation>Puertos disponibles: %1</translation>
    </message>
    <message>
        <location filename="src/device/commport.cpp" line="50"/>
        <source>Connecting to %1 - baudrate: %2, data bits: %3, parity: %4, stop bits: %5</source>
        <translation>Conectándose a %1 - ratio de baudios: %2, bits de datos: %3, paridad: %4, bit de parada: %5</translation>
    </message>
    <message>
        <location filename="src/device/commport.cpp" line="60"/>
        <source>%1 connected OK</source>
        <translation>%1 se conectó correctamente</translation>
    </message>
    <message>
        <location filename="src/device/commport.cpp" line="64"/>
        <source>%1 connect ERROR: %2</source>
        <translation>%1 error de conexión: %2</translation>
    </message>
    <message>
        <location filename="src/device/commport.cpp" line="74"/>
        <source>%1 closed</source>
        <translation>%1 cerrado</translation>
    </message>
    <message>
        <location filename="src/device/commport.cpp" line="91"/>
        <source>possible solution:
Wait at least 10 seconds and then try again.</source>
        <translation>Posible solución:
Espere al menos 10 segundos y vuelva a intentarlo.</translation>
    </message>
    <message>
        <location filename="src/device/commport.cpp" line="100"/>
        <source>There are no ports available.</source>
        <translation>No hay puerto disponibles.</translation>
    </message>
    <message>
        <location filename="src/device/commport.cpp" line="103"/>
        <source>Selected port %1 is not available.
List of accessible ports:

</source>
        <translation>El puerto seleccionado (%1) no se encuentra disponible.
Lista de puertos disponibles:

</translation>
    </message>
</context>
<context>
    <name>siut::DeviceDriver</name>
    <message>
        <location filename="src/device/sidevicedriver.cpp" line="124"/>
        <source>Garbage received, stripping %1 characters from beginning of buffer</source>
        <translation>Datos corruptos recibidos, eliminado %1 caracteres del principio del buffer</translation>
    </message>
    <message>
        <location filename="src/device/sidevicedriver.cpp" line="141"/>
        <source>NAK received</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="src/device/sidevicedriver.cpp" line="147"/>
        <source>Legacy protocol is not supported, switch station to extended one.</source>
        <translation>Protocolo legacy no soportado, cambie la base al protocolo extendido.</translation>
    </message>
    <message>
        <location filename="src/device/sidevicedriver.cpp" line="154"/>
        <source>Valid message shall end with ETX or NAK, throwing data away</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="src/device/sidevicedriver.cpp" line="170"/>
        <source>SIDeviceDriver::sendCommand() - ERROR Sending of EXT commands only is supported for sending.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>siut::SiTask</name>
    <message>
        <location filename="src/device/sitask.cpp" line="24"/>
        <source>SiCommand timeout after %1 sec.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>
