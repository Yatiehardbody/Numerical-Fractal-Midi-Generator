// Presentation only. No generated event data or engine settings are changed.
const nmgTheme={h:90,s:.45,v:.93,
 rgb(h,s,v){const f=n=>{const k=(n+h/60)%6;return Math.round(255*(v-v*s*Math.max(0,Math.min(k,4-k,1))));};return [f(5),f(3),f(1)];},
 hsv(r,g,b){r/=255;g/=255;b/=255;const v=Math.max(r,g,b),d=v-Math.min(r,g,b);return [d?((v===r?(g-b)/d+(g<b?6:0):v===g?(b-r)/d+2:(r-g)/d+4)*60):0,v?d/v:0,v];},
 apply(hex){if(!/^#[0-9a-f]{6}$/i.test(hex))hex='#c3ee83';const rgb=hex.slice(1).match(/../g).map(x=>parseInt(x,16));[this.h,this.s,this.v]=this.hsv(...rgb);const root=document.documentElement;
 root.style.setProperty('--accent',hex);root.style.setProperty('--accent-ink',rgb.reduce((a,v,i)=>a+v*[.299,.587,.114][i],0)>145?'#14200d':'#ffffff');
 for(const [key,h,s,l] of NMG_THEME_COLORS){if(hex.toLowerCase()==='#c3ee83')root.style.removeProperty(key);else root.style.setProperty(key,`hsl(${this.h} ${Math.min(100,s*Math.min(1.6,this.s/.45))}% ${l}%)`);}
 document.getElementById('themeSV').style.backgroundColor=`hsl(${this.h} 100% 50%)`;
 document.getElementById('themeCursor').style.left=this.s*100+'%';document.getElementById('themeCursor').style.top=(1-this.v)*100+'%';
 document.getElementById('themeHue').value=this.h;document.getElementById('themeHex').value=hex.slice(1).toUpperCase();document.getElementById('themeHex').style.background=hex;document.getElementById('themeHex').style.color=root.style.getPropertyValue('--accent-ink');
 ['R','G','B'].forEach((c,i)=>{document.getElementById('theme'+c).value=rgb[i];document.getElementById('theme'+c+'Value').textContent=rgb[i].toString(16).toUpperCase().padStart(2,'0');});
 },
 choose(hex){this.apply(hex);nmgBridge.plugin.theme=document.getElementById('themeHex').value;nmgBridge.saveConfig();},
 fromHSV(){this.choose('#'+this.rgb(this.h,this.s,this.v).map(x=>x.toString(16).padStart(2,'0')).join(''));},
 setup(){const $=id=>document.getElementById(id),dialog=$('themeDialog');
 $('nmgTheme').onclick=()=>{dialog.showModal();nmgBridge.dragBounds();};$('themeClose').onclick=()=>dialog.close();dialog.addEventListener('close',()=>nmgBridge.dragBounds());dialog.onclick=e=>{if(e.target===dialog){const r=dialog.getBoundingClientRect();if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom)dialog.close();}};
 $('themeHex').onchange=e=>{const text=e.target.value.replace(/^#/,'');if(/^[0-9a-f]{6}$/i.test(text))this.choose('#'+text);else this.apply('#'+(nmgBridge.plugin.theme||'c3ee83'));};
 $('themeReset').onclick=()=>this.choose('#c3ee83');$('themeHue').oninput=e=>{this.h=Number(e.target.value);this.fromHSV();};
 for(const c of ['R','G','B'])$('theme'+c).oninput=()=>this.choose('#'+['R','G','B'].map(c=>Number($('theme'+c).value).toString(16).padStart(2,'0')).join(''));
 const sv=$('themeSV');const point=e=>{const r=sv.getBoundingClientRect();this.s=Math.max(0,Math.min(1,(e.clientX-r.left)/r.width));this.v=1-Math.max(0,Math.min(1,(e.clientY-r.top)/r.height));this.fromHSV();};
 sv.onpointerdown=e=>{sv.setPointerCapture(e.pointerId);point(e);};sv.onpointermove=e=>{if(sv.hasPointerCapture(e.pointerId))point(e);};sv.onkeydown=e=>{if(!['ArrowLeft','ArrowRight','ArrowUp','ArrowDown'].includes(e.key))return;e.preventDefault();this.s=Math.max(0,Math.min(1,this.s+(e.key==='ArrowRight'?.02:e.key==='ArrowLeft'?-.02:0)));this.v=Math.max(0,Math.min(1,this.v+(e.key==='ArrowUp'?.02:e.key==='ArrowDown'?-.02:0)));this.fromHSV();};
 }
};
