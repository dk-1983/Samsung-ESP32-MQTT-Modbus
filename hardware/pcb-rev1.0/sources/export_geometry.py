from pathlib import Path
import json
import pcbnew as p
r=Path(__file__).resolve().parents[1];b=p.LoadBoard(str(r/'samsung-rev1.0.kicad_pcb'))
def xy(a):return[p.ToMM(a.x),p.ToMM(a.y)]
d={'footprints':[],'tracks':[],'holes':[],'width':30,'height':48,'zones':[],'vias':[]}
for f in b.GetFootprints():
 item={'ref':f.GetReference(),'value':f.GetValue(),'pos':xy(f.GetPosition()),'label':xy(f.Reference().GetPosition()),'bottom':f.GetLayer()==p.B_Cu,'pads':[]}
 box=f.GetBoundingBox(False,False);item['bbox']=[p.ToMM(box.GetLeft()),p.ToMM(box.GetTop()),p.ToMM(box.GetRight()),p.ToMM(box.GetBottom())]
 for a in f.Pads():
  q={'number':a.GetNumber(),'pos':xy(a.GetPosition()),'size':xy(a.GetSize()),'drill':xy(a.GetDrillSize()),'net':a.GetNetname(),'shape':str(a.GetShape()),'npth':a.GetAttribute()==p.PAD_ATTRIB_NPTH}
  if round(a.GetOrientationDegrees())%180==90:q['size']=q['size'][::-1]
  item['pads'].append(q)
 d['footprints'].append(item)
for t in b.GetTracks():
 if isinstance(t,p.PCB_VIA):d['vias'].append({'pos':xy(t.GetPosition()),'width':p.ToMM(t.GetWidth(p.F_Cu)),'drill':p.ToMM(t.GetDrillValue()),'net':t.GetNetname()})
 else:d['tracks'].append({'a':xy(t.GetStart()),'b':xy(t.GetEnd()),'width':p.ToMM(t.GetWidth()),'net':t.GetNetname(),'layer':t.GetLayerName()})
for z in b.Zones():
 q=z.GetFilledPolysList(z.GetLayer());item={'net':z.GetNetname(),'layer':b.GetLayerName(z.GetLayer()),'polygons':[]}
 def points(chain):return [xy(chain.CPoint(i)) for i in range(chain.PointCount())]
 for i in range(q.OutlineCount()):item['polygons'].append({'outer':points(q.COutline(i)),'holes':[points(q.CHole(i,j)) for j in range(q.HoleCount(i))]})
 d['zones'].append(item)
d['labels']=[{'text':t.GetText(),'pos':xy(t.GetPosition()),'layer':b.GetLayerName(t.GetLayer()),'size':p.ToMM(t.GetTextSize().y)} for t in b.Drawings() if isinstance(t,p.PCB_TEXT)]
(r/'review/geometry.json').write_text(json.dumps(d,indent=2),encoding='utf-8')
assert {t['layer'] for t in d['tracks']}=={'F.Cu','B.Cu'}
