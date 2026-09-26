"""Save passive sniffer chunks as JSONL; never sends UART commands."""
import argparse,json,time
from pathlib import Path
import requests,yaml
p=argparse.ArgumentParser();p.add_argument('--host',default='10.0.0.15');p.add_argument('--secrets',required=True);p.add_argument('--seconds',type=int,default=60);p.add_argument('--output',required=True);p.add_argument('--allow-bridge',action='store_true');a=p.parse_args()
c=yaml.safe_load(Path(a.secrets).read_text(encoding='utf-8'));s=requests.Session();s.trust_env=False;s.auth=('admin',c['web_password'])
end=time.monotonic()+a.seconds;cursor=0;boot=None;totals={18:0,17:0};gaps=0;last={}
with Path(a.output).open('w',encoding='utf-8') as f:
 while time.monotonic()<end:
  try:
   resp=s.get('http://'+a.host+'/capture',params={'after':cursor},timeout=5);resp.raise_for_status();j=resp.json();last=j
   if not j.get('passive') and not (a.allow_bridge and j.get('bridge')):raise RuntimeError('Not a passive sniffer; bridge capture requires --allow-bridge')
   if boot!=j['boot_id']:
    f.write(json.dumps({'event':'boot','boot_id':j['boot_id'],'host_time':time.time()})+'\n');boot=j['boot_id'];cursor=0
    if j['last_seq'] and not j['chunks']:continue
   if j['oldest_seq']>cursor+1:
    lost=j['oldest_seq']-cursor-1;gaps+=lost;f.write(json.dumps({'event':'ring_gap','chunks':lost})+'\n')
   for chunk in j['chunks']:
    if chunk['seq']<=cursor:continue
    chunk['boot_id']=boot;chunk['host_time']=time.time();f.write(json.dumps(chunk)+'\n');totals.setdefault(chunk['gpio'],0);totals[chunk['gpio']]+=len(bytes.fromhex(chunk['hex']));cursor=chunk['seq']
   f.flush()
  except requests.RequestException as e:
   f.write(json.dumps({'event':'http_error','type':type(e).__name__})+'\n')
  time.sleep(1)
print(json.dumps({'saved_bytes':totals,'ring_gap_chunks':gaps,'last_seq':last.get('last_seq'),'rx18_bytes':last.get('rx18_bytes'),'rx17_bytes':last.get('rx17_bytes'),'output':a.output}))
