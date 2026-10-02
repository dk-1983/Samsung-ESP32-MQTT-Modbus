"""Check localization contracts and render real Portal::send_page_ on the host."""
from pathlib import Path
import importlib.util
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / 'components/samsung_portal'
OUT = ROOT / 'work/ui-check'
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('ui_generator', ROOT / 'tools/build_ui_translations.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


def templates(text):
    return dict(re.findall(r'static const char (\w+)\[\].*?R"HTML\((.*?)\)HTML";', text, re.S))


assert (UI / 'UiEnglish.h').read_text(encoding='utf-8') == generator.generate()
english = templates((UI / 'UiEnglish.h').read_text(encoding='utf-8'))
original = {}
for path in UI.glob('*Page.h'):
    original.update(templates(path.read_text(encoding='utf-8')))

routes = {}
for path in UI.glob('*.cpp'):
    routes.update(re.findall(r'web_\.on\("([^"]+)",HTTP_GET,\[this\]\(\)\{if\(test_auth_\(\)\)send_page_\((\w+)\);\}\);', path.read_text(encoding='utf-8')))
portal = (UI / 'samsung_portal.cpp').read_text(encoding='utf-8')
english_routes = dict(re.findall(r'if\(path=="([^"]+)"\)localized=(\w+);', portal))
assert set(routes.values()) == set(original), 'Every source page must be reachable'
assert english_routes == {route: name + '_EN' for route, name in routes.items()}, 'Missing English route'
assert 'web_.collectHeaders(headers,1)' in portal and 'const char *headers[]={"Cookie"}' in portal

for name, ru in original.items():
    en = english[name + '_EN']
    assert not re.search('[А-Яа-яЁё]', en), name
    for attr in ('id', 'name', 'value', 'href', 'min', 'max', 'minlength', 'maxlength', 'type'):
        pattern = r'\b' + attr + r'="([^"]*)"'
        assert re.findall(pattern, ru) == re.findall(pattern, en), (name, attr)
    assert '<html lang="ru">' in ru and '<html lang="en">' in en

# A missing translation, an empty translation and duplicate keys must fail closed.
with tempfile.TemporaryDirectory(dir=OUT) as temp:
    test_ui = Path(temp)
    (test_ui / 'i18n').mkdir()
    (test_ui / 'UiShell.h').write_text('', encoding='utf-8')
    (test_ui / 'TestPage.h').write_text('static const char TEST[] =R"HTML(Непереведено)HTML";', encoding='utf-8')
    try:
        generator.UI = test_ui
        for catalog in ('Привет\tHello\n', 'Привет\t\n', 'Привет\tHello\nПривет\tHi\n', ''):
            (test_ui / 'i18n/en.tsv').write_text(catalog, encoding='utf-8')
            try:
                generator.generate()
            except ValueError:
                pass
            else:
                raise AssertionError('Invalid/incomplete translation catalog accepted')
    finally:
        generator.UI = UI

# Compile the real renderer body, rather than reimplementing its language selection.
start = portal.index('void Portal::send_page_(')
end = portal.index('bool Portal::save_(', start)
includes = ['UiShell.h', 'UiEnglish.h', 'ui_language.h'] + sorted(p.name for p in UI.glob('*Page.h'))
source = '#include "tests/ui_host.h"\n'
source += '\n'.join(f'#include "components/samsung_portal/{name}"' for name in includes)
source += '\nnamespace esphome::samsung_portal {\n' + portal[start:end] + '\n}\n'
source += 'int main(int argc,char **argv){if(argc!=3)return 2;esphome::samsung_portal::Portal p;p.web_.path=argv[1];p.web_.cookie=argv[2];const char *page=nullptr;\n'
source += '\n'.join(f'if(p.web_.path=="{route}")page={name};' for route, name in routes.items())
source += '\nif(!page)return 3;p.send_page_(page);std::cout<<p.web_.headers["Content-Language"]<<"\\n"<<p.web_.headers["Cache-Control"]<<"\\n"<<p.web_.headers["Vary"]<<"\\n"<<p.web_.body;return 0;}\n'
cpp = OUT / 'renderer.cpp'
binary = OUT / ('renderer.exe' if os.name == 'nt' else 'renderer')
cpp.write_text(source, encoding='utf-8')
os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(ROOT / 'work/zig-global-cache')
os.environ['ZIG_LOCAL_CACHE_DIR'] = str(ROOT / 'work/zig-local-cache')
subprocess.run([sys.executable, '-m', 'ziglang', 'c++', '-std=c++17', '-Wall', '-Wextra', '-I', str(ROOT), str(cpp), '-o', str(binary)], check=True)
fixtures = {}
for route in routes:
    fixtures[route] = {}
    for cookie, lang in [('', 'en'), ('samsung_ui_lang=en', 'en'), ('samsung_ui_lang=ru', 'ru'), ('samsung_ui_lang=de', 'en'), ('other_samsung_ui_lang=ru', 'en')]:
        response = subprocess.check_output([str(binary), route, cookie]).decode('utf-8').replace('\r\n', '\n')
        content_lang, cache, vary, html = response.split('\n', 3)
        assert content_lang == lang and cache == 'no-store' and vary == 'Cookie'
        assert f'<html lang="{lang}">' in html
        assert '__TOKEN__' not in html and '__NAV__' not in html and '__STYLE__' not in html
        assert f'value="{lang}" selected' in html and 'id="ui-language"' in html
        assert "Max-Age=31536000; Path=/; SameSite=Lax" in html
        fixtures[route][lang] = html
    print('PASS real C++ page renderer:', route, '(default, EN, RU, invalid/unrelated cookies)')
(OUT / 'fixtures.json').write_text(json.dumps(fixtures, ensure_ascii=False), encoding='utf-8')

version = re.search(r'SAMSUNG_FIRMWARE_VERSION "([\d.]+)"', (UI / 'version.h').read_text())[1]
assert re.search(r'^    version: ' + re.escape(version) + r'$', (ROOT / 'samsung-s3.yaml').read_text(), re.M)
for name in ('README.md', 'README_RU.md'):
    assert f'# Samsung-ESP32-MQTT-Modbus — {version}' in (ROOT / name).read_text(encoding='utf-8')
print('UI generator, form contracts, language routes and firmware/document versions: OK')
