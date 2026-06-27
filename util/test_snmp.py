import time
from pysnmp.hlapi import *

AUTH = UsmUserData('snmpv3admin',
                   authKey='authpassphrase01', privKey='privpassphrase01',
                   authProtocol=usmHMACSHAAuthProtocol,
                   privProtocol=usmAesCfb128Protocol)
TARGET = UdpTransportTarget(('10.149.130.75', 161))

while True:
    for (errInd, errSt, errIdx, varBinds) in getCmd(
        SnmpEngine(), AUTH, TARGET, ContextData(),
        ObjectType(ObjectIdentity('1.3.6.1.4.1.9999.1.1.0')),
        ObjectType(ObjectIdentity('1.3.6.1.4.1.9999.1.4.0')),
        ObjectType(ObjectIdentity('1.3.6.1.4.1.9999.1.6.0')),
        ObjectType(ObjectIdentity('1.3.6.1.4.1.9999.1.7.0')),
        ObjectType(ObjectIdentity('1.3.6.1.4.1.9999.1.8.0')),
    ):
        if errInd or errSt:
            print(f"Error: {errInd or errSt}")
        else:
            v = {str(var[0]): var[1].prettyPrint() for var in varBinds}
            ts = time.strftime('%H:%M:%S')
            print(f"[{ts}]  "
                  f"Температура: {int(v['1.3.6.1.4.1.9999.1.1.0'])/10:.1f}°C  "
                  f"Heap: {v['1.3.6.1.4.1.9999.1.4.0']}%  "
                  f"Протечка: {'ТРЕВОГА' if v['1.3.6.1.4.1.9999.1.6.0'] != '0' else 'ОК'}  "
                  f"Дверь1: {'ОТКРЫТА' if v['1.3.6.1.4.1.9999.1.7.0'] != '0' else 'закрыта'}  "
                  f"Дверь2: {'ОТКРЫТА' if v['1.3.6.1.4.1.9999.1.8.0'] != '0' else 'закрыта'}")
    time.sleep(2)
