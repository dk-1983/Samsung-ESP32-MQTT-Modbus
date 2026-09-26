"""Generate the review-stage component database, KiCad symbols and PCB netlist."""
from pathlib import Path
import csv,json,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
LIB='4vrs_Samsung'
fp=lambda name:LIB+':'+name if name else ''
components=[]
def add(ref,value,symbol,footprint,status='selected',note=''):
 components.append(dict(reference=ref,value=value,symbol=LIB+':'+symbol,footprint=fp(footprint),status=status,note=note))
add('U1','ESP32-S3-WROOM-1-N16R8','ESP32-S3-WROOM-1-N16R8','ESP32-S3-WROOM-1')
add('U2','AMS1117-3.3','AMS1117-3.3','SOT-223-3_TabPin2','proposed_package','SOT-223 proposed; tab is VOUT, pad 2. Confirm regulator package.')
add('U3','MAX485','MAX485','SOIC-8_3.9x4.9mm_P1.27mm',note='SOIC-8 confirmed; exact ordering suffix/manufacturer to select.')
for i in range(1,9):add('R'+str(i),'20k' if i in (2,5,6) else '10k','R','R_0805_2012Metric',note='0805 confirmed. Tolerance/power not specified.')
for i in (1,3,4):add('C'+str(i),'100nF','C','C_0805_2012Metric',note='0805 confirmed. Dielectric and voltage rating to select.')
add('C2','47uF 6.3V','CP','','footprint_pending','Polarized. Body diameter/height and lead or pad spacing required.')
nets={
 '+5V':['U2.3','U3.8','C1.1','C4.1'],
 '+3V3':['U2.2','U1.2','R1.1','C2.1','C3.1'],
 'GND':['U1.1','U1.40','U1.41','U2.1','U3.5','C1.2','C2.2','C3.2','C4.2','R2.2','R5.2','R6.2','R8.2'],
 'EN':['U1.3','R1.2'],
 'MAIN_TX_5V':['R4.1'],
 'MAIN_RX_3V3':['R4.2','R5.1','U1.11'],
 'MAIN_RX_INPUT':['U1.10'],
 'FACTORY_TX_5V':['R3.1'],
 'FACTORY_RX_3V3':['R3.2','R2.1','U1.8'],
 'FACTORY_RX_INPUT':['U1.9'],
 'MAX485_RO':['U3.1','R7.1'],
 'MB_RX':['R7.2','R6.1','U1.12'],
 'MB_TX':['U1.17','U3.4'],
 'MB_DE':['U1.23','U3.2','U3.3','R8.1'],
 'PROG_TX':['U1.37'], 'PROG_RX':['U1.36'],
 'RS485_A':['U3.6'], 'RS485_B':['U3.7'],
}
external=[
 ('+5V','+5V (controller mb); +5V (display CN1), контакт 6'),
 ('GND','GND (controller mb); GND (display CN1), контакт 5'),
 ('MAIN_TX_5V','TX (controller mb): провод 10 → R4 → GPIO18'),
 ('MAIN_RX_INPUT','RX (controller mb): провод 9 ← GPIO17'),
 ('FACTORY_TX_5V','TX (display CN1): контакт 9 → R3 → GPIO15'),
 ('FACTORY_RX_INPUT','RX (display CN1): контакт 10 ← GPIO16'),
 ('PROG_TX','GPIO43 / pad37 → RX программатора (логика 3,3 В)'),
 ('PROG_RX','GPIO44 / pad36 ← TX программатора (логика 3,3 В)'),
 ('GND','GND программатора; общая земля'),
 ('RS485_A','Линия A RS485'),('RS485_B','Линия B RS485'),
]
ROOT.joinpath('components/components.json').write_text(json.dumps(components,indent=2,ensure_ascii=False),encoding='utf-8')
ROOT.joinpath('components/nets.json').write_text(json.dumps(nets,indent=2),encoding='utf-8')
with ROOT.joinpath('components/BOM.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.DictWriter(f,fieldnames=components[0].keys());w.writeheader();w.writerows(components)
with ROOT.joinpath('components/connections.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.writer(f);w.writerow(['net','component','pin']);w.writerows((n,*node.split('.')) for n,nodes in nets.items() for node in nodes)
with ROOT.joinpath('components/external_connections.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.writer(f);w.writerow(['net','external_connection']);w.writerows(external)

# All module pad numbers, including unconnected pins, are explicit in the PCB symbol.
names={1:'GND',2:'3V3',3:'EN',4:'GPIO4',5:'GPIO5',6:'GPIO6',7:'GPIO7',8:'GPIO15',9:'GPIO16',10:'GPIO17',11:'GPIO18',12:'GPIO8',13:'GPIO19',14:'GPIO20',15:'GPIO3',16:'GPIO46',17:'GPIO9',18:'GPIO10',19:'GPIO11',20:'GPIO12',21:'GPIO13',22:'GPIO14',23:'GPIO21',24:'GPIO47',25:'GPIO48',26:'GPIO45',27:'GPIO0',28:'GPIO35_PSRAM',29:'GPIO36_PSRAM',30:'GPIO37_PSRAM',31:'GPIO38',32:'GPIO39',33:'GPIO40',34:'GPIO41',35:'GPIO42',36:'GPIO44_RXD0',37:'GPIO43_TXD0',38:'GPIO2',39:'GPIO1',40:'GND',41:'EP_GND'}
pins={}
pins['ESP32-S3-WROOM-1-N16R8']=[(str(i),names[i],'power_in' if i in (1,2,40,41) else 'input' if i in (3,36) else 'output' if i==37 else 'no_connect' if i in (28,29,30) else 'bidirectional') for i in range(1,42)]
pins['AMS1117-3.3']=[('1','GND','power_in'),('2','VOUT','power_out'),('3','VIN','power_in')]
pins['MAX485']=[('1','RO','output'),('2','~{RE}','input'),('3','DE','input'),('4','DI','input'),('5','GND','power_in'),('6','A','bidirectional'),('7','B','bidirectional'),('8','VCC','power_in')]
pins['R']=[('1','~','passive'),('2','~','passive')];pins['C']=pins['R'];pins['CP']=[('1','+','passive'),('2','-','passive')]
footprints={'ESP32-S3-WROOM-1-N16R8':'ESP32-S3-WROOM-1','AMS1117-3.3':'SOT-223-3_TabPin2','MAX485':'SOIC-8_3.9x4.9mm_P1.27mm','R':'R_0805_2012Metric','C':'C_0805_2012Metric','CP':''}
sources={'ESP32-S3-WROOM-1-N16R8':'https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf','MAX485':'https://www.analog.com/en/products/max485.html','AMS1117-3.3':'https://datasheet.lcsc.com/lcsc/1811142212_Advanced-Monolithic-Systems-AMS1117-3-3_C6186.pdf'}
def q(s):return json.dumps(str(s),ensure_ascii=False)
symbols=['(kicad_symbol_lib (version 20231120) (generator "4vrs")']
for name,pp in pins.items():
 passive=name in ('R','C','CP');half=(len(pp)+1)//2;body_x=12.7 if not passive else 2.54;body_y=max(5.08,half*1.27) if not passive else 2.54
 ref='R' if name=='R' else 'C' if name in ('C','CP') else 'U'
 symbols.append(f'(symbol {q(name)} (pin_names (offset 0.508)) (in_bom yes) (on_board yes)')
 for prop,value,y in [('Reference',ref,body_y+5.08),('Value',name,body_y+2.54),('Footprint',fp(footprints[name]),0),('Datasheet',sources.get(name,''),0)]:
  hide=' hide' if prop in ('Footprint','Datasheet') else ''
  symbols.append(f'(property {q(prop)} {q(value)} (at 0 {y} 0) (effects (font (size 1.27 1.27)){hide}))')
 if name in ('C','CP'):
  symbols.append(f'(symbol {q(name+"_0_1")}')
  for x1,y1,x2,y2 in [(-.635,-2.54,-.635,2.54),(.635,-2.54,.635,2.54),(-2.54,0,-.635,0),(.635,0,2.54,0)]:
   symbols.append(f'(polyline (pts (xy {x1} {y1}) (xy {x2} {y2})) (stroke (width 0.254) (type default)) (fill (type none)))')
  if name=='CP':symbols.append('(text "+" (at -2.54 3.81 0) (effects (font (size 1.27 1.27))))')
  symbols.append(')')
 else:
  symbols.append(f'(symbol {q(name+"_0_1")} (rectangle (start {-body_x} {body_y if not passive else 1.016}) (end {body_x} {-body_y if not passive else -1.016}) (stroke (width 0) (type default)) (fill (type background))))')
 symbols.append(f'(symbol {q(name+"_1_1")}')
 for index,(num,label,kind) in enumerate(pp):
  left=index<half;x=-(body_x+2.54) if left else body_x+2.54;y=0 if passive else (half-1)*1.27-(index if left else index-half)*2.54
  symbols.append(f'(pin {kind} line (at {x} {y} {0 if left else 180}) (length 2.54) (name {q(label)} (effects (font (size 1.016 1.016)))) (number {q(num)} (effects (font (size 1.016 1.016)))))')
 symbols.append('))')
symbols.append(')')
ROOT.joinpath('kicad/4vrs_Samsung.kicad_sym').write_text('\n'.join(symbols),encoding='utf-8')
ROOT.joinpath('kicad/sym-lib-table').write_text('(sym_lib_table\n (lib (name "4vrs_Samsung")(type "KiCad")(uri "${KIPRJMOD}/4vrs_Samsung.kicad_sym")(options "")(descr "Samsung bridge review components"))\n)\n',encoding='utf-8')
ROOT.joinpath('kicad/fp-lib-table').write_text('(fp_lib_table\n (lib (name "4vrs_Samsung")(type "KiCad")(uri "${KIPRJMOD}/4vrs_Samsung.pretty")(options "")(descr "Vendor footprints; see footprint_sources.json"))\n)\n',encoding='utf-8')
# KiCad XML netlist. External connector footprints are deliberately not invented.
root=ET.Element('export',version='E');design=ET.SubElement(root,'design')
for key,val in [('source','Samsung_transparency_bridge_revA'),('date','2026-09-26'),('tool','4VRS hardware review generator')]:ET.SubElement(design,key).text=val
cc=ET.SubElement(root,'components')
for c in components:
 e=ET.SubElement(cc,'comp',ref=c['reference']);ET.SubElement(e,'value').text=c['value'];ET.SubElement(e,'footprint').text=c['footprint']
 ET.SubElement(e,'libsource',lib=LIB,part=c['symbol'].split(':')[1],description=c['note'])
ll=ET.SubElement(root,'libparts')
for name,pp in pins.items():
 part=ET.SubElement(ll,'libpart',lib=LIB,part=name);sub=ET.SubElement(part,'pins')
 for num,label,kind in pp:ET.SubElement(sub,'pin',num=num,name=label,type=kind)
nn=ET.SubElement(root,'nets')
for code,(name,nodes) in enumerate(nets.items(),1):
 net=ET.SubElement(nn,'net',code=str(code),name=name)
 for node in nodes:ref,pin=node.split('.');ET.SubElement(net,'node',ref=ref,pin=pin)
ET.indent(root);ET.ElementTree(root).write(ROOT/'kicad/Samsung_bridge_review.net',encoding='utf-8',xml_declaration=True)
# Structural checks, not an ERC/DRC substitute.
refs={c['reference']:c for c in components};assigned=set()
for net,nodes in nets.items():
 for node in nodes:
  assert node not in assigned,('pin on two nets',node);assigned.add(node);ref,pin=node.split('.')
  assert ref in refs and pin in {x[0] for x in pins[refs[ref]['symbol'].split(':')[1]]},node
assert nets['EN']==['U1.3','R1.2'] and 'C3.1' in nets['+3V3']
assert len(components)==15
print(f'Created {len(components)} BOM rows, {len(nets)} nets and {len(pins)} KiCad symbols; net consistency passed')
