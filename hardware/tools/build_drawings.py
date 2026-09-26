from pathlib import Path
import csv,json,html
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.units import mm
ROOT=Path(__file__).resolve().parents[1]
pdfmetrics.registerFont(TTFont('Arial','C:/Windows/Fonts/arial.ttf'))
pdfmetrics.registerFont(TTFont('ArialB','C:/Windows/Fonts/arialbd.ttf'))
W,H=420,297
pdf=canvas.Canvas(str(ROOT/'drawings/Samsung_transparency_bridge_revA-review.pdf'),pagesize=(W*mm,H*mm))
pdf.setTitle('Samsung-ESP32-MQTT-Modbus | Electrical schematic | Rev A')
pdf.setAuthor('4VRS / Dmitriy')
svg=[]
def line(x,y,x2,y2,color='#17212b',width=.32,dash=False):
 pdf.setStrokeColor(color);pdf.setLineWidth(width*mm);pdf.setDash(2*mm,1*mm) if dash else pdf.setDash()
 pdf.line(x*mm,(H-y)*mm,x2*mm,(H-y2)*mm)
 svg.append(f'<line x1="{x}" y1="{y}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{width}"'+(' stroke-dasharray="2 1"' if dash else '')+'/>')
def text(x,y,s,size=3,bold=False,anchor='start',color='#17212b'):
 pdf.setFillColor(color);pdf.setFont('ArialB' if bold else 'Arial',size*mm)
 {'start':pdf.drawString,'middle':pdf.drawCentredString,'end':pdf.drawRightString}[anchor](x*mm,(H-y)*mm,s)
 svg.append(f'<text x="{x}" y="{y}" font-family="Arial, sans-serif" font-size="{size}" font-weight="{700 if bold else 400}" text-anchor="{anchor}" fill="{color}">{html.escape(s)}</text>')
def box(x,y,w,h,color='#17212b',fill=None):
 pdf.setStrokeColor(color);pdf.setLineWidth(.3*mm);pdf.setDash();pdf.setFillColor(fill or '#ffffff');pdf.rect(x*mm,(H-y-h)*mm,w*mm,h*mm,stroke=1,fill=bool(fill))
 svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill or "none"}" stroke="{color}" stroke-width="0.3"/>')
def dot(x,y):
 pdf.setFillColor('#17212b');pdf.circle(x*mm,(H-y)*mm,.65*mm,stroke=0,fill=1)
 svg.append(f'<circle cx="{x}" cy="{y}" r=".65" fill="#17212b"/>')
def gnd(x,y):
 line(x,y,x,y+2);line(x-3,y+2,x+3,y+2);line(x-2,y+3.3,x+2,y+3.3);line(x-1,y+4.6,x+1,y+4.6)
def resistor(x,y,ref,value,vertical=False):
 if vertical:
  line(x,y,x,y+3);box(x-1.5,y+3,3,7);line(x,y+10,x,y+13);text(x-3 if ref=='R6' else x+3,y+5,ref,2.5,anchor='end' if ref=='R6' else 'start');text(x-3 if ref=='R6' else x+3,y+9,value,2.5,anchor='end' if ref=='R6' else 'start')
 else:
  line(x,y,x+3,y);box(x+3,y-1.5,8,3);line(x+11,y,x+14,y);text(x+7,y-5,ref,2.5,anchor='middle');text(x+7,y+6,value,2.5,anchor='middle')
def cap(x,y,ref,value,polar=False):
 line(x,y,x,y+5);line(x-3,y+5,x+3,y+5);line(x-3,y+7,x+3,y+7);line(x,y+7,x,y+13)
 text(x+4,y+4,ref,2.5);text(x+4,y+9,value,2.5)
 if polar:text(x-5,y+3,'+',2.7)
def net(x,y,label):text(x,y-1.8,label,2.6,bold=True)
def header(sheet,title):
 global svg
 svg=['<svg xmlns="http://www.w3.org/2000/svg" width="420mm" height="297mm" viewBox="0 0 420 297"><rect width="420" height="297" fill="white"/>']
 box(7,7,406,283);text(15,20,'4VRS',6,True);text(46,20,'Samsung-ESP32-MQTT-Modbus',5,True)
 text(405,18,'REV A / ЭЛЕКТРИЧЕСКАЯ СХЕМА',3,True,'end','#a25a00');text(405,25,'26.09.2026',2.7,anchor='end')
 line(15,29,405,29);text(15,37,title,3.8,True)
 box(15,270,390,15);line(285,270,285,285);line(360,270,360,285)
 text(19,276,'Samsung UART bridge + Modbus RS485',3.2,True);text(19,282,'Схема электрическая принципиальная · Samsung UART bridge',2.6)
 text(289,276,'4VRS-SAM-001 / Э3',3,True);text(289,282,'A3 • Масштаб: условно',2.6)
 text(364,276,f'Лист {sheet} / 2',3,True);text(364,282,'Rev A',2.6)
def finish(name):
 (ROOT/f'drawings/{name}.svg').write_text('\n'.join(svg+['</svg>']),encoding='utf-8');pdf.showPage()

header(1,'Схема электрическая принципиальная')
text(16,46,'ПИТАНИЕ И EN',3.2,True)
box(46,58,36,26);text(64,64,'U2',3,True,'middle');text(64,70,'AMS1117-3.3',3,anchor='middle')
line(20,75,46,75);text(43,73,'3 IN',2.5,anchor='end');line(20,75,20,58);text(16,55,'+5V (controller mb)',2.8,True)
line(82,75,150,75);text(84,73,'2 OUT / TAB',2.5);net(106,75,'+3V3')
line(64,84,64,102);text(67,90,'1 GND',2.4);gnd(64,102)
dot(29,75);cap(29,75,'C1','0,1 мкФ');line(29,88,29,102);gnd(29,102)
dot(103,75);cap(103,75,'C2','47 мкФ / 6,3 В',True);line(103,88,103,102);gnd(103,102)
line(231,52,231,55);net(224,52,'+3V3');resistor(231,55,'R1','10 кОм',True)
line(231,68,231,74)
dot(140,75);cap(140,75,'C3','0,1 мкФ');line(140,88,140,102);gnd(140,102)
text(16,115,'C2 и C3: между +3V3 и GND. EN: подтяжка R1 к +3V3.',2.8,True)
text(16,120,'Питание controller mb и display CN1: общие +5V и GND.',2.6)

box(165,74,90,137);text(210,82,'U1',4,True,'middle');text(210,89,'ESP32-S3-WROOM-1',3.7,True,'middle');text(210,95,'N16R8',3.2,anchor='middle')
text(210,103,'GPIO ≠ номер контакта модуля',2.8,True,'middle')
line(184,74,184,59);net(177,59,'+3V3');text(187,70,'2 / 3V3',2.6)
text(234,70,'3 / EN',2.6)
text(286,59,'ПРОГРАММАТОР / UART 3,3 В',3.1,True)
text(286,66,'TX и RX указаны со стороны адаптера',2.6)
for yy,pad,gpio,label in [(111,'37','GPIO43 / TXD0','RX адаптера'),(121,'36','GPIO44 / RXD0','TX адаптера')]:
 line(255,yy,365,yy);text(258,yy-2,pad,2.4);text(251,yy+1,gpio,2.7,anchor='end');text(366,yy+1,label,2.7)
line(365,77,365,86);gnd(365,86);text(368,79,'GND адаптера',2.7)
text(286,98,'BOOT: GPIO0 = 0 при сбросе EN',2.6)
text(169,133,'MAIN UART',2.7,True);text(251,133,'FACTORY UART',2.7,True,'end')
# Main UART at left, contact numbers belong to Samsung CN1.
text(16,134,'ОСНОВНАЯ ПЛАТА SAMSUNG',3.1,True)
text(16,140,'controller mb = motherboard',2.6)
text(16,146,'TX (controller mb)',2.6,True);line(45,150,68,150);resistor(68,150,'R4','10 кОм');line(82,150,115,150);dot(115,150);line(115,150,165,150)
text(162,147,'11',2.4,anchor='end');text(169,151,'GPIO18 / RX',3)
resistor(115,150,'R5','20 кОм',True);line(115,163,115,166);gnd(115,166)
text(16,176,'RX (controller mb)',2.6,True);line(45,180,165,180);text(162,177,'10',2.4,anchor='end');text(169,181,'GPIO17 / TX',3)
# Factory UART at right.
text(286,134,'ШТАТНАЯ ПЛАТА ДИСПЛЕЯ / Wi-Fi',3.1,True)
text(286,140,'Контакты CN1 на самой плате',2.6)
line(255,150,280,150);dot(280,150);line(280,150,303,150);resistor(303,150,'R3','10 кОм');line(317,150,365,150);text(341,146,'TX (display CN1)',2.6,True)
text(258,147,'8',2.4);text(251,151,'GPIO15 / RX',3,anchor='end');resistor(280,150,'R2','20 кОм',True);line(280,163,280,166);gnd(280,166)
line(255,180,365,180);text(341,176,'RX (display CN1)',2.6,True);text(258,177,'9',2.4);text(251,181,'GPIO16 / TX',3,anchor='end')
# External power connections share the named supply and ground nets.
text(16,190,'GND (controller mb)',2.6,True);line(16,193,44,193);gnd(44,193)
net(325,158,'+5V');line(325,158,362,158);text(364,159,'+5V (display CN1)',2.4,True)
line(335,167,362,167);gnd(335,167);text(364,168,'GND (display CN1)',2.4,True)
# Modbus named nets, exact physical pads.
text(169,194,'GPIO8 / RX',2.7);text(162,193,'12',2.3,anchor='end');line(165,194,145,194);text(143,195,'MB_RX',2.7,True,'end')
text(251,194,'GPIO9 / TX',2.7,anchor='end');text(258,193,'17',2.3);line(255,194,272,194);text(274,195,'MB_TX',2.7,True)
text(210,204,'GPIO21 / DE  ·  контакт 23',2.7,anchor='middle');line(210,211,210,218);text(210,223,'MB_DE',2.7,True,'middle')
line(178,211,178,224);text(163,232,'GND: 1, 40, EP41',2.5);gnd(178,224)

# Compact RS485 stage.
text(286,201,'MODBUS RTU / RS485',3.1,True)
box(300,221,37,39);text(303,211,'U3 / MAX485',3,True)
text(303,228,'1 RO',2.6);line(300,227,285,227);resistor(271,227,'R7','10 кОм');line(271,227,246,227);dot(254,227);text(243,228,'MB_RX',2.7,True,'end')
resistor(254,227,'R6','20 кОм',True);line(254,240,254,244);gnd(254,244)
text(303,237,'2 /RE',2.6);text(303,243,'3 DE',2.6);line(300,236,290,236);line(300,242,290,242);line(290,236,290,242);dot(290,236)
line(275,236,290,236);text(273,237,'MB_DE',2.6,True,'end')
resistor(283,236,'R8','10 кОм',True);line(283,236,290,236);dot(283,236);gnd(283,249)
text(303,253,'4 DI',2.6);line(300,252,294,252);line(294,252,294,265);line(294,265,268,265);text(266,266,'MB_TX',2.6,True,'end')
line(321,221,321,215);text(333,219,'8 VCC',2.3)
line(321,215,365,215);line(365,215,365,190);line(365,190,380,190);net(369,190,'+5V');cap(380,190,'C4','0,1 мкФ');gnd(380,203)
text(333,233,'7 B',2.6,anchor='end');line(337,232,346,232);line(346,232,346,219);line(346,219,392,219);text(396,220,'B',3.2,True)
text(333,251,'6 A',2.6,anchor='end');line(337,250,392,250);text(396,251,'A',3.2,True)
line(318,260,318,264);text(321,264,'5 GND',2.4);gnd(318,264)
# Link VCC to its labelled net explicitly rather than via the body.
# C4 top terminal and U3 VCC are connected by a continuous wire.
text(16,206,'СОХРАНИТЬ ШТАТНЫЕ СОЕДИНЕНИЯ',3,True)
text(16,213,'display CN1 №6: +5V; display CN1 №5: GND.',2.8)
text(16,219,'Питание платы дисплея остаётся подключённым.',2.8)
text(16,225,'№11 и остальные цепи CN1 не разрываются.',2.8)
text(16,241,'RX-делители: 10 кОм последовательно + 20 кОм на GND.',2.8)
text(16,247,'Все символы GND и одноимённые метки электрически соединены.',2.6)
text(16,253,'Незадействованные GPIO не показаны. EP41: сверить монтаж.',2.6)
text(16,259,'MAX485: SOIC-8. R и керамика: 0805. C2 и разъёмы: уточнить.',2.6,color='#a25a00')
finish('Samsung_transparency_bridge_revA-sheet1')

header(2,'Таблица соединений и база компонентов / подготовка к PCB')
text(16,48,'СОЕДИНЕНИЯ ESP32 И КОНДИЦИОНЕРА',3.5,True)
rows=[['Сеть / назначение','GPIO','Контакт U1','Соединение'],['MAIN_RX','18','11','Основная плата CN1 №10 → R4 → GPIO18; R5 → GND'],['MAIN_TX','17','10','GPIO17 → основная плата CN1 №9'],['FACTORY_RX','15','8','Плата дисплея CN1 №9 → R3 → GPIO15; R2 → GND'],['FACTORY_TX','16','9','GPIO16 → плата дисплея CN1 №10'],['MB_RX','8','12','MAX485 RO (1) → R7 → GPIO8; R6 → GND'],['MB_TX','9','17','GPIO9 → MAX485 DI (4)'],['MB_DE','21','23','GPIO21 → MAX485 /RE (2) и DE (3); R8 → GND'],['PROG_TX / RX','43 / 44','37 / 36','GPIO43 → RX адаптера; GPIO44 ← TX адаптера; GND общий'],['+3V3 / EN / GND','—','2 / 3 / 1,40,41','3,3 В / разрешение / земля; EP41 → GND']]
def table(x,y,widths,rows,height=8.5):
 for ri,row in enumerate(rows):
  xx=x
  for w,s in zip(widths,row):
   box(xx,y+ri*height,w,height,fill='#eef3f6' if ri==0 else None);text(xx+2,y+ri*height+height*.66,str(s),2.7,ri==0);xx+=w
table(16,53,[50,18,28,292],rows,8)
text(16,145,'ПЕРЕЧЕНЬ ЭЛЕМЕНТОВ',3.5,True)
bom=[['Поз.','Номинал / модель','Кол.','Корпус / статус'],['U1','ESP32-S3-WROOM-1-N16R8','1','Модуль Espressif; GPIO и pad сверены с datasheet'],['U2','AMS1117-3.3','1','SOT-223 / tab = OUT; корпус предложен для PCB'],['U3','MAX485','1','SOIC-8, 3,9 × 4,9 мм, шаг 1,27 мм'],['R1, R3, R4, R7, R8','10 кОм','5','0805; допуск и мощность выбрать при закупке'],['R2, R5, R6','20 кОм','3','0805; допуск и мощность выбрать при закупке'],['C1, C3, C4','0,1 мкФ','3','0805; рабочее напряжение выбрать при закупке'],['C2','47 мкФ / 6,3 В','1','Полярный; посадочное место требует размеров корпуса']]
table(16,150,[47,93,18,230],bom,8)
text(16,224,'ПРАВКИ ОБОЗНАЧЕНИЙ',3.1,True);text(16,231,'Второй «R6» на исходном рисунке (10 кОм на линии DE) переименован в R8.',2.8)
text(16,240,'ГРАНИЦЫ ЭТОЙ РЕВИЗИИ',3.1,True)
text(16,247,'Основа PCB: символы, посадочные места, BOM и сети. Корпус C2 и разъёмы ещё не выбраны; разводки / Gerber нет.',2.8)
text(16,253,'Программатор: UART 3,3 В + GND; для загрузчика нужны GPIO0 и сброс EN. Автосброс не добавлен.',2.8)
text(16,261,'Источники распиновок: Espressif ESP32-S3-WROOM-1; Analog Devices MAX485; AMS1117 datasheet. Ссылки в README.',2.6)
finish('Samsung_transparency_bridge_revA-sheet2')
pdf.save()
print('Created PDF and two SVG sheets')
