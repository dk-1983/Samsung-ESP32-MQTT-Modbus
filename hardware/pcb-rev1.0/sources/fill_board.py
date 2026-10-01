from pathlib import Path
import pcbnew as p
out=Path(__file__).parent.parent
b=p.LoadBoard(str(out/'samsung-routed.kicad_pcb'))
def pt(x,y):return p.VECTOR2I(p.FromMM(x),p.FromMM(y))
def zone(net,layer,rect,priority):
 z=p.ZONE(b);z.SetLayer(layer);z.SetNetCode(b.GetNetcodeFromNetname(net));z.SetLocalClearance(p.FromMM(.25));z.SetPadConnection(p.ZONE_CONNECTION_FULL);z.SetAssignedPriority(priority);z.SetThermalReliefGap(p.FromMM(.25));z.SetThermalReliefSpokeWidth(p.FromMM(.3));z.SetMinThickness(p.FromMM(.2));z.SetIslandRemovalMode(p.ISLAND_REMOVAL_MODE_ALWAYS)
 q=z.Outline();q.NewOutline()
 x,y,w,h=rect
 for a,c in [(x,y),(x+w,y),(x+w,y+h),(x,y+h)]:q.Append(p.FromMM(a),p.FromMM(c))
 b.Add(z)

for layer in [p.F_Cu,p.B_Cu]:
 zone('GND',layer,(.5,7.5,29,40),0)
 zone('+3V3',layer,(3.6,30,9,6.4),2)
p.ZONE_FILLER(b).Fill(b.Zones())
for net in ['GND','+3V3']:
 zs=[z for z in b.Zones() if z.GetNetname()==net]
 for ix in range(28):
  for iy in range(39):
   x=1.5+ix;y=8+iy
   if any((v.GetPosition()-pt(x,y)).EuclideanNorm()<p.FromMM(.8) for v in b.GetTracks() if isinstance(v,p.PCB_VIA)):continue
   if any(a.GetBoundingBox().Contains(pt(x,y)) for f in b.GetFootprints() for a in f.Pads()):continue
   if all(any(z.GetLayer()==layer and all(z.HitTestFilledArea(layer,pt(x+dx,y+dy)) for dx,dy in [(0,0),(.45,0),(-.45,0),(0,.45),(0,-.45)]) for z in zs) for layer in [p.F_Cu,p.B_Cu]):
    v=p.PCB_VIA(b);v.SetPosition(pt(x,y));v.SetWidth(p.FromMM(.6));v.SetDrill(p.FromMM(.3));v.SetLayerPair(p.F_Cu,p.B_Cu);v.SetNetCode(b.GetNetcodeFromNetname(net));b.Add(v)
p.ZONE_FILLER(b).Fill(b.Zones())
# Stitch small grounded copper fragments that the coarse grid missed.
for z in list(b.Zones()):
 polys=z.GetFilledPolysList(z.GetLayer())
 for island in range(polys.OutlineCount()):
  box=polys.Outline(island).BBox();x0=p.ToMM(box.GetX());y0=p.ToMM(box.GetY());w=p.ToMM(box.GetWidth());h=p.ToMM(box.GetHeight())
  candidates=[]
  for ix in range(int(w/.25)+1):
   if candidates:break
   for iy in range(int(h/.25)+1):
    if candidates:break
    x=x0+ix*.25;y=y0+iy*.25
    if not polys.Outline(island).PointInside(pt(x,y)):continue
    if any((v.GetPosition()-pt(x,y)).EuclideanNorm()<p.FromMM(.7) for v in b.GetTracks() if isinstance(v,p.PCB_VIA)):continue
    if all(any(q.GetLayer()==layer and q.GetNetname()==z.GetNetname() and all(q.HitTestFilledArea(layer,pt(x+dx,y+dy)) for dx,dy in [(0,0),(.4,0),(-.4,0),(0,.4),(0,-.4)]) for q in b.Zones()) for layer in [p.F_Cu,p.B_Cu]):
     inpad=any(a.GetBoundingBox().Contains(pt(x,y)) for f in b.GetFootprints() for a in f.Pads())
     candidates.append((inpad,(x-(x0+w/2))**2+(y-(y0+h/2))**2,x,y))
  if candidates:
   _,_,x,y=min(candidates);v=p.PCB_VIA(b);v.SetPosition(pt(x,y));v.SetWidth(p.FromMM(.6));v.SetDrill(p.FromMM(.3));v.SetLayerPair(p.F_Cu,p.B_Cu);v.SetNetCode(b.GetNetcodeFromNetname(z.GetNetname()));b.Add(v);print('GND island stitch',island,round(x,3),round(y,3))

p.ZONE_FILLER(b).Fill(b.Zones())
p.SaveBoard(str(out/'samsung-rev1.0.kicad_pcb'),b)
