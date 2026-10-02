// Exercise the real C++-rendered pages against simulated API responses only.
const {chromium} = require('playwright');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '../..');
const output = path.join(root, 'work/ui-check');
const pages = JSON.parse(fs.readFileSync(path.join(output, 'fixtures.json'), 'utf8'));
const base = 'http://samsung-ui.test';
fs.mkdirSync(path.join(output, 'screenshots'), {recursive: true});

async function main() {
 const browser = await chromium.launch({headless: true, ...(process.env.UI_BROWSER_CHANNEL ? {channel: process.env.UI_BROWSER_CHANNEL} : {})});
 const errors = [], posts = [];
 let status = {ac_fresh:true,can_command:true,room:23,target:24,power:0,mode:4,fan:1,swing:0,preset:0,command_result:'idle',uart:true,mqtt_enabled:true,mqtt_connected:true};
 let failure = false;
 const replies = {
  '/mqtt/config':{enabled:true,discovery:true,connected:true,host:'mqtt.test',port:1883,username:'test',prefix:'samsung-s3',password_set:true},
  '/modbus/config':{rtu:false,tcp:true,unit:1,baud:9600},
  '/settings/config':{credentials_ready:true},
  '/wifi/status':{connected:true,ssid:'Test Wi-Fi',ip:'192.0.2.10',mac:'00:00:00:00:00:01',rssi:-50,channel:6,ap_active:false,ap_ssid:'Samsung-Setup'},
  '/system/status':{version:'1.0.2',boot_id:1,controller:'ESP32-S3',ip:'192.0.2.10',mac:'00:00:00:00:00:01',hostname:'samsung-s3',uptime_s:90000,wifi:true,ac_fresh:true,uart:true,mqtt_enabled:true,mqtt_connected:true,rtu:false,tcp:true,free_heap:180000,min_heap:150000,max_block:100000,free_psram:7000000,psram_size:8388608,flash_size:16777216,storage_ok:true,update_busy:false,restarting:false},
  '/updates/status':{installed:'1.0.2',available:'',phase:'idle',error:'',download_percent:0,busy:false,web_enabled:true,effective_enabled:true}
 };
 try {
  for (const language of ['en','ru']) {
   const context = await browser.newContext();
   if (language === 'ru') await context.addCookies([{name:'samsung_ui_lang',value:'ru',url:base}]);
   await context.route('**/*', async route => {
    const request = route.request(), url = new URL(request.url());
    assert.equal(url.origin, base, 'No device or external network access is allowed');
    const name = url.pathname;
    if (request.method() === 'POST') {
     const data = Object.fromEntries(new URLSearchParams(request.postData()));
     assert.equal(data.token, 'UI_TEST_TOKEN');
     posts.push({path:name,data});
     return route.fulfill({status:200,contentType:'application/json',body:JSON.stringify(name==='/settings/config'?{changed:true,restarting:true}:{accepted:true,confirmed:false})});
    }
    if (pages[name]) {
     const ru = /(?:^|;\s*)samsung_ui_lang=ru(?:;|$)/.test(request.headers().cookie || '');
     return route.fulfill({status:200,contentType:'text/html; charset=utf-8',headers:{'Content-Language':ru?'ru':'en','Cache-Control':'no-store'},body:pages[name][ru?'ru':'en']});
    }
    if (name === '/control/status') {
     if (failure) return route.fulfill({status:503,body:'Simulated outage'});
     return route.fulfill({status:200,contentType:'application/json',body:JSON.stringify(status)});
    }
    if (replies[name]) return route.fulfill({status:200,contentType:'application/json',body:JSON.stringify(replies[name])});
    throw Error('Unexpected request: '+request.method()+' '+name);
   });
   const page = await context.newPage();
   page.on('pageerror', error => errors.push(error.message));
   page.on('dialog', dialog => dialog.accept());
   for (const width of [1280,390]) {
    await page.setViewportSize({width,height:900});
    for (const route of Object.keys(pages)) {
     await page.goto(base+route);
     await page.waitForLoadState('networkidle');
     assert.equal(await page.locator('html').getAttribute('lang'),language);
     assert.equal(await page.locator('#ui-language').inputValue(),language);
     assert(await page.locator('#ui-language').isVisible());
     const header = await page.locator('nav').evaluate(nav => ({
      links: [...nav.querySelectorAll('a')].map(el => ({y:el.getBoundingClientRect().y,height:el.getBoundingClientRect().height})),
      picker: {y:nav.querySelector('select').getBoundingClientRect().y,height:nav.querySelector('select').getBoundingClientRect().height}
     }));
     assert(header.links.every(link=>link.height<=41),`${language} header buttons stretched at ${width}px`);
     if(width===1280) assert(header.links.every(link=>Math.abs(link.y-header.picker.y)<1),`${language} language picker wrapped on desktop: ${JSON.stringify(header)}`);
     if(route==='/control') await page.locator('nav').screenshot({path:path.join(output,'screenshots',`${language}-header-${width}.png`)});

     assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1),`${language} ${route} overflows at ${width}px`);
     if(language==='en') {
      const text=await page.locator('body').evaluate(body=>{const clone=body.cloneNode(true);clone.querySelectorAll('script,style,select#ui-language').forEach(el=>el.remove());return clone.textContent;});
      assert(!/[А-Яа-яЁё]/.test(text),route+' has untranslated visible text');
     }
     if(route==='/control'||route==='/settings') await page.screenshot({path:path.join(output,'screenshots',`${language}-${route.slice(1)}-${width}.png`),fullPage:true});
    }
   }
   // Switching reloads the current page, persists across links, and never posts.
   const before=posts.length, other=language==='en'?'ru':'en';
   await page.goto(base+'/control');
   await Promise.all([page.waitForEvent('load'),page.locator('#ui-language').selectOption(other)]);
   assert.equal(await page.locator('html').getAttribute('lang'),other);
   const cookie=(await context.cookies()).find(c=>c.name==='samsung_ui_lang');
   assert.equal(cookie.path,'/');assert.equal(cookie.sameSite,'Lax');assert(cookie.expires>Date.now()/1000+300*86400);
   await page.locator('nav a[href="/mqtt"]').click();
   assert.equal(await page.locator('html').getAttribute('lang'),other);
   await Promise.all([page.waitForEvent('load'),page.locator('#ui-language').selectOption(language)]);
   assert.equal(posts.length,before,'Language switching must not change device settings');

   // Controls: preserve wire values and never display requested state as confirmed state.
   await page.goto(base+'/control');
   await page.locator('[data-power="1"]:enabled').waitFor();
   await page.locator('[data-power="1"]').click();
   await page.waitForFunction(()=>document.getElementById('result').textContent.includes('принята')||document.getElementById('result').textContent.includes('accepted'));
   assert.equal(posts.at(-1).path,'/control/set');assert.equal(posts.at(-1).data.field,'power');assert.equal(posts.at(-1).data.value,'1');
   assert.equal(await page.locator('#power').textContent(),language==='en'?'Off':'Выключен');
   for (const [field,value] of [['mode','1'],['fan','5'],['swing','3'],['preset','2'],['target','25']]) {
    const form=page.locator(`form[data-field="${field}"]`);
    if(field==='target')await form.locator('input').fill(value);else await form.locator('select').selectOption(value);
    const count=posts.length;await form.locator('button').click();await page.waitForLoadState('networkidle');
    assert.equal(posts.length,count+1);assert.equal(posts.at(-1).data.field,field);assert.equal(posts.at(-1).data.value,value);
   }
   status={...status,ac_fresh:false,can_command:false,power:null,room:null,target:null,mode:null};
   await page.reload();await page.waitForLoadState('networkidle');
   assert(await page.locator('[data-power="1"]').isDisabled());assert.equal(await page.locator('#room').textContent(),'—');
   failure=true;await page.reload();await page.waitForLoadState('networkidle');
   assert(await page.locator('[data-power="1"]').isDisabled());assert.equal(await page.locator('#power').textContent(),'—');
   failure=false;status={...status,ac_fresh:true,can_command:true,power:0,room:23,target:24,mode:4};

   await page.goto(base+'/mqtt');await page.locator('#save:enabled').waitFor();
   await page.locator('#host').fill('broker.test');await page.locator('#save').click();await page.waitForLoadState('networkidle');
   assert.equal(posts.at(-1).data.host,'broker.test');assert.equal(posts.at(-1).data.password,'');assert.equal(posts.at(-1).data.clear_password,'0');
   await page.goto(base+'/modbus');await page.locator('#save:enabled').waitFor();
   await page.locator('#unit').fill('2');await page.locator('#baud').selectOption('19200');await page.locator('#save').click();await page.waitForLoadState('networkidle');
   assert.equal(posts.at(-1).data.unit,'2');assert.equal(posts.at(-1).data.baud,'19200');
   await page.goto(base+'/settings');await page.locator('#save:enabled').waitFor();
   const unchanged=posts.length;await page.locator('#save').click();assert.equal(posts.length,unchanged);
   await page.locator('[name=web_password]').fill('ExamplePassword123');await page.locator('[name=web_confirm]').fill('DifferentPassword123');
   await page.locator('#save').click();assert.equal(posts.length,unchanged);
   await page.locator('[name=web_confirm]').fill('ExamplePassword123');await page.locator('#save').click();await page.waitForLoadState('networkidle');
   assert.equal(posts.at(-1).data.web_password,'ExamplePassword123');assert.equal(posts.at(-1).data.ota_password,'');
   await page.goto(base+'/wifi/reset');assert(await page.locator('#reset').isDisabled());
   await page.locator('#ack').check();await page.locator('#reset').click();await page.waitForLoadState('networkidle');
   assert.equal(posts.at(-1).data.confirm,'RESET_WIFI');
   await page.goto(base+'/about');await page.locator('#restart:enabled').waitFor();await page.locator('#restart').click();await page.waitForLoadState('networkidle');
   assert.equal(posts.at(-1).data.confirm,'RESTART');
   await page.goto(base+'/updates');await page.waitForLoadState('networkidle');assert(await page.locator('#install').isDisabled());
   await page.locator('#check').click();await page.waitForLoadState('networkidle');assert.equal(posts.at(-1).path,'/updates/check');
   await context.clearCookies();await page.reload();assert.equal(await page.locator('html').getAttribute('lang'),'en');
   await context.close();console.log('PASS',language,': 9 pages at desktop/mobile widths, cookie persistence, form/API contracts, stale/offline state, confirmations');
  }
  assert.deepEqual(errors,[],'Browser runtime errors');
  fs.writeFileSync(path.join(output,'browser-results.json'),JSON.stringify({status:'PASS',pages:9,languages:['en','ru'],widths:[1280,390],mockPosts:posts.length,browser:browser.version(),errors},null,2));
 } finally {await browser.close();}
}
main().catch(error=>{console.error(error);process.exitCode=1;});
