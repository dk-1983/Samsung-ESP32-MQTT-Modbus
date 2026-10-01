from pathlib import Path
import os, json
import pcbnew as p
from circuit import parts

out=Path(__file__).parent.parent
libroot=Path(os.environ['LOCALAPPDATA'])/'Programs/KiCad/10.0/share/kicad/footprints'
b=p.BOARD()
b.GetDesignSettings().SetCopperLayerCount(2)
b.GetDesignSettings().SetBoardThickness(p.FromMM(1.0))
def pt(x,y): return p.VECTOR2I(p.FromMM(x),p.FromMM(y))
def line(a,z,layer,width=.1):
 s=p.PCB_SHAPE();s.SetShape(p.SHAPE_T_SEGMENT);s.SetStart(pt(*a));s.SetEnd(pt(*z));s.SetLayer(layer);s.SetWidth(p.FromMM(width));b.Add(s)
for a,z in [((0,0),(30,0)),((30,0),(30,48)),((30,48),(0,48)),((0,48),(0,0))]:line(a,z,p.Edge_Cuts)
nets={}
for name in sorted({n for c in parts for n in c['pins'].values()}):
 n=p.NETINFO_ITEM(b,name);b.Add(n);nets[name]=n
locations={
'U1':(15,13.75,0),'U2':(8,33,0),'U3':(23,33,0),
'R1':(2.5,11,90),'R2':(2.5,21.5,90),'R3':(2.5,25.5,90),'R8':(2.5,17.5,90),'R9':(3,28.5,0),
'R4':(17,32,90),'R5':(17,28,90),'R6':(19.5,38,0),'R7':(25.5,40,0),
'C1':(5,40,0),'C2':(2.5,33,90),'C3':(13,40,0),'C4':(12.5,34,90),
'C5':(2.5,8,0),'C6':(27.5,37,90),'C7':(2.5,14,0),'C8':(14,28.5,0),
'SB1':(21,23,0),'SB2':(10,23,0),'JP1':(25,43,0),
'TP1':(27.5,10,0),'TP2':(27.5,14,0),'TP3':(27.5,18,0),
'TP4':(8,10,0),'TP5':(12,10,0),'TP6':(16,10,0),
'TP7':(28,45.5,0),'TP8':(24.5,45.5,0),
'TP9':(2,45.5,0),'TP10':(4.8,45.5,0),'TP11':(7.6,45.5,0),'TP12':(10.4,45.5,0),
'TP13':(13,45.5,0),'TP14':(15.8,45.5,0),'TP15':(18.6,45.5,0),'TP16':(21.4,45.5,0)}
placed={}
for c in parts:
 if not c['footprint']:continue
 lib,name=c['footprint'].split(':')
 f=p.FootprintLoad(str(libroot/(lib+'.pretty')),name)
 if not f:raise RuntimeError(c['footprint'])
 f.SetReference(c['ref']);f.SetValue(c['value'])
 x,y,angle=locations[c['ref']];f.SetPosition(pt(x,y));f.SetOrientationDegrees(angle)
 if c['ref'] in ['SB1','SB2','R5','R9','C4','C6','C8','TP4','TP5','TP6']:
  f.SetParent(b);f.Flip(pt(x,y),False)
 if c['ref'].startswith('TP'):
  for g in list(f.GraphicalItems()):
   if g.GetLayer() in [p.F_SilkS,p.B_SilkS,p.F_CrtYd,p.B_CrtYd]:g.SetLayer(p.Dwgs_User)
 for pad in f.Pads():
  if pad.GetNumber() in c['pins']:pad.SetNet(nets[c['pins'][pad.GetNumber()]])
 f.Reference().SetTextSize(pt(.8,.8));f.Reference().SetTextThickness(p.FromMM(.12))
 f.Reference().SetVisible(False);f.Reference().SetLayer(p.B_SilkS);f.Reference().SetMirrored(True);f.Reference().SetPosition(pt(x,y))
 f.Value().SetVisible(False)
 b.Add(f);placed[c['ref']]=f
 if c['ref']=='U1':
  for g in list(f.GraphicalItems()):
   if g.GetLayer()==p.F_SilkS and g.GetBoundingBox().GetTop()<p.FromMM(0.2): f.Remove(g)
  for pad in f.Pads():
   if pad.GetNumber()=='41' and pad.GetDrillSize().x:pad.SetDrillSize(pt(.3,.3))
def text(value,x,y,layer=p.F_SilkS,size=.8):
 t=p.PCB_TEXT(b);t.SetText(value);t.SetPosition(pt(x,y));t.SetLayer(layer);t.SetTextSize(pt(size,size));t.SetTextThickness(p.FromMM(.15));t.SetMirrored(layer==p.B_SilkS);b.Add(t)
text('SAMSUNG',15,15,p.B_SilkS,1.2)
text('UART BRIDGE',15,17.5,p.B_SilkS,.9)
text('Designed by 4vrs',15,40,p.B_SilkS,.9)
text('rev1.0',15,42,p.B_SilkS,.9)
for label,x in [('EN',8),('BOOT',12),('3V3',16)]:text(label,x,12,p.B_SilkS,.8)
for label,x,y in [('MAIN',6,43),('DISPLAY',17,43),('A',28,43.5),('B',24.5,47.3),('TX0',27.5,8),('RX0',27.5,12),('GND',27.5,16)]:text(label,x,y,size=.8)
for x,label in [(2,'5V'),(4.8,'G'),(7.6,'TX'),(10.4,'RX'),(13,'5V'),(15.8,'G'),(18.6,'TX'),(21.4,'RX')]:text(label,x,47.3,size=.8)
p.SaveBoard(str(out/'samsung-placement.kicad_pcb'),b)
(out/'circuit.json').write_text(json.dumps(parts,indent=2),encoding='utf-8')
print('Placed',len(placed),'footprints')

