// صفحه‌ی تنظیمات موچی (http://moochi.local یا آی‌پی ربات)
#pragma once
static const char WEB_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="fa" dir="rtl"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>موچی</title>
<style>
:root{--bg:#fff5f8;--card:#fff;--ink:#3a2a33;--mut:#8a7480;--pink:#e85d8f;--line:#f1d4df}
@media(prefers-color-scheme:dark){:root{--bg:#1d1418;--card:#2a1e24;--ink:#f6e9ef;--mut:#b99aa8;--pink:#ff7aa8;--line:#3d2b33}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.7 Vazirmatn,Tahoma,sans-serif}
main{max-width:520px;margin:auto;padding:16px}
h1{font-size:26px;margin:8px 0 2px;color:var(--pink)}h2{font-size:16px;margin:0 0 10px}
.sub{color:var(--mut);margin:0 0 16px}
.card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:14px 16px;margin:12px 0}
label{display:block;color:var(--mut);font-size:13px;margin:8px 0 2px}
input,select,textarea{width:100%;font:inherit;color:inherit;background:var(--bg);border:1px solid var(--line);border-radius:10px;padding:8px 10px}
textarea{min-height:80px;resize:vertical}
button{font:inherit;border:0;border-radius:10px;padding:9px 14px;background:var(--pink);color:#fff;cursor:pointer;margin:6px 0 0 6px}
button.g{background:transparent;color:var(--pink);border:1px solid var(--pink)}
.row{display:flex;gap:8px}.row>*{flex:1}
.days{display:flex;gap:4px;flex-wrap:wrap}.days label{display:flex;align-items:center;gap:2px;margin:0;color:var(--ink)}
.days input{width:auto}
.stat{display:grid;grid-template-columns:auto 1fr;gap:2px 12px}.stat b{font-weight:600}
#toast{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:var(--ink);color:var(--bg);padding:8px 16px;border-radius:20px;opacity:0;transition:.3s}
</style></head><body><main>
<h1>موچی ♥</h1><p class="sub">ربات کوچولوی )HTML" OWNER_NAME R"HTML(</p>

<div class="card"><h2>وضعیت</h2><div class="stat" id="st">...</div></div>

<div class="card"><h2>پیام به موچی</h2>
<textarea id="msg" placeholder="مثلا: سلام عشقم ❤"></textarea>
<button onclick="send()">بفرست</button><button class="g" onclick="api('/api/msg',{t:'❤'})">فقط قلب ❤</button></div>

<div class="card"><h2>نقاشی برای موچی 🎨</h2>
<canvas id="cv" width="128" height="48" style="width:100%;image-rendering:pixelated;background:#000;border-radius:10px;touch-action:none"></canvas>
<div class="row" style="align-items:center"><label style="margin:0"><input type="checkbox" id="er" style="width:auto"> پاک‌کن</label>
<select id="pen"><option value="1">قلم نازک</option><option value="2" selected>قلم متوسط</option><option value="4">قلم کلفت</option></select></div>
<button onclick="sendDoodle()">بفرست برای موچی</button><button class="g" onclick="clr()">پاک کردن</button><button class="g" onclick="copyDoodle()">کپی برای بات</button></div>

<div class="card"><h2>آخرین پیام روی موچی</h2>
<p id="last" style="margin:0 0 4px;white-space:pre-wrap">—</p><p class="sub" id="lastst" style="margin:0"></p>
<button class="g" onclick="act('showmsg')">دوباره روی موچی نشان بده</button><button class="g" onclick="act('read')">برداشتن علامت نامه 💌</button>
<p class="sub" style="margin:6px 0 0">روی خود موچی: تاچ چپ یا راست را بزن تا پیام نشان داده شود و علامت نامه برود.</p></div>

<div class="card"><h2>زنگ ساعت</h2>
<div class="row"><div><label>ساعت</label><input type="time" id="al"></div>
<div><label>روشن؟</label><select id="aon"><option value="1">روشن</option><option value="0">خاموش</option></select></div></div>
<label>روزها</label><div class="days" id="days"></div>
<button onclick="saveAlarm()">ذخیره</button></div>

<div class="card"><h2>شهر و ساعت</h2>
<select id="city"></select><button onclick="saveCity()">ذخیره</button>
<p class="sub" style="margin:6px 0 0">برای آب‌وهوا و تنظیم خودکار ساعت</p></div>

<div class="card"><h2>روزهای خاص (شمسی)</h2>
<div class="row"><div><label>تولد )HTML" OWNER_NAME R"HTML(: ماه</label><input id="bm" type="number" min="0" max="12"></div><div><label>روز</label><input id="bd" type="number" min="0" max="31"></div></div>
<div class="row"><div><label>سالگرد ازدواج: ماه</label><input id="am" type="number" min="0" max="12"></div><div><label>روز</label><input id="ad" type="number" min="0" max="31"></div></div>
<button onclick="api('/api/set',{bm:v('bm'),bd:v('bd'),am:v('am'),ad:v('ad')})">ذخیره</button></div>

<div class="card"><h2>سرگرمی</h2>
<button class="g" onclick="act('app',0)">توپ جادویی</button><button class="g" onclick="act('app',1)">تاس</button>
<button class="g" onclick="act('app',2)">بازی سرعت</button><button class="g" onclick="act('app',3)">بازی حافظه</button>
<button class="g" onclick="act('app',4)">تایمر</button><button class="g" onclick="act('app',5)">چراغ خواب</button>
<button class="g" onclick="act('app',6)">عشق‌سنج</button><button class="g" onclick="act('boot')">سلام اول</button>
<button class="g" onclick="act('fal')">فال حافظ</button><button class="g" onclick="act('music')">آهنگ</button>
<button class="g" onclick="act('ask')">بپرس حالت چطوره</button><button class="g" onclick="act('app',9)">جواب به )HTML" FRIEND_NAME R"HTML(</button>
<label>حالت چهره</label>
<div class="row"><select id="mood"><option>happy</option><option>love</option><option>angry</option><option>sad</option><option>surprised</option><option>sleep</option><option>dizzy</option><option>wink</option><option>curious</option><option>sulk</option></select>
<button onclick="act('mood',v('mood'))">نشان بده</button></div></div>

<div class="card"><h2>وای‌فای</h2>
<label>اسم وای‌فای</label><input id="ssid"><label>رمز</label><input id="pass" type="password">
<button onclick="if(confirm('موچی با وای‌فای جدید دوباره روشن می‌شود'))api('/api/set',{ssid:v('ssid'),pass:v('pass')})">ذخیره و ری‌استارت</button></div>

<div class="card"><h2>دیدار و یادآورها</h2>
<label>روز دیدار (میلادی) — در صفحه‌ی ساعت شمارش معکوس می‌شود</label><input type="date" id="meet">
<label>یادآورهای هر روز (هر خط: ساعت و متن)</label><textarea id="rem" dir="rtl" placeholder="09:00 صبحونه یادت نره&#10;22:30 قرصت ❤"></textarea>
<button onclick="api('/api/set',{meet:v('meet'),rem:v('rem')})">ذخیره</button><button class="g" onclick="act('note','این یه یادآور آزمایشیه ⏰')">آزمایش</button></div>

<div class="card"><h2>بات بله / تلگرام 🤖</h2>
<p class="sub" style="margin:0" id="botst"></p>
<label>پیام‌رسان</label><select id="bapi"><option value="0">بله (bale.ai)</option><option value="1">تلگرام</option></select>
<label>توکن بات (از BotFather)</label><input id="btok" dir="ltr" placeholder="123456789:ABC...">
<button onclick="api('/api/set',{botTok:v('btok')||'*',botApi:v('bapi')})">ذخیره</button>
<p class="sub" style="margin:6px 0 0">بعد از ذخیره ، در بات بنویس: <b dir="ltr" id="pairc"></b></p></div>
<div id="toast"></div>
</main><script>
const C=[["تهران","<+0330>-3:30",35.69,51.39],["تبریز","<+0330>-3:30",38.08,46.29],["مشهد","<+0330>-3:30",36.30,59.60],["اصفهان","<+0330>-3:30",32.65,51.67],["شیراز","<+0330>-3:30",29.59,52.58],
["تورنتو","EST5EDT,M3.2.0,M11.1.0",43.65,-79.38],["مونترال","EST5EDT,M3.2.0,M11.1.0",45.50,-73.57],["اتاوا","EST5EDT,M3.2.0,M11.1.0",45.42,-75.70],
["ونکوور","PST8PDT,M3.2.0,M11.1.0",49.28,-123.12],["کلگری","MST7MDT,M3.2.0,M11.1.0",51.05,-114.07],["ادمونتون","MST7MDT,M3.2.0,M11.1.0",53.55,-113.49],
["وینیپگ","CST6CDT,M3.2.0,M11.1.0",49.90,-97.14],["هالیفکس","AST4ADT,M3.2.0,M11.1.0",44.65,-63.58]];
const D=["شنبه","یکشنبه","دوشنبه","سه‌شنبه","چهارشنبه","پنجشنبه","جمعه"];
const $=id=>document.getElementById(id),v=id=>$(id).value;
C.forEach((c,i)=>$('city').add(new Option(c[0],i)));
D.forEach((d,i)=>$('days').insertAdjacentHTML('beforeend',`<label><input type=checkbox id=d${i} checked>${d}</label>`));
function toast(t){$('toast').textContent=t;$('toast').style.opacity=1;setTimeout(()=>$('toast').style.opacity=0,1800)}
async function api(u,p){const r=await fetch(u+'?'+new URLSearchParams(p),{method:'POST'});toast(r.ok?'انجام شد ♥':'خطا');load()}
function act(a,x){api('/api/do',{a:a,x:x??''})}
function send(){const t=v('msg').trim();if(t){api('/api/msg',{t:t});$('msg').value=''}}
function saveAlarm(){let m=0;D.forEach((d,i)=>{if($('d'+i).checked)m|=1<<i});const [h,mi]=(v('al')||'07:00').split(':');api('/api/set',{ah:+h,am2:+mi,aon:v('aon'),adays:m})}
function saveCity(){const c=C[v('city')];api('/api/set',{city:v('city'),tz:c[1],lat:c[2],lon:c[3]})}
let first=true;
async function load(){try{const s=await (await fetch('/api/status')).json();
$('st').innerHTML=`<b>ساعت</b><span>${s.time} — ${s.date}</span><b>آب‌وهوا</b><span>${s.temp==null?'—':s.temp+'°'}</span><b>عشق‌سنج</b><span>${s.love}٪ ♥</span><b>پیام خوانده‌نشده</b><span>${s.unread?'دارد 💌':'ندارد'}</span><b>امتیاز محبت</b><span>${s.xp} ⭐</span><b>حال امروز )HTML" OWNER_NAME R"HTML(</b><span>${s.mood||'—'}</span>${s.meetDays>=0?`<b>تا دیدار</b><span>${s.meetDays} روز ✈</span>`:''}<b>وای‌فای</b><span>${s.wifi}</span>`;
$('last').textContent=s.msg||'—';$('lastst').textContent=s.msg?(s.unread?'هنوز خوانده نشده 💌':'خوانده شده ✓'):'';
$('pairc').textContent='/pair '+s.pair;
$('botst').innerHTML=!s.bot?'هنوز تنظیم نشده':(s.owner?'✅ وصل است و صاحبش ثبت شده':'⏳ توکن ذخیره شده — حالا در بات بنویس /pair '+s.pair);
if(first){first=false;$('al').value=String(s.ah).padStart(2,'0')+':'+String(s.am).padStart(2,'0');$('aon').value=s.aon;D.forEach((d,i)=>$('d'+i).checked=!!(s.adays>>i&1));
$('city').value=s.city;$('meet').value=s.meet;$('rem').value=s.rem;$('bapi').value=s.botApi;$('bm').value=s.bm;$('bd').value=s.bd;$('am').value=s.anm;$('ad').value=s.and;$('ssid').value=s.ssid}}catch(e){}}
load();setInterval(load,5000);
// ---- نقاشی ۱۲۸×۴۸ ----
const cv=$('cv'),cx=cv.getContext('2d');let dn=false,lp=null;
function clr(){cx.fillStyle='#000';cx.fillRect(0,0,128,48)}clr();
function pt(e){const r=cv.getBoundingClientRect(),t=e.touches?e.touches[0]:e;return[(t.clientX-r.left)*128/r.width,(t.clientY-r.top)*48/r.height]}
function dot(x,y){const w=+v('pen');cx.fillStyle=$('er').checked?'#000':'#fff';cx.fillRect(Math.round(x-w/2),Math.round(y-w/2),w,w)}
function mv(e){if(!dn)return;e.preventDefault();const p=pt(e);if(lp){const d=Math.max(Math.abs(p[0]-lp[0]),Math.abs(p[1]-lp[1]),1);for(let i=0;i<=d;i++)dot(lp[0]+(p[0]-lp[0])*i/d,lp[1]+(p[1]-lp[1])*i/d)}else dot(...p);lp=p}
cv.onmousedown=cv.ontouchstart=e=>{dn=true;lp=null;mv(e)};onmouseup=ontouchend=()=>{dn=false;lp=null};cv.onmousemove=cv.ontouchmove=mv;
function b64(){const d=cx.getImageData(0,0,128,48).data,b=new Uint8Array(768);for(let y=0;y<48;y++)for(let x=0;x<128;x++)if(d[(y*128+x)*4]>127)b[y*16+(x>>3)]|=1<<(x&7);let s='';b.forEach(c=>s+=String.fromCharCode(c));return btoa(s)}
async function sendDoodle(){const r=await fetch('/api/doodle',{method:'POST',body:new URLSearchParams({d:b64()})});toast(r.ok?'رسید ♥':'خطا')}
async function copyDoodle(){try{await navigator.clipboard.writeText('DOODLE:'+b64());toast('کپی شد — در بات بچسبان')}catch(e){prompt('این را کپی کن:','DOODLE:'+b64())}}
</script></body></html>)HTML";
