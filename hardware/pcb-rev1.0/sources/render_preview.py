from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json
r=Path(__file__).resolve().parents[1];d=json.loads((r/'review/geometry.json').read_text())
im=Image.new('RGB',(1450,1210),'#f0f3f7');dr=ImageDraw.Draw(im)
def font(n):return ImageFont.truetype('C:/Windows/Fonts/arial.ttf',n)
dr.text((50,22),'4VRS / SAMSUNG UART BRIDGE / rev1.0',font=font(30),fill='#162c40')
dr.text((50,67),'30 x 48 mm  |  Two copper layers  |  Review, not a photomask',font=font(21),fill='#42566a')
for bottom,ox in [(False,55),(True,795)]:
 def pos(q):return (ox+(30-q[0] if bottom else q[0])*20,175+q[1]*20)
 def box(a,z,**kw):
  x,y=pos(a);xx,yy=pos(z);dr.rectangle((min(x,xx),min(y,yy),max(x,xx),max(y,yy)),**kw)
 dr.text((ox,124),'BOTTOM / viewed from below' if bottom else 'TOP / component side',font=font(23),fill='#162c40')
 box((0,0),(30,48),fill='#183e35',outline='#142c26',width=3)
 layer='B.Cu' if bottom else 'F.Cu'
 for zone in d['zones']:
  if zone['layer']!=layer:continue
  for poly in zone['polygons']:
   dr.polygon([pos(q) for q in poly['outer']],fill='#396858' if zone['net']=='GND' else '#527d70')
   for hole in poly['holes']:dr.polygon([pos(q) for q in hole],fill='#183e35')
 for t in d['tracks']:
  if t['layer']==layer:dr.line([pos(t['a']),pos(t['b'])],fill='#7aa89b',width=max(1,round(t['width']*20)))
 for v in d['vias']:
  x,y=pos(v['pos']);a=v['width']*10;dr.ellipse((x-a,y-a,x+a,y+a),fill='#bba06c');a=v['drill']*10;dr.ellipse((x-a,y-a,x+a,y+a),fill='#10221d')
 for f in d['footprints']:
  for pad in f['pads']:
   if not pad['drill'][0] and f['bottom']!=bottom:continue
   x,y=pos(pad['pos']);sx,sy=pad['size'][0]*10,pad['size'][1]*10
   dr.rectangle((x-sx,y-sy,x+sx,y+sy),fill='#d5b877')
   if pad['drill'][0]:
    a=pad['drill'][0]*10;dr.ellipse((x-a,y-a,x+a,y+a),fill='#10221d')
 for label in d['labels']:
  if label['layer']!=('B.Silkscreen' if bottom else 'F.Silkscreen'):continue
  x,y=pos(label['pos']);dr.text((x,y),label['text'],font=font(max(12,round(label['size']*20))),fill='white',anchor='mm')
 if not bottom:
  box((6,.5),(24,27),outline='#eeeecc',width=2)
  dr.text(pos((15,3)),'ANTENNA',font=font(18),fill='white',anchor='mm')
  dr.text(pos((15,19)),'ESP32-S3',font=font(18),fill='white',anchor='mm')
dr.text((50,1162),'UART labels TX / RX refer to the connected Samsung board. All grounds are common.',font=font(20),fill='#42566a')
im.save(r/'review/samsung-both-sides.png')
