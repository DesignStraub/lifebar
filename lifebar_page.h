#pragma once
#include <Arduino.h>

/* =====================================================================
   LifeBar - Auswertungsseite unter http://lifebar.local

   Liegt im PROGMEM und wird mit send_P ausgeliefert, belegt also kein
   RAM. Die Seite holt sich /info und /events selbst und rechnet alles
   im Browser - das Geraet muss nichts aggregieren.
   ===================================================================== */

static const char PAGE_HTML[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html><html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LifeBar</title>
<style>
*{box-sizing:border-box}
body{margin:0;padding:28px 20px 60px;background:#0b0b0f;color:#f2f2f7;
 font-family:ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif}
.wrap{max-width:940px;margin:0 auto}
h1{font-size:13px;letter-spacing:.18em;color:#7a7a88;font-weight:500;
 margin:0 0 18px;text-transform:uppercase}
.hero{background:#15151c;border:1px solid #23232e;border-radius:14px;
 padding:22px 24px;margin-bottom:18px}
.hrow{display:flex;align-items:baseline;gap:14px;flex-wrap:wrap}
.lvl{font-size:34px;font-weight:700;line-height:1}
.title{font-size:20px;font-weight:600;letter-spacing:.04em}
.spacer{flex:1}
.tot{color:#7a7a88;font-size:14px}
.bar{height:20px;background:#23232e;border-radius:6px;margin-top:16px;
 position:relative;overflow:hidden}
.fill{height:100%;width:0;transition:width .5s ease}
.barlab{position:absolute;inset:0;display:flex;align-items:center;
 justify-content:center;font-size:12px;font-variant-numeric:tabular-nums}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}
@media(max-width:760px){.grid{grid-template-columns:1fr}}
.card{background:#15151c;border:1px solid #23232e;border-radius:14px;padding:20px}
.card h2{font-size:11px;letter-spacing:.16em;color:#7a7a88;font-weight:500;
 margin:0 0 16px;text-transform:uppercase}
.cal{display:flex;gap:3px;overflow-x:auto;padding-bottom:4px}
.col{display:flex;flex-direction:column;gap:3px}
.day{width:14px;height:14px;border-radius:3px;background:#1c1c24;
 cursor:pointer;position:relative}
.day:hover{outline:1.5px solid #4a4a5a}
.day.sel{outline:2px solid #f2f2f7}
.day .bad{position:absolute;right:-1px;top:-1px;width:5px;height:5px;
 border-radius:50%;background:#ff4d4d}
.legend{display:flex;align-items:center;gap:6px;margin-top:12px;
 font-size:11px;color:#5c5c6a}
.legend i{width:11px;height:11px;border-radius:2px;display:block}
.stat{display:flex;justify-content:space-between;padding:7px 0;
 border-bottom:1px solid #1e1e28;font-size:14px}
.stat:last-child{border:0}
.stat span:last-child{font-variant-numeric:tabular-nums}
.mut{color:#7a7a88}
.ev{display:flex;align-items:center;gap:10px;padding:8px 0;
 border-bottom:1px solid #1a1a22;font-size:14px}
.ev:last-child{border:0}
.dot{width:7px;height:7px;border-radius:50%;flex:none}
.ev time{color:#5c5c6a;font-size:12px;width:132px;flex:none;
 font-variant-numeric:tabular-nums}
.ev b{font-weight:500;flex:1}
.ev em{font-style:normal;font-variant-numeric:tabular-nums}
.pos{color:#3ddc7f}.neg{color:#ff6b6b}
#chron{margin-top:14px;font-size:12px;color:#5c5c6a;display:flex;
 flex-wrap:wrap;gap:6px}
#chron span{background:#1c1c24;border-radius:4px;padding:3px 8px}
#chron span:first-child{color:#f2f2f7}
#foot{margin-top:22px;font-size:11px;color:#3f3f4c;text-align:center}
.empty{color:#5c5c6a;font-size:14px;padding:10px 0}
.pnls{display:flex;flex-wrap:wrap;gap:10px}
.pnl{background:#1c1c24;border:1px solid #23232e;border-radius:10px;
 padding:10px 12px;min-width:132px;flex:1}
.pnl.v{border-color:#3a2028}
.pnl b{display:block;font-size:12px;letter-spacing:.08em;color:#8a8a99;
 font-weight:500}
.pnl s{display:block;text-decoration:none;font-size:19px;margin:4px 0 8px;
 font-variant-numeric:tabular-nums}
.pnl div{display:flex;gap:8px}
.pnl button{flex:1;padding:11px 0;border:0;border-radius:7px;font-size:16px;
 background:#23232e;color:#f2f2f7;cursor:pointer}
.pnl button.p{background:#2b6b45}
.pnl.v button.p{background:#7a2e38}
.pnl button:active{filter:brightness(1.4)}
.pie{display:flex;align-items:center;gap:20px}
.pie svg{flex:none}
.leg{font-size:13px;flex:1;min-width:0}
.leg div{display:flex;align-items:center;gap:8px;padding:3px 0}
.leg i{width:9px;height:9px;border-radius:2px;flex:none}
.leg span:first-of-type{flex:1;overflow:hidden;text-overflow:ellipsis;
 white-space:nowrap}
.leg b{font-weight:500;font-variant-numeric:tabular-nums;color:#7a7a88}
#hours svg{width:100%;height:auto;display:block}
#hours rect{cursor:default}
#hours rect:hover{opacity:1!important}
.hx{fill:#4a4a5a;font-size:9px}
</style></head><body><div class="wrap">

<h1>LifeBar</h1>

<div class="hero">
  <div class="hrow">
    <div class="lvl" id="lvl">LVL -</div>
    <div class="title" id="cls">&nbsp;</div>
    <div class="spacer"></div>
    <div class="tot" id="tot"></div>
  </div>
  <div id="chron"></div>
  <div class="bar"><div class="fill" id="fill"></div>
    <div class="barlab" id="exp"></div></div>
</div>

<div class="grid">
  <div class="card">
    <h2 id="calhead">Last 13 weeks</h2>
    <div class="cal" id="cal"></div>
    <div class="legend"><span id="lless">less</span>
      <i style="background:#1c1c24"></i><i style="background:#1d4a2e"></i>
      <i style="background:#2a7d46"></i><i style="background:#3ab161"></i>
      <i style="background:#4ee88a"></i><span id="lmore">more</span>
      <span style="margin-left:10px">
        <i style="background:#ff4d4d;border-radius:50%;display:inline-block;
           width:6px;height:6px"></i> <span id="lvice">Vices</span></span>
    </div>
    <div style="margin-top:18px" id="daystats"></div>
  </div>

  <div class="card">
    <h2 id="evhead">History</h2>
    <div id="events"></div>
  </div>
</div>

<div class="grid" style="margin-top:18px">
  <div class="card"><h2 id="bhead">Skills</h2>
    <div class="pnls" id="panelsS"></div></div>
  <div class="card"><h2 id="bheadv">Vices</h2>
    <div class="pnls" id="panelsV"></div></div>
</div>

<div class="card" style="margin-top:18px">
  <h2 id="hhead">Time of day</h2>
  <div id="hours"></div>
</div>

<div class="grid" style="margin-top:18px">
  <div class="card"><h2 id="dh1">Skills</h2>
    <div class="pie"><div id="pie1"></div><div class="leg" id="leg1"></div></div></div>
  <div class="card"><h2 id="dh2">Vices</h2>
    <div class="pie"><div id="pie2"></div><div class="leg" id="leg2"></div></div></div>
</div>

<div id="foot"></div>
</div>
<script>
const L={de:{w13:"Letzte 13 Wochen",less:"weniger",more:"mehr",vice:"Laster",
   hist:"Verlauf",evOn:"Ereignisse am ",none:"Keine Ereignisse.",
   nothing:"Nichts eingetragen.",tot:" gesamt",ev:" Ereignisse",
   noclock:" - Geraet hat keine gueltige Uhrzeit",off:"Geraet nicht erreichbar",
   noclock2:"Das Geraet hat keine gueltige Uhrzeit.",tod:"Tageszeit",todAvg:"Tageszeit - Schnitt aller Tage",skills:"Skills",
   vices2:"Laster",noData:"Noch keine Daten.",noVices:"Keine Laster eingerichtet."},
 en:{w13:"Last 13 weeks",less:"less",more:"more",vice:"Vices",
   hist:"History",evOn:"Events on ",none:"No events.",
   nothing:"Nothing logged.",tot:" total",ev:" events",
   noclock:" - device has no valid clock",off:"Device unreachable",
   noclock2:"The device has no valid clock.",tod:"Time of day",todAvg:"Time of day - average of all days",skills:"Skills",
   vices2:"Vices",noData:"No data yet.",noVices:"No vices set up."}};
let LC="en",loc="en-US",h12=false,INFO=null;

const RARITY=[[25,"#ff8000"],[20,"#a335ee"],[15,"#0070dd"],
              [10,"#1eff00"],[5,"#ffffff"],[0,"#9d9d9d"]];
const rarity=l=>{for(const[m,c]of RARITY)if(l>=m)return c;return"#9d9d9d"};
const hrs=m=>(m/60).toFixed(1).replace(".0","")+" h";
const key=d=>d.toISOString().slice(0,10);
const pad=n=>String(n).padStart(2,"0");
/* folgt der Einstellung auf dem Geraet */
const clk=d=>h12
  ?((d.getHours()%12||12)+":"+pad(d.getMinutes())+(d.getHours()<12?" AM":" PM"))
  :(pad(d.getHours())+":"+pad(d.getMinutes()));

let events=[],lastTs=0;
/* Startet auf heute - die ganze Historie auf einmal ist selten das,
   was man sehen will. Klick auf den Tag hebt die Auswahl auf.      */
let sel=new Date().toISOString().slice(0,10);

async function pullInfo(){
  const i=await(await fetch("/info")).json();
  INFO=i;
  /* Geraet wurde zurueckgesetzt: eigene Kopie verwerfen */
  if(i.events<events.length){events=[];lastTs=0;sel=null;}
  const col=rarity(i.level);
  lvl.textContent="LVL "+i.level;  lvl.style.color=col;
  cls.textContent=i.class;         cls.style.color=col;
  LC=(i.lang==="de")?"de":"en";
  h12=!!i.clock12;
  loc=(LC==="de")?"de-DE":"en-GB";
  document.documentElement.lang=LC;
  calhead.textContent=L[LC].w13;
  lless.textContent=L[LC].less; lmore.textContent=L[LC].more;
  lvice.textContent=L[LC].vice;
  tot.textContent=hrs(i.total_min)+L[LC].tot;
  const p=i.need?Math.min(100,i.have/i.need*100):0;
  fill.style.width=p+"%";
  fill.style.background=col;
  fill.style.filter="brightness(.5)";
  exp.textContent=i.have+" / "+i.need+" EXP";
  const ts=i.titles||[];
  chron.innerHTML=ts.length
    ?ts.slice(0,8).map(t=>"<span>"+t+"</span>").join("")
    :"";
  foot.textContent=i.time_valid?"":L[LC].noclock2;
}

async function pullEvents(){
  const t=await(await fetch("/events?since="+lastTs)).text();
  const rows=t.trim().split("\n").slice(1);
  for(const r of rows){
    if(!r)continue;
    const[ts,type,name,delta]=r.split(",");
    const n=+ts; if(!n)continue;
    events.push({ts:n,type,name,delta:+delta});
    if(n>lastTs)lastTs=n;
  }
  events.sort((a,b)=>b.ts-a.ts);
}

function byDay(){
  const m={};
  for(const e of events){
    const k=key(new Date(e.ts*1000));
    (m[k]=m[k]||{min:0,bad:0,list:[]});
    if(e.type==="skill")m[k].min+=e.delta; else m[k].bad+=e.delta;
    m[k].list.push(e);
  }
  return m;
}

function shade(min,peak){
  if(min<=0)return"#1c1c24";
  const r=min/Math.max(peak,60);
  if(r<.25)return"#1d4a2e";
  if(r<.5) return"#2a7d46";
  if(r<.8) return"#3ab161";
  return"#4ee88a";
}

function drawCal(map){
  const today=new Date();today.setHours(12,0,0,0);
  const back=(today.getDay()+6)%7;               // Montag = 0
  const monday=new Date(today);monday.setDate(today.getDate()-back);
  const peak=Math.max(60,...Object.values(map).map(d=>d.min));

  cal.innerHTML="";
  for(let w=12;w>=0;w--){
    const col=document.createElement("div");col.className="col";
    for(let d=0;d<7;d++){
      const day=new Date(monday);
      day.setDate(monday.getDate()-w*7+d);
      const k=key(day),info=map[k];
      const el=document.createElement("div");
      el.className="day";el.dataset.k=k;
      if(day>today)el.style.opacity=".25";
      el.style.background=shade(info?info.min:0,peak);
      el.title=day.toLocaleDateString(loc)+
        (info?" - "+hrs(info.min)+(info.bad?", "+info.bad+"x "+L[LC].vice:""):"");
      if(info&&info.bad>0){
        const b=document.createElement("span");b.className="bad";el.append(b);
      }
      el.onclick=()=>{sel=(sel===k?null:k);render()};
      if(sel===k)el.classList.add("sel");
      col.append(el);
    }
    cal.append(col);
  }
}

function drawDayStats(map){
  const k=sel||key(new Date());
  const info=map[k]||{min:0,bad:0,list:[]};
  const per={};
  for(const e of info.list)
    if(e.type==="skill")per[e.name]=(per[e.name]||0)+e.delta;

  let h='<div class="stat"><span class="mut">'+
    new Date(k).toLocaleDateString(loc,{weekday:"long",day:"2-digit",month:"2-digit"})+'</span><span>'+hrs(info.min)+'</span></div>';
  const ent=Object.entries(per).filter(([,v])=>v>0)
                  .sort((a,b)=>b[1]-a[1]);
  for(const[n,v]of ent)
    h+='<div class="stat"><span class="mut">'+n+'</span><span>'+hrs(v)+'</span></div>';
  if(!ent.length)h+='<div class="empty">'+L[LC].nothing+'</div>';
  daystats.innerHTML=h;
}

/* Was innerhalb von zwei Minuten aufs selbe Modul gebucht wurde,
   gehoert zusammen. Skills und Laster werden getrennt gebuendelt.  */
const GAP=120;
function merge(list){                 /* list ist absteigend sortiert */
  const groups=[],open={};
  for(const e of list){
    const k=e.type+"|"+e.name,g=open[k];
    if(g&&Math.abs(g.from-e.ts)<=GAP){
      g.delta+=e.delta;g.n++;
      g.from=Math.min(g.from,e.ts);g.to=Math.max(g.to,e.ts);
      continue;
    }
    const ng={type:e.type,name:e.name,delta:e.delta,n:1,from:e.ts,to:e.ts};
    groups.push(ng);open[k]=ng;
  }
  /* Summe null heisst: direkt wieder zurueckgenommen */
  return groups.filter(g=>g.delta!==0).sort((a,b)=>b.to-a.to);
}

function drawEvents(map){
  const raw=sel?(map[sel]?map[sel].list:[]):events;
  const list=merge(raw).slice(0,40);
  evhead.textContent=sel?L[LC].evOn+new Date(sel).toLocaleDateString(loc)
                       :L[LC].hist;

  if(!list.length){events_.innerHTML='<div class="empty">'+L[LC].none+'</div>';return}
  let h="";
  for(const e of list){
    const a=new Date(e.from*1000),b=new Date(e.to*1000);
    const bad=e.type!=="skill";
    const val=bad?(e.delta>0?"+"+e.delta+"x":e.delta+"x")
                 :(e.delta>0?"+":"-")+hrs(Math.abs(e.delta));
    let t=pad(a.getDate())+"."+pad(a.getMonth()+1)+" "+clk(a);
    if(e.n>1&&clk(a)!==clk(b)) t+="-"+clk(b);
    h+='<div class="ev"><span class="dot" style="background:'+
       (bad?"#ff4d4d":"#3ddc7f")+'"></span>'+
       '<time>'+t+'</time>'+
       '<b>'+e.name+'</b>'+
       '<em class="'+(e.delta>0?(bad?"neg":"pos"):"mut")+'">'+val+'</em></div>';
  }
  events_.innerHTML=h;
}

const events_=document.getElementById("events");

const PAL_S=["#3ddc7f","#4ea8ff","#ffb03a","#c678dd","#5ad2c8","#a3d15c"];
const PAL_V=["#ff6b6b","#ff9f45","#e05d8f","#d4553a","#c94f7c","#f0762b"];

/* Stunde 0-23: Skill-Minuten nach oben, Laster nach unten.
   Mit gewaehltem Tag genau dieser Tag, sonst der Schnitt.          */
function drawHours(map){
  const days=Object.keys(map).length||1;
  /* Dieselbe Buendelung wie in der Historie: was dort wegen einer
     Ruecknahme verschwindet, taucht auch hier nicht auf.            */
  const src=merge(sel?(map[sel]?map[sel].list:[]):events);
  const up=Array(24).fill(0),dn=Array(24).fill(0);
  const du=Array.from({length:24},()=>({})),dd=Array.from({length:24},()=>({}));
  for(const e of src){
    if(e.delta<=0)continue;          /* netto negativ: nichts anzuzeigen */
    const h=new Date(e.from*1000).getHours();
    if(e.type==="skill"){up[h]+=e.delta; du[h][e.name]=(du[h][e.name]||0)+e.delta;}
    else                {dn[h]+=e.delta; dd[h][e.name]=(dd[h][e.name]||0)+e.delta;}
  }
  const div=sel?1:days;
  const mu=Math.max(...up)/div||1,md=Math.max(...dn)/div||1;

  hhead.textContent=sel
    ?L[LC].tod+" - "+new Date(sel).toLocaleDateString(loc)
    :L[LC].todAvg;

  const parts=(o,f)=>Object.entries(o).sort((a,b)=>b[1]-a[1])
                        .map(([n,v])=>n+" "+f(v/div)).join(", ");
  const W=960,H=150,PB=118,BW=W/24;
  let g="";
  for(let h=0;h<24;h++){
    const x=h*BW+2,w=BW-4;
    const hu=(up[h]/div/mu)*88,hd=(dn[h]/div/md)*20;
    const lab=clk(new Date(2000,0,1,h,0));

    if(hu>0.5)g+=`<rect x="${x}" y="${PB-30-hu}" width="${w}" height="${hu}"
      rx="2" fill="#3ddc7f" opacity="${.45+.55*(up[h]/div/mu)}"
      ><title>${lab} - ${parts(du[h],hrs)}</title></rect>`;
    if(hd>0.5)g+=`<rect x="${x}" y="${PB-28}" width="${w}" height="${hd}"
      rx="2" fill="#ff6b6b" opacity="${.5+.5*(dn[h]/div/md)}"
      ><title>${lab} - ${parts(dd[h],v=>(sel?v:Math.round(v*10)/10)+"x")}</title></rect>`;
    if(h%3===0)g+=`<text class="hx" x="${x+w/2}" y="${H-4}"
      text-anchor="middle">${pad(h)}</text>`;
  }
  hours.innerHTML=`<svg viewBox="0 0 ${W} ${H}" preserveAspectRatio="none">
    <line x1="0" y1="${PB-29}" x2="${W}" y2="${PB-29}" stroke="#23232e"/>
    ${g}</svg>`;
}

function drawDonut(box,legend,entries,pal,fmt){
  if(!entries.length){box.innerHTML="";legend.innerHTML=
    '<div class="empty">'+L[LC].noData+'</div>';return}
  const tot=entries.reduce((a,e)=>a+e[1],0)||1;
  const R=42,SW=16,Cc=2*Math.PI*R;
  let off=0,seg="",leg="";
  entries.forEach(([n,v],i)=>{
    const len=Cc*v/tot,col=pal[i%pal.length];
    seg+=`<circle cx="60" cy="60" r="${R}" fill="none" stroke="${col}"
      stroke-width="${SW}" stroke-dasharray="${len} ${Cc-len}"
      stroke-dashoffset="${-off}" transform="rotate(-90 60 60)"/>`;
    off+=len;
    leg+=`<div><i style="background:${col}"></i><span>${n}</span>
      <b>${fmt(v)} &middot; ${Math.round(v/tot*100)}%</b></div>`;
  });
  box.innerHTML=`<svg width="120" height="120" viewBox="0 0 120 120">
    <circle cx="60" cy="60" r="${R}" fill="none" stroke="#1c1c24"
      stroke-width="${SW}"/>${seg}</svg>`;
  legend.innerHTML=leg;
}

/* Bewusst aus /info und nicht aus dem Log: das Geraet fuehrt die
   vollstaendigen Summen, das Log kennt nur die Zeit seit seiner
   Einrichtung.                                                      */
/* Buchungsflaechen wie am Geraet: dieselben Spalten, dieselbe
   Schrittweite. Nach jedem Tap wird der Stand neu geholt.         */
async function book(item,delta){
  try{ await fetch("/book?item="+item+"&delta="+delta); }catch(e){}
  await tick();
}

function drawPanels(){
  bhead.textContent=L[LC].skills; bheadv.textContent=L[LC].vices2;

  if(!INFO){panelsS.innerHTML=panelsV.innerHTML="";return}
  if(!INFO.time_valid){
    panelsS.innerHTML=panelsV.innerHTML=
      '<div class="empty">'+L[LC].noclock3+'</div>';return}

  let a="";
  for(const o of (INFO.slots||[]))
    a+='<div class="pnl"><b>'+o.n+'</b><s>'+hrs(o.m)+'</s><div>'+
       '<button onclick="book(\'s'+o.i+'\',-1)">-</button>'+
       '<button class="p" onclick="book(\'s'+o.i+'\',1)">+0.5 h</button>'+
       '</div></div>';
  panelsS.innerHTML=a||'<div class="empty">'+L[LC].noData+'</div>';

  let b="";
  for(const o of (INFO.vslots||[]))
    b+='<div class="pnl v"><b>'+o.n+'</b><s>'+o.c+'x</s><div>'+
       '<button onclick="book(\'v'+o.i+'\',-1)">-</button>'+
       '<button class="p" onclick="book(\'v'+o.i+'\',1)">+1</button>'+
       '</div></div>';
  panelsV.innerHTML=b||'<div class="empty">'+L[LC].noVices+'</div>';
}

function drawPies(){
  dh1.textContent=L[LC].skills; dh2.textContent=L[LC].vices2;
  if(!INFO){drawDonut(pie1,leg1,[],PAL_S,hrs);
            drawDonut(pie2,leg2,[],PAL_V,v=>v+"x");return}
  const s1=(INFO.skills||[]).map(o=>[o.n,o.m]).sort((a,b)=>b[1]-a[1]);
  const s2=(INFO.vices ||[]).map(o=>[o.n,o.c]).sort((a,b)=>b[1]-a[1]);
  drawDonut(pie1,leg1,s1,PAL_S,hrs);
  drawDonut(pie2,leg2,s2,PAL_V,v=>v+"x");
}

function render(){
  const map=byDay();
  drawCal(map);drawDayStats(map);drawEvents(map);
  drawHours(map);drawPies();drawPanels();
}

async function tick(){
  try{ await pullInfo(); await pullEvents(); render(); }
  catch(e){ foot.textContent="Geraet nicht erreichbar"; }
}
tick();
setInterval(tick,10000);
</script></body></html>
)HTMLPAGE";
