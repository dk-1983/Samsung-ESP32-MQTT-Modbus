"""Package only clean CI-built Samsung OTA artifacts, never the local private firmware."""
from pathlib import Path
import hashlib,json,os,re,subprocess
r=Path(__file__).resolve().parents[1]
if os.environ.get('GITHUB_ACTIONS')!='true':raise SystemExit('Public OTA must be built in clean CI')
if (r/'secrets.yaml').read_bytes()!=(r/'release.defaults.yaml').read_bytes():raise SystemExit('Not public defaults')
if subprocess.check_output(['git','status','--porcelain','--untracked-files=no'],cwd=r).strip():raise SystemExit('Modified tracked source')
b=r/'work/build';src=(b/'src/main.cpp').read_text()
if 'set_public_release(true)' not in src:raise SystemExit('Not a public OTA build')
v=re.search(r'SAMSUNG_FIRMWARE_VERSION "([0-9.]+)"',(r/'components/samsung_portal/version.h').read_text())[1]
data=(b/'.pioenvs/samsung-s3/firmware.ota.bin').read_bytes()
if len(data)<128 or len(data)>0x7c0000 or data[0]!=0xe9 or data[12:14]!=bytes([9,0]):raise SystemExit('Wrong app image')
o=r/'work/public-release';o.mkdir(parents=True,exist_ok=False)
(o/f'samsung-s3-n16r8-v{v}-ota.bin').write_bytes(data)
meta={'version':v,'public_ota':True,'target':'ESP32-S3-WROOM-1 N16R8','sha256':hashlib.sha256(data).hexdigest(),'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()}
(o/'build-info.json').write_text(json.dumps(meta,indent=2)+'\n')
print('Prepared public OTA',v)
