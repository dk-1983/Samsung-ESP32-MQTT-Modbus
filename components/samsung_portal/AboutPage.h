// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
static const char ABOUT_PAGE[] PROGMEM=R"HTML(<!doctype html><html lang="ru"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Samsung · О системе</title>__STYLE__</head><body data-page="about">__NAV__<main><h1>О системе</h1><p class="muted">Версия, соединения и память контроллера</p><p id="message" class="status" role="status">Получаем данные…</p><section class="card"><dl id="info"></dl></section><details style="margin-top:16px"><summary>Техническая диагностика</summary><pre id="raw"></pre></details><section class="card" style="margin-top:16px"><h2>Перезагрузка контроллера</h2><p>Настройки сохранятся. Связь с веб-пультом, MQTT и Modbus временно прервётся. После запуска UART включится автоматически.</p><button id="restart" disabled>Перезагрузить</button></section><section class="card" style="margin-top:16px"><h2>О проекте</h2><p>4vrs Samsung-ESP32-MQTT-Modbus — локальное управление кондиционером Samsung через UART, MQTT и Modbus. Облако Samsung не требуется.</p><p><a href="https://github.com/dk-1983/Samsung-ESP32-MQTT-Modbus" target="_blank" rel="noopener">Исходники и документация</a></p></section></main><footer>4vrs · Samsung-ESP32 · ESP32-S3</footer><script>
const token='__TOKEN__',el=id=>document.getElementById(id);
let last=null,waiting=false,oldBoot=null,deadline=0,inflight=false;
const kb=n=>(n/1024).toFixed(1)+' КБ',yes=v=>v?'Включён':'Выключен';
function render(s){
 const rows=[['Прошивка',s.version],['Контроллер',s.controller],['Адрес',s.ip],['MAC Wi-Fi',s.mac],['Имя',s.hostname],['Время работы',Math.floor(s.uptime_s/86400)+' д '+Math.floor(s.uptime_s%86400/3600)+' ч '+Math.floor(s.uptime_s%3600/60)+' мин'],['Wi-Fi',s.wifi?'Подключён':s.ap?'Точка настройки':'Нет связи'],['Кондиционер',s.ac_fresh?'Свежие данные':'Нет свежего ответа'],['UART',yes(s.uart)],['MQTT',s.mqtt_enabled?(s.mqtt_connected?'Подключён':'Нет подключения'):'Выключен'],['Modbus RTU',yes(s.rtu)],['Modbus TCP',yes(s.tcp)],['Свободная RAM',kb(s.free_heap)],['Минимум RAM за сеанс',kb(s.min_heap)],['Крупнейший свободный блок',kb(s.max_block)],['PSRAM',kb(s.free_psram)+' свободно из '+kb(s.psram_size)],['Flash',(s.flash_size/1048576).toFixed(0)+' МБ'],['Хранилище настроек',s.storage_ok?'Инициализировано':'Ошибка'],['Обновление',s.update_busy?'Выполняется':'Не выполняется']];
 el('info').replaceChildren();for(const [k,v] of rows){const dt=document.createElement('dt'),dd=document.createElement('dd');dt.textContent=k;dd.textContent=v;el('info').append(dt,dd)}
 el('raw').textContent=JSON.stringify(s,null,2);
 el('restart').disabled=waiting||s.update_busy||s.restarting;
}
async function refresh(){
 if(inflight||document.hidden&&!waiting)return;inflight=true;
 try{const r=await fetch('/system/status',{cache:'no-store',signal:AbortSignal.timeout(4000)});if(!r.ok)throw Error('HTTP '+r.status);const s=await r.json();if(!s.version||typeof s.boot_id!=='number')throw Error('Некорректный ответ');last=s;render(s);
  if(waiting&&s.boot_id!==oldBoot){location.reload();return}
  if(!waiting)el('message').textContent='Контроллер на связи · данные обновлены';
 }catch(e){if(!waiting){el('message').textContent='Нет актуальных данных контроллера. '+e.message;el('restart').disabled=true}}
 finally{inflight=false;if(waiting&&Date.now()>deadline){waiting=false;el('message').textContent='Пока не удалось подтвердить перезапуск. Проверьте сеть и обновите страницу.'}}
}
el('restart').onclick=async()=>{
 if(!last||waiting||!confirm('Перезагрузить контроллер? Настройки сохранятся, связь временно прервётся.'))return;
 oldBoot=last.boot_id;waiting=true;deadline=Date.now()+90000;el('restart').disabled=true;
 try{const r=await fetch('/system/restart',{method:'POST',body:new URLSearchParams({token,confirm:'RESTART'}),signal:AbortSignal.timeout(5000)});if(!r.ok){waiting=false;throw Error(await r.text())}el('message').textContent='Перезагрузка запрошена. Ожидаем восстановления связи…'}
 catch(e){el('message').textContent=waiting?'Ответ не получен. Проверяем, перезапустился ли контроллер…':'Перезагрузка не выполнена: '+e.message}
};
refresh();setInterval(refresh,3000);
</script></body></html>)HTML";
