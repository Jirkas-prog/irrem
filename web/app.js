"use strict";
const i18n=window.IRremI18n,t=i18n.t;
const $=id=>document.getElementById(id);
let catalog={revision:0,buttons:[],commands:[]},state={},selected=0,commandId=0,busy=false,eventSignature='',learnTarget=0,virtual=null;
const text=(tag,value,cls)=>{const e=document.createElement(tag);i18n.bindText(e,value);if(cls)e.className=cls;return e;};
function notice(message,error=false){i18n.bindText($('notice'),()=>(i18n.message(message)));$('notice').className=error?'error':'';$('notice').hidden=false;}
function confirmAction(message){const dialog=$('confirm-dialog');i18n.bindText($('confirm-message'),()=>(message));dialog.returnValue='';return new Promise(resolve=>{dialog.onclose=()=>resolve(dialog.returnValue==='confirm');dialog.showModal();});}
async function request(path,options={}) {
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),options.timeout||8000);
  try {
    const response=await fetch('/api/'+path,{cache:'no-store',...options,signal:controller.signal});
    if(!response.ok){let data;try{data=await response.json();}catch{}throw new Error(t(data?.error)||t("Chyba {0}",response.status));}
    return response;
  }catch(e){if(e.name==='AbortError')throw new Error(t('Zařízení neodpovědělo včas. Zkontrolujte připojení.'));throw e;}
  finally{clearTimeout(timer);}
}
async function api(path,data){return(await request(path,data===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json','X-IRRem':'1'},body:JSON.stringify(data)})).json();}
async function action(fn){if(busy)return;busy=true;try{await fn();}catch(e){notice(e.message,true);}finally{busy=false;}}
function route(){
  const aliases={'mqtt-settings':'mqtt',network:'wifi',remotes:'buttons',overview:'remote',live:'system'};
  const hash=location.hash.slice(1),page=aliases[hash]||hash||'remote';
  const actual=['remote','buttons','wifi','mqtt','system'].includes(page)?page:'remote';
  for(const el of document.querySelectorAll('.page'))el.hidden=el.id!=='page-'+actual;
  for(const link of document.querySelectorAll('.sidebar nav a')){if(link.hash==='#'+actual)link.setAttribute('aria-current','page');else link.removeAttribute('aria-current');}
  if(actual!=='remote')endVirtual();window.scrollTo(0,0);
}
window.addEventListener('hashchange',route);route();
function commandName(id){return catalog.commands.find(c=>c.id===id)?.name||t('Bez akce');}
function selectOptions(select,value){select.replaceChildren(i18n.option(()=>(t('— Bez akce —')),'0'),...catalog.commands.map(c=>i18n.option(()=>(c.name),String(c.id))));select.value=String(value||0);}
function renderCatalog(){
  i18n.bindText($('key-count'),()=>(`${catalog.buttons.length}/64`));i18n.bindText($('command-count'),()=>(`${catalog.commands.length}/64`));
  const query=$('map-search').value.trim().toLocaleLowerCase();
  $('button-list').replaceChildren(...catalog.buttons.filter(b=>(b.name+' '+b.key).toLocaleLowerCase().includes(query)).map(b=>{
    const el=text('button',()=>(''),'list-button');el.type='button';el.setAttribute('aria-pressed',String(b.id===selected));
    el.append(text('strong',()=>(b.name)),text('small',()=>(`${b.key?t('IR spárováno'):t('Pouze virtuální')} · ${commandName(b.single)}`)));el.onclick=()=>editButton(b.id);return el;
  }));
  $('command-list').replaceChildren(...catalog.commands.map(c=>{
    const el=text('button',()=>(''),'list-button');el.type='button';el.setAttribute('aria-pressed',String(c.id===commandId));el.append(text('strong',()=>(c.name)),text('small',()=>(c.code)));el.onclick=()=>editCommand(c.id);return el;
  }));
  if(!virtual){
    $('virtual-grid').replaceChildren(...catalog.buttons.map(b=>{
      const c=catalog.commands.find(c=>c.id===b.single),el=text('button',()=>(''),'virtual-key');el.type='button';el.dataset.id=b.id;
      if(c?.code==='off')el.classList.add('power-off');else if(b.name==='retez_on'||b.name==='TV_L_ON')el.classList.add('power-on');
      const labels={retez_on:t('Zapnout'),retez_off:t('Vypnout'),retez_dim_plus:t('Jas +'),retez_dim_minus:t('Jas −'),retez_timer_off:t('Bílá'),retez_timer_4h:'4H',retez_timer_6h:'6H',retez_timer_8h:'8H'};
      const name=labels[b.name]||(b.name.startsWith('retez_mode_')?t('Režim ')+b.name.slice(11):b.name);
      el.append(text('strong',()=>(name)),text('small',()=>(b.single?commandName(b.single):b.double?t('Dvojstisk: ')+commandName(b.double):b.long?t('Podržení: ')+commandName(b.long):t('Bez přiřazené akce'))));
      el.setAttribute('aria-label',b.name);el.disabled=!state.mqtt||!!state.learning;
      el.onpointerdown=e=>{if(e.button!==0)return;e.preventDefault();el.setPointerCapture(e.pointerId);startVirtual(b.id,el);};
      el.onpointerup=el.onpointercancel=()=>endVirtual();
      el.onkeydown=e=>{if([' ','Enter'].includes(e.key)){e.preventDefault();if(!e.repeat)startVirtual(b.id,el);}};
      el.onkeyup=e=>{if([' ','Enter'].includes(e.key)){e.preventDefault();endVirtual();}};
      el.oncontextmenu=e=>e.preventDefault();return el;
    }));
  }
  $('remote-empty').hidden=!!catalog.buttons.length;i18n.prune();
}
async function loadCatalog(){catalog=await api('catalog');renderCatalog();}
function editButton(id){
  if(learnTarget && learnTarget!==id){notice(t('Dokončete nebo zrušte probíhající párování.'),true);return;}
  selected=id;const b=catalog.buttons.find(b=>b.id===id);$('button-form').hidden=!b;$('choose-button').hidden=!!b;
  if(b){$('button-name').value=b.name;i18n.bindText($('button-key'),()=>(b.key||t('Nenapárováno · virtuální ovládání funguje i bez IR')));
    selectOptions($('binding-single'),b.single);selectOptions($('binding-double'),b.double);selectOptions($('binding-long'),b.long);
    $('long-seconds').value=b.long_ms/1000;$('double-seconds').value=b.double_ms/1000;$('repeat-seconds').value=b.repeat_ms/1000;$('clear-signal').disabled=!b.key;}
  updateGestures();renderCatalog();
}
function updateGestures(){
  const double=Number($('binding-double').value),hold=Number($('binding-long').value);
  const repeats=catalog.commands.find(c=>c.id===hold)?.code.startsWith('dim/');
  $('double-seconds').disabled=!double;$('long-seconds').disabled=!hold;$('repeat-seconds').disabled=!repeats;
  i18n.bindText($('gesture-hint'),()=>((double?t('Dvojstisk je zapnutý: jednoduchý stisk počká na uplynutí mezery. '):t('Dvojstisk je vypnutý: rychlé stisky se počítají jednotlivě. '))+(hold?t('Podržení je zapnuté; krátký stisk s jinou akcí se vyhodnotí po uvolnění. Dlouhé DIM se opakuje.'):t('Podržení je vypnuté: i dlouhé držení znamená jediný jednoduchý stisk.'))));
}
for(const id of ['binding-single','binding-double','binding-long'])$(id).onchange=updateGestures;
function buttonData(){return {id:selected,name:$('button-name').value.trim(),single:Number($('binding-single').value),double:Number($('binding-double').value),long:Number($('binding-long').value),long_ms:Math.round(Number($('long-seconds').value)*1000),double_ms:Math.round(Number($('double-seconds').value)*1000),repeat_ms:Math.round(Number($('repeat-seconds').value)*1000),revision:catalog.revision};}
async function saveButton(extra={}){if(!$('button-form').reportValidity())return;await api('buttons',{...buttonData(),...extra});await loadCatalog();editButton(selected);notice(t('Tlačítko uloženo.'));return true;}
$('new-button-form').onsubmit=e=>{e.preventDefault();action(async()=>{const name=$('new-button-name').value.trim();await api('buttons',{id:0,name,revision:catalog.revision});await loadCatalog();$('new-button-name').value='';editButton(catalog.buttons.find(b=>b.name===name).id);notice(t('Tlačítko vytvořeno. Teď můžete přiřadit funkce nebo IR signál.'));});};
$('button-form').onsubmit=e=>{e.preventDefault();action(()=>saveButton());};
$('map-search').oninput=renderCatalog;
$('refresh-catalog').onclick=()=>action(async()=>{await loadCatalog();editButton(selected);notice(t('Seznam obnoven.'));});
$('delete-button').onclick=()=>action(async()=>{if(!await confirmAction(t('Smazat vybrané tlačítko včetně jeho IR signálu a přiřazení?')))return;await api('buttons/delete',{id:selected,revision:catalog.revision});selected=0;await loadCatalog();editButton(0);notice(t('Tlačítko smazáno.'));});
$('clear-signal').onclick=()=>action(()=>saveButton({clear_signal:true}));
$('learn-button').onclick=()=>action(async()=>{await api('learn',{});learnTarget=selected;$('capture').hidden=false;$('save-signal').disabled=true;i18n.bindText($('learn-message'),()=>(t('Uvolněte ovladač a stiskněte vybrané tlačítko.')));i18n.bindText($('candidate'),()=>(''));});
$('cancel-learn').onclick=()=>action(async()=>{await api('learn/cancel',{});learnTarget=0;$('capture').hidden=true;});
$('save-signal').onclick=()=>action(async()=>{if(!state.candidate_id||selected!==learnTarget)throw new Error(t('Vyberte znovu párování tlačítka.'));if(await saveButton({candidate_id:state.candidate_id})){learnTarget=0;$('capture').hidden=true;}});
$('add-profile').onclick=()=>action(async()=>{await api('profile',{id:Number($('profile').value),revision:catalog.revision});await loadCatalog();notice(t('Předvolba přidána. Duplicitní signály byly přeskočeny; přiřaďte funkcím příkazy.'));});
for(const tab of ['buttons','commands'])$('tab-'+tab).onclick=()=>{for(const name of ['buttons','commands']){$(name+'-pane').hidden=name!==tab;$('tab-'+name).setAttribute('aria-selected',String(name===tab));}};
function commandCode(){const type=$('command-type').value,pct=$('command-brightness').value,k=$('command-kelvin').value;
  if(type==='white')return `white/${pct}/${k}`;
  if(type==='rgb'){const hex=$('command-color').value;return `rgb/${pct}/`+[1,3,5].map(i=>parseInt(hex.slice(i,i+2),16)).join('/');}
  return type==='custom'?$('command-custom').value.trim():type==='white-current'?'white':type;
}
function previewCommand(){const type=$('command-type').value;$('brightness-field').hidden=!['white','rgb'].includes(type);$('kelvin-field').hidden=type!=='white';$('rgb-field').hidden=type!=='rgb';$('custom-field').hidden=type!=='custom';i18n.bindText($('command-code'),()=>(commandCode()));}
function editCommand(id){commandId=id;const c=catalog.commands.find(c=>c.id===id);i18n.bindText($('command-title'),()=>(c?t('Upravit příkaz'):t('Nový příkaz')));$('delete-command').hidden=!c;$('command-name').value=c?.name||'';const parts=(c?.code||'white/36/3000').split('/');let type=parts[0];
  if(type==='white'&&parts.length===1)type='white-current';else if(type==='dim')type=c.code;
  $('command-type').value=type;$('command-brightness').value=Number(parts[1])||36;$('command-kelvin').value=Number(parts[2])||3000;
  if(type==='rgb')$('command-color').value='#'+parts.slice(2).map(n=>Number(n).toString(16).padStart(2,'0')).join('');
  $('command-custom').value=c?.code||'';previewCommand();renderCatalog();
}
$('new-command').onclick=()=>editCommand(0);
$('command-form').oninput=previewCommand;
$('command-form').onsubmit=e=>{e.preventDefault();action(async()=>{const name=$('command-name').value.trim();await api('commands',{id:commandId,name,code:commandCode(),revision:catalog.revision});await loadCatalog();editCommand(catalog.commands.find(c=>c.name===name).id);if(selected)editButton(selected);notice(t('Příkaz uložen. Node-RED není potřeba upravovat.'));});};
$('delete-command').onclick=()=>action(async()=>{if(!await confirmAction(t('Smazat příkaz? Příkaz přiřazený tlačítku smazat nelze.')))return;await api('commands/delete',{id:commandId,revision:catalog.revision});commandId=0;await loadCatalog();editCommand(0);notice(t('Příkaz smazán.'));});
function startVirtual(id,button){
  if((virtual&&!virtual.ending)||!state.mqtt||state.learning)return;
  const v={id,token:crypto.getRandomValues(new Uint32Array(1))[0]||1,button,chain:virtual?virtual.chain:Promise.resolve(),timer:0,ending:false,failed:false,waiting:0};virtual=v;button.classList.add('active');
  function queue(phase){++v.waiting;v.chain=v.chain.then(async()=>{if(v.failed)return;try{await api('virtual',{button_id:id,token:v.token,phase});}catch(e){v.failed=true;clearInterval(v.timer);button.classList.remove('active');if(virtual===v)virtual=null;notice(e.message,true);}finally{--v.waiting;}});}
  v.queue=queue;queue('press');v.timer=setInterval(()=>{if(!v.ending&&!v.waiting)queue('hold');},150);
}
function endVirtual(){const v=virtual;if(!v||v.ending)return;v.ending=true;clearInterval(v.timer);v.button.classList.remove('active');v.queue('release');v.chain.finally(()=>{if(virtual===v)virtual=null;});}
window.addEventListener('pointerup',endVirtual);window.addEventListener('pointercancel',endVirtual);window.addEventListener('blur',endVirtual);document.addEventListener('visibilitychange',()=>{if(document.hidden)endVirtual();});
function loadSettings(values){for(const form of document.querySelectorAll('.settings-form'))for(const el of form.elements){if(!el.name)continue;if(el.type==='password'||el.name.startsWith('clear_')){if(el.type==='checkbox')el.checked=false;else el.value='';continue;}if(values[el.name]===undefined)continue;if(el.type==='checkbox')el.checked=!!values[el.name];else el.value=values[el.name];}
  for(const name of ['wifi','mqtt','backup'])i18n.bindText($(name+'-secret'),()=>(values[name+'_password_set']?t('Heslo je uložené.'):t('Heslo není uložené.')));settingsPreviews();
}
function settingsPreviews(){const fields=name=>document.querySelector(`[name="${name}"]`);i18n.bindText($('hostname-preview'),()=>((fields('hostname').value||'irrem')+'.local'));
  for(const name of ['mqtt','backup']){$(name+'-ca-field').hidden=!fields(name+'_tls').checked;fields(name+'_ca').required=fields(name+'_tls').checked&&!!fields(name+'_host').value;}
  const base=fields('mqtt_topic').value||state.base_topic||'wifi2mqtt';for(const suffix of ['trigger','state','availability'])i18n.bindText($('preview-'+suffix),()=>(base+'/'+suffix));
}
for(const form of document.querySelectorAll('.settings-form')){
  form.oninput=settingsPreviews;form.onsubmit=e=>{e.preventDefault();action(async()=>{const data={revision:catalog.revision};for(const el of form.elements){if(!el.name)continue;data[el.name]=el.type==='checkbox'?el.checked:el.type==='number'?Number(el.value):el.value;}
    await api('settings',data);notice(t('Nastavení uloženo. Zařízení se restartuje.'));for(const el of form.elements)if(el.type==='password')el.value='';});};
}
$('mqtt-reconnect').onclick=()=>action(async()=>{await api('mqtt/reconnect',{});notice(t('Opětovné připojení vyžádáno. Rozepsané nastavení se neukládá.'));});
async function downloadBackup(scope){const response=await request('backup',{method:'POST',headers:{'Content-Type':'application/json','X-IRRem':'1'},body:JSON.stringify({scope})});const url=URL.createObjectURL(await response.blob()),a=document.createElement('a');a.href=url;a.download=`irrem-${scope}-${new Date().toISOString().slice(0,10)}.irrem`;document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),30000);notice(t('Záloha připravena ke stažení.'));}
for(const scope of ['buttons','system']){
  $('backup-'+scope).onclick=()=>action(()=>downloadBackup(scope));
  $('restore-'+scope).onchange=e=>action(async()=>{const file=e.target.files[0];if(!file)return;try{
    if(file.size>100000)throw new Error(t('Soubor zálohy je příliš velký.'));const body=await file.arrayBuffer();
    const options={method:'POST',headers:{'Content-Type':'application/octet-stream','X-IRRem':'1','X-IRrem-Preview':'1'},body};
    const preview=await(await request('restore',options)).json();if(preview.scope!==scope)throw new Error(t('Tento soubor patří do obnovy ')+(preview.scope==='system'?t('celého systému.'):t('tlačítek.')));
    if(!await confirmAction(t("Obnovit {0} tlačítek a {1} příkazů? {2}",preview.buttons,preview.commands,scope==='system'?t('Nahradí se i síťová nastavení a zařízení se restartuje.'):t('Současný seznam bude nahrazen.'))))return;
    options.headers['X-IRrem-Preview']='0';options.headers['X-IRrem-Revision']=String(preview.revision);await request('restore',options);
    if(scope==='buttons'){selected=0;await loadCatalog();editButton(0);}notice(scope==='system'?t('Systém obnoven. Zařízení se restartuje.'):t('Tlačítka a příkazy obnoveny.'));
  }finally{e.target.value='';}});
}
$('firmware-file').onchange=()=>{$('update-firmware').disabled=!$('firmware-file').files.length||state.ota_available===false;};
$('update-firmware').onclick=()=>action(async()=>{const file=$('firmware-file').files[0];if(!file)return;if(!await confirmAction(t("Nahrát {0} jako firmware IRrem? Během nahrávání neodpojujte napájení.",file.name)))return;
  $('update-progress').hidden=false;$('update-progress').value=0;i18n.bindText($('update-status'),()=>(t('Nahrávám firmware…')));
  await new Promise((resolve,reject)=>{const x=new XMLHttpRequest();x.open('POST','/api/update');x.setRequestHeader('X-IRRem','1');x.setRequestHeader('Content-Type','application/octet-stream');x.timeout=120000;x.upload.onprogress=e=>{if(e.lengthComputable)$('update-progress').value=e.loaded/e.total*100;};x.onload=()=>{let data;try{data=JSON.parse(x.responseText);}catch{}if(x.status>=200&&x.status<300)resolve();else reject(new Error(data?.error||t('Aktualizace selhala.')));};x.onerror=()=>reject(new Error(t('Spojení se přerušilo. Ověřte verzi v systému.')));x.ontimeout=()=>reject(new Error(t('Vypršel čas aktualizace. Ověřte verzi v systému.')));x.send(file);});
  i18n.bindText($('update-status'),()=>(t('Firmware ověřen. Zařízení se restartuje.')));notice(t('Aktualizace úspěšná, probíhá restart.'));
});
$('restart').onclick=()=>action(async()=>{await api('restart',{});notice(t('Zařízení se restartuje.'));});
function renderDiagnostics(s){
  $('diagnostics').replaceChildren(...Object.entries({'IP':s.ip,'MQTT':s.mqtt?t('Připojeno'):t('Odpojeno'),[t('Čas NTP')]:s.time_synced?t('Synchronizován'):t('Čeká'),[t('Volná RAM')]:Math.round(s.free_heap/1024)+' kB',[t('Přijaté IR rámce')]:s.received_frames,[t('Odeslané příkazy')]:s.sent_events,[t('Neodeslané příkazy')]:s.dropped_events,[t('Poslední IR kód')]:s.last_key||'—',[t('OTA oddíl')]:s.ota_slot||'—'}).flatMap(([k,v])=>[text('dt',()=>(k)),text('dd',()=>(String(v)))]));
}
function renderState(s){
  const oldBoot=state.boot_id;state=s;i18n.bindText($('connection'),()=>(t('Zařízení připojeno')));$('connection').classList.remove('offline');i18n.bindText($('header-host'),()=>(s.hostname+'.local'));i18n.bindText($('version'),()=>('IRrem '+s.version));
  i18n.bindText($('remote-status'),()=>(s.learning?t('Párování IR'):s.mqtt?t('MQTT připojeno'):t('MQTT odpojeno')));$('pulse').classList.toggle('active',s.active||!!virtual);
  for(const button of document.querySelectorAll('.virtual-key'))button.disabled=!s.mqtt||s.learning;
  const last=s.events?.[0];i18n.bindText($('last-action'),()=>(last?`${last.button} → ${last.command_name||last.command} · ${{single:t('stisk'),double:t('dvojstisk'),long:t('podržení'),repeat:t('opakování')}[last.gesture]||last.gesture}`:t('Připraveno')));i18n.bindText($('last-time'),()=>(last?.timestamp?new Date(last.timestamp).toLocaleTimeString(i18n.locale):''));
  i18n.bindText($('wifi-status'),()=>(s.wifi?`${s.ip} · ${s.rssi} dBm`:t('Čekám na Wi-Fi')));i18n.bindText($('mqtt-detail'),()=>(t(s.mqtt_detail)));i18n.bindText($('mqtt-endpoint'),()=>(`${s.broker_host||t('Broker není nastaven')}:${s.broker_port} · ${s.broker_ip||t('bez IP')}`));$('mqtt-dot').classList.toggle('online',s.mqtt);
  renderDiagnostics(s);
  if(learnTarget){i18n.bindText($('learn-message'),()=>(t(s.learn_message)||t('Čekám na signál…')));i18n.bindText($('candidate'),()=>(s.candidate_key||''));$('save-signal').disabled=!s.candidate_id||s.learning;}
  const signature=s.boot_id+':'+(last?.sequence||0);if(signature!==eventSignature){eventSignature=signature;$('event-log').replaceChildren(...(s.events||[]).map(e=>{const el=text('div',()=>(''),'event');el.append(text('small',()=>(e.timestamp?new Date(e.timestamp).toLocaleTimeString(i18n.locale):'—')),text('span',()=>(`${e.button} · ${e.gesture} · ${e.source}`)),text('code',()=>(e.command)));return el;}));}
  if(s.ota_available===false)i18n.bindText($('update-status'),()=>(t('OTA vyžaduje první instalaci rozdělení flash přes USB.')));
  i18n.prune();
  if(oldBoot&&oldBoot!==s.boot_id){notice(t('Zařízení je znovu online.'));initialize().catch(e=>notice(e.message,true));}
}
async function poll(){try{renderState(await api('state'));}catch(e){i18n.bindText($('connection'),()=>(t('Spojení přerušeno')));$('connection').classList.add('offline');state.mqtt=false;for(const b of document.querySelectorAll('.virtual-key'))b.disabled=true;endVirtual();}finally{setTimeout(poll,document.hidden?2500:learnTarget?250:600);}}
async function initialize(){const [c,s,p]=await Promise.all([api('catalog'),api('settings'),api('profiles')]);catalog=c;loadSettings(s);renderCatalog();editCommand(0);if(selected)editButton(selected);$('profile').replaceChildren(...p.profiles.map(x=>i18n.option(()=>(t("{0} · {1} tlačítek",t(x.name),x.count)),String(x.id))));}
initialize().catch(e=>notice(e.message,true));poll();

// Rebuild display-only lists; never reload settings or replace form values on a language change.
document.addEventListener('irrem-languagechange',()=>{
  const held=virtual;endVirtual();
  if(held)held.chain.finally(renderCatalog);else renderCatalog();
  if(state.boot_id)renderDiagnostics(state);
  updateGestures();
  i18n.bindText($('command-title'),()=>t(commandId?'Upravit příkaz':'Nový příkaz'));
  i18n.prune();
});
