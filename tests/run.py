from pathlib import Path
import os
import subprocess
import sys
root = Path(__file__).resolve().parents[1]
out = root / 'work'
out.mkdir(exist_ok=True)
os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global-cache')
os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local-cache')
binary = out / ('test_protocol.exe' if os.name == 'nt' else 'test_protocol')
log_path = out / 'host-compile.log'
with log_path.open('w', encoding='utf-8') as log:
    result = subprocess.run([sys.executable, '-m', 'ziglang', 'c++', '-std=c++17', '-Wall', '-Wextra',
                             '-I', str(root), str(root / 'tests/test_protocol.cpp'), '-o', str(binary)],
                            stdout=log, stderr=subprocess.STDOUT)
if result.returncode:
    print('\n'.join(log_path.read_text(encoding='utf-8', errors='replace').splitlines()[-60:]))
    sys.exit(result.returncode)
subprocess.run([str(binary)], check=True)
