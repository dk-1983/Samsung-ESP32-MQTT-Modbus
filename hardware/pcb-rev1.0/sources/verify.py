from pathlib import Path
import json,csv
import pcbnew as p
r=Path(__file__).resolve().parents[1]
b=p.LoadBoard(str(r/'samsung-rev1.0.kicad_pcb'))
parts=json.loads((r/'circuit.json').read_text())
expected={ (c['ref'],k):v for c in parts for k,v in c['pins'].items() }
actual={ (f.GetReference(),a.GetNumber()):a.GetNetname() for f in b.GetFootprints() for a in f.Pads() if a.GetNetname() }
assert actual==expected,(actual.items()-expected.items(),expected.items()-actual.items())
# Check semiconductor pin mapping independently against Samsung hardware/nets.json.
base=json.loads((r.parent/'components/nets.json').read_text())
rename={'MAIN_RX_3V3':'H_RX','MAIN_RX_INPUT':'H_TX','FACTORY_TX_5V':'FACTORY_TX','FACTORY_RX_3V3':'F_RX','FACTORY_RX_INPUT':'F_TX','MAX485_RO':'RO_5V','MB_DE':'DIR','PROG_TX':'UART0_TX','PROG_RX':'UART0_RX','RS485_A':'A','RS485_B':'B'}
count=0
for net,pins in base.items():
 for pin in pins:
  ref,num=pin.split('.')
  if ref.startswith('U'):
   assert actual[(ref,num)]==rename.get(net,net),(pin,net,actual[(ref,num)])
   count+=1
with (r/'BOM.csv').open('w',newline='',encoding='utf-8-sig') as out:
 w=csv.writer(out);w.writerow(['Reference','Value','Footprint','MPN'])
 for c in parts:w.writerow([c['ref'],c['value'],c['footprint'],c['mpn']])
with (r/'connections.csv').open('w',newline='',encoding='utf-8-sig') as out:
 w=csv.writer(out);w.writerow(['Reference','Pad','Net'])
 for (ref,num),net in sorted(actual.items()):w.writerow([ref,num,net])
report={'assigned_logical_pins':len(actual),'semiconductor_pins_checked_against_Samsung_schematic':count,'board_mm':[30,48],'status':'all specified nets match','note':'PCB references follow Haier carrier component numbering; Samsung old drawing uses other resistor and capacitor numbers.'}
(r/'review/pin-check.json').write_text(json.dumps(report,indent=2))
print(report)
