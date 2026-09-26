from pathlib import Path
import os
import subprocess
import sys
root = Path(__file__).resolve().parents[1]
out = root / 'work'
out.mkdir(exist_ok=True)
os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global-cache')
os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local-cache')
for name in ('test_protocol', 'test_bridge', 'test_inline'):
    binary = out / (name + ('.exe' if os.name == 'nt' else ''))
    log_path = out / (name + '-compile.log')
    with log_path.open('w', encoding='utf-8') as log:
        result = subprocess.run([sys.executable, '-m', 'ziglang', 'c++', '-std=c++17', '-Wall', '-Wextra',
                                 '-I', str(root), str(root / ('tests/' + name + '.cpp')), '-o', str(binary)],
                                stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print('\n'.join(log_path.read_text(encoding='utf-8', errors='replace').splitlines()[-60:]))
        sys.exit(result.returncode)
    subprocess.run([str(binary)], check=True)
