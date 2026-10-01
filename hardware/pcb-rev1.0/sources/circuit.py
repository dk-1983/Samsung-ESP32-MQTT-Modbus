"""Electrical source for PCB v1.1. Net names refer to ESP pins unless HAIER is explicit."""
from pathlib import Path
import json

parts = []
def add(ref, value, symbol, footprint, pins, mpn='', note=''):
    parts.append(dict(ref=ref, value=value, symbol=symbol, footprint=footprint,
                      pins={str(k):v for k,v in pins.items()}, mpn=mpn, note=note))

add('U1','ESP32-S3-WROOM-1-N16R8','RF_Module:ESP32-S3-WROOM-1',
    'RF_Module:ESP32-S3-WROOM-1',
    {1:'GND',2:'+3V3',3:'EN',8:'F_RX',9:'F_TX',10:'H_TX',11:'H_RX',12:'MB_RX',17:'MB_TX',
     23:'DIR',27:'BOOT',36:'UART0_RX',37:'UART0_TX',40:'GND',41:'GND'},
    'ESP32-S3-WROOM-1-N16R8')
add('U2','LM1117-3.3','Regulator_Linear:LM1117MP-3.3','Package_TO_SOT_SMD:SOT-223-3_TabPin2',
    {1:'GND',2:'+3V3',3:'+5V'},'LM1117MP-3.3', 'TAB is +3V3, not GND')
add('U3','MAX485','Interface_UART:MAX485E','Package_SO:SOIC-8_3.9x4.9mm_P1.27mm',
    {1:'RO_5V',2:'DIR',3:'DIR',4:'MB_TX',5:'GND',6:'A',7:'B',8:'+5V'},'MAX485ESA+')
for ref,value,pins in [
    ('R1','10k',{1:'+3V3',2:'EN'}),
    ('R2','10k',{1:'MAIN_TX_5V',2:'H_RX'}),('R3','20k',{1:'H_RX',2:'GND'}),
    ('R4','10k',{1:'RO_5V',2:'MB_RX'}),('R5','20k',{1:'MB_RX',2:'GND'}),
    ('R8','10k',{1:'FACTORY_TX',2:'F_RX'}),('R9','20k',{1:'F_RX',2:'GND'}),
    ('R6','10k',{1:'DIR',2:'GND'}),('R7','120',{1:'A',2:'TERM'})]:
    add(ref,value,'Device:R','Resistor_SMD:R_0805_2012Metric',pins)
for ref,value,net,mpn in [('C1','220u 10V','+5V','TAJC227M010RNJ'),('C3','100u 10V','+3V3','TAJC107M010RNJ')]:
    add(ref,value,'Device:C_Polarized','Capacitor_Tantalum_SMD:CP_EIA-6032-28_Kemet-C',
        {1:net,2:'GND'},mpn,'AVX case C; verify land pattern against AVX, not merely case name')
for ref,value,net in [('C2','100n 50V','+5V'),('C4','100n 50V','+3V3'),
                       ('C5','100n 50V','+3V3'),('C6','100n 50V','+5V'),
                       ('C7','1u 16V','EN'),('C8','10u 10V','+3V3')]:
    add(ref,value,'Device:C','Capacitor_SMD:C_0805_2012Metric',{1:net,2:'GND'})
for ref,net in [('SB1','BOOT'),('SB2','EN')]:
    add(ref,ref,'Switch:SW_Push','Button_Switch_SMD:SW_SPST_TL3342',{1:net,2:'GND'},
        note='Low-profile switch candidate; mechanical drawing to verify')
add('JP1','TERM ENABLE','Jumper:SolderJumper_2_Open',
    'Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm',{1:'TERM',2:'B'})
# Solder pads: TX/RX labels refer to the external Samsung board.
for i,net in enumerate(['UART0_TX','UART0_RX','GND','EN','BOOT','+3V3','A','B','+5V','GND','MAIN_TX_5V','H_TX','+5V','GND','FACTORY_TX','F_TX'],1):
    add(f'TP{i}',net,'Connector:TestPoint','TestPoint:TestPoint_Pad_D2.0mm',{1:net})
