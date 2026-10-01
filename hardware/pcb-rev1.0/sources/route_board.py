"""Initial two-layer routing candidate. Must pass KiCad DRC before release."""
from pathlib import Path
import heapq, math, time
import pcbnew as p

out=Path(__file__).parent.parent
b=p.LoadBoard(str(out/'samsung-placement.kicad_pcb'))
step=.1; x0=.8;y0=7.4;nx=285;ny=399
def pos(x,y):return p.VECTOR2I(p.FromMM(x),p.FromMM(y))
def cell(x,y,l=0):return (round((x-x0)/step),round((y-y0)/step),l)
def xy(c):return x0+c[0]*step,y0+c[1]*step
def idx(c):return c[2]*nx*ny+c[1]*nx+c[0]
def valid(c):return 0<=c[0]<nx and 0<=c[1]<ny
pads=[];groups={};routes=[]
for f in sorted(b.GetFootprints(),key=lambda f:f.GetReference()):
 for a in f.Pads():
  if not a.IsOnLayer(p.F_Cu) and not a.IsOnLayer(p.B_Cu):continue
  box=a.GetBoundingBox();r=[p.ToMM(box.GetLeft()),p.ToMM(box.GetTop()),p.ToMM(box.GetRight()),p.ToMM(box.GetBottom())]
  layers=[l for l,layer in enumerate([p.F_Cu,p.B_Cu]) if a.IsOnLayer(layer)]
  net=a.GetNetname();center=(p.ToMM(a.GetPosition().x),p.ToMM(a.GetPosition().y))
  pads.append((r,layers,net))
  if net:groups.setdefault(net,[]).append((center,layers))
def blockrect(grid,r,layers):
 xa=max(0,math.floor((r[0]-x0)/step));xb=min(nx-1,math.ceil((r[2]-x0)/step))
 ya=max(0,math.floor((r[1]-y0)/step));yb=min(ny-1,math.ceil((r[3]-y0)/step))
 for l in layers:
  for y in range(ya,yb+1):
   for x in range(xa,xb+1):grid[l*nx*ny+y*nx+x]=1
def gridfor(net,width):
 g=bytearray(2*nx*ny);margin=.21+width/2
 for r,layers,n in pads:
  if n!=net:blockrect(g,[r[0]-margin,r[1]-margin,r[2]+margin,r[3]+margin],layers)
 for a,z,l,n,w in routes:
  if n==net:continue
  rad=margin+w/2;count=max(1,math.ceil(math.dist(a,z)/(.1)))
  for i in range(count+1):
   t=i/count;x=a[0]+(z[0]-a[0])*t;y=a[1]+(z[1]-a[1])*t
   blockrect(g,[x-rad,y-rad,x+rad,y+rad],[l])
 return g
def astar(start,ends,g):
 goals=set(ends);end=next(iter(goals))
 def h(c):return (abs(c[0]-end[0])+abs(c[1]-end[1]))*10
 q=[];cost={};parent={}
 for s in start:
  if not valid(s) or g[idx(s)]:continue
  cost[s]=0;heapq.heappush(q,(h(s),0,s))
 while q:
  _,v,c=heapq.heappop(q)
  if cost.get(c)!=v:continue
  if c in goals:
   path=[c]
   while c in parent:c=parent[c];path.append(c)
   return path[::-1]
  for dx,dy,dl,weight in [(1,0,0,10),(-1,0,0,10),(0,1,0,10),(0,-1,0,10),(0,0,1,85)]:
   z=(c[0]+dx,c[1]+dy,1-c[2] if dl else c[2])
   if not valid(z) or g[idx(z)]:continue
   if dl:
    if any(.01 < math.dist(xy(z),v) < .6 for v in via_points):continue
    # Via diameter 0.6: extra safety beyond track clearance envelope.
    if any(not valid((z[0]+i,z[1]+j,l)) or g[idx((z[0]+i,z[1]+j,l))] for l in [0,1] for i in [-2,-1,0,1,2] for j in [-2,-1,0,1,2]):continue
   nv=v+weight
   if nv<cost.get(z,1e20):cost[z]=nv;parent[z]=c;heapq.heappush(q,(nv+h(z),nv,z))
 return None
def track(a,z,l,net,width):
 if math.dist(a,z)<.001:return
 t=p.PCB_TRACK(b);t.SetStart(pos(*a));t.SetEnd(pos(*z));t.SetLayer([p.F_Cu,p.B_Cu][l]);t.SetWidth(p.FromMM(width));t.SetNetCode(b.GetNetcodeFromNetname(net));b.Add(t);routes.append((a,z,l,net,width))
via_points=[]
failed=[]
# Local control signals first, then supplies and return.
order=sorted(groups,key=lambda n:(0 if n=='EN' else 1 if n=='+3V3' else 2 if n=='H_RX' else 4 if n=='GND' else 3,len(groups[n]),n))
for net in order:
 nodes=groups[net];width=.4 if net in ['GND','+5V','+3V3'] else .25
 connected=[nodes[0]];remaining=nodes[1:]
 while remaining:
  _,i,j=min((math.dist(a[0],z[0]),i,j) for i,a in enumerate(connected) for j,z in enumerate(remaining))
  a=connected[i];z=remaining.pop(j);g=gridfor(net,width)
  starts=[cell(*a[0],l) for l in a[1]];ends=[cell(*z[0],l) for l in z[1]]
  path=astar(starts,ends,g)
  if path is None:failed.append((net,a,z));connected.append(z);continue
  track(a[0],xy(path[0]),path[0][2],net,width)
  # Merge collinear runs to produce editable, short segment lists.
  run=path[0];previous=run;direction=None
  for nxt in path[1:]:
   d=(nxt[0]-previous[0],nxt[1]-previous[1],nxt[2]-previous[2])
   if d[2]:
    track(xy(run),xy(previous),previous[2],net,width)
    v=p.PCB_VIA(b);v.SetPosition(pos(*xy(previous)));v.SetWidth(p.FromMM(.6));v.SetDrill(p.FromMM(.3));v.SetLayerPair(p.F_Cu,p.B_Cu);v.SetNetCode(b.GetNetcodeFromNetname(net));b.Add(v) if not any(math.dist(xy(previous),q)<.01 for q in via_points) else None
    via_points.append(xy(previous))
    for l in [0,1]:routes.append((xy(previous),xy(previous),l,net,.6))
    run=nxt;direction=None
   elif direction!=d:
    track(xy(run),xy(previous),previous[2],net,width);run=previous;direction=d
   previous=nxt
  track(xy(run),xy(path[-1]),path[-1][2],net,width)
  track(xy(path[-1]),z[0],path[-1][2],net,width)
  connected.append(z)
 print(net,'completed; failures so far',len(failed),flush=True)
p.SaveBoard(str(out/'samsung-routed.kicad_pcb'),b)
(out/'routing-pending.txt').write_text('\n'.join(map(str,failed)),encoding='utf-8')
print('Unrouted pairs',len(failed),'tracks/vias',len(list(b.GetTracks())),flush=True)
