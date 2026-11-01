const fs=require('node:fs'),http=require('node:http'),path=require('node:path');
const web=path.resolve(__dirname,'../web');
const settings={name:'IRrem',hostname:'irrem',ssid:'Domácí Wi-Fi',ntp:'pool.ntp.org',mqtt_enabled:true,discovery_enabled:true,mqtt_host:'homeassistant.local',mqtt_port:1883,mqtt_user:'irrem',mqtt_tls:false,mqtt_ca:'',backup_host:'',backup_port:1883,backup_user:'',backup_tls:false,backup_ca:'',mqtt_client_id:'',mqtt_topic:'wifi2mqtt',discovery_prefix:'homeassistant',birth_topic:'homeassistant/status',keep_alive:15,reconnect_seconds:5,socket_timeout:2,release_ms:180,hold_ms:100,wifi_password_set:true,mqtt_password_set:true,backup_password_set:false,screen:true,screen_timeout:0};
const commands=[{id:1,name:'Zapnout · bílá 36 %',code:'white/36/3000'},{id:2,name:'Vypnout relé',code:'off'},{id:3,name:'Jas +',code:'dim/up'},{id:4,name:'Jas −',code:'dim/down'},{id:5,name:'Bílá · zachovat jas',code:'white'},...[1,2,5,10,36,60,80,100].map((n,i)=>({id:6+i,name:`Bílá ${n} %`,code:`white/${n}/3000`})),...[5,20,30].map((n,i)=>({id:14+i,name:`Teplá RGB ${n} %`,code:`rgb/${n}/255/180/100`}))];
const names=['retez_on','retez_off',...Array.from({length:8},(_,i)=>'retez_mode_'+(i+1)),'retez_timer_4h','retez_timer_6h','retez_timer_8h','retez_timer_off','retez_dim_minus','retez_dim_plus','TV_L_ON','TV_L_OFF','TV_L_100','TV_L_001'];
let catalog={revision:1,commands,buttons:names.map((name,i)=>({id:17+i,name,key:`NEC:32:${i.toString(16).padStart(16,'0')}`,single:[1,2,6,7,8,9,10,11,12,13,14,15,16,5,4,3,1,2,13,6][i],double:0,long:i===14?4:i===15?3:0,long_ms:1000,double_ms:350,repeat_ms:350,raw:false}))};
let nextId=37,sequence=0,virtual=null;
const state={version:'0.3.0',device_id:'irrem-test',boot_id:'a1b2c3d4',base_topic:'wifi2mqtt',hostname:'irrem',wifi:true,ip:'192.0.2.54',rssi:-51,mqtt:true,mqtt_detail:'Připojeno',broker_host:'homeassistant.local',broker_port:1883,broker_ip:'192.0.2.234',free_heap:96000,time_synced:true,ota_available:true,ota_slot:'ota_0',events:[],received_frames:800,sent_events:20,dropped_events:0,learning:false,candidate_id:0,candidate_key:'',last_key:'RC5:12:0000000000000077'};
const log=[];
function event(button){const c=commands.find(c=>c.id===button.single);if(!c)return;state.events.unshift({sequence:++sequence,button:button.name,command:c.code,command_name:c.name,gesture:'single',source:'web',timestamp:new Date().toISOString()});state.events=state.events.slice(0,12);}
const server=http.createServer(async(req,res)=>{
 try{
  const route=new URL(req.url,'http://localhost').pathname;
  if(!route.startsWith('/api/')&&route!=='/__test'){
   const file={'/':'index.html','/app.js':'app.js','/style.css':'style.css'}[route];if(!file){res.writeHead(404);res.end();return;}
   res.setHeader('Content-Type',file.endsWith('.html')?'text/html; charset=utf-8':file.endsWith('.css')?'text/css':'application/javascript');res.end(fs.readFileSync(path.join(web,file)));return;
  }
  const chunks=[];for await(const c of req)chunks.push(c);const bytes=Buffer.concat(chunks);const body=bytes.length&&(req.headers['content-type']||'').startsWith('application/json')?JSON.parse(bytes):{};
  let data={},status=200;const op=route.slice(5);
  if(route==='/__test'){if(req.method==='POST')Object.assign(state,body);data={state,catalog,log};}
  else if(req.method==='GET'){
   if(op==='state')data={...state,revision:catalog.revision,key_count:catalog.buttons.length,command_count:commands.length};
   else if(op==='settings')data={...settings,revision:catalog.revision};
   else if(op==='catalog')data=catalog;
   else if(op==='profiles')data={profiles:[{id:0,name:'RGB 24 tlačítek',count:24}]};else status=404;
  }else{
   if(req.headers['x-irrem']!=='1')throw new Error('Missing custom header');log.push({op,body,size:bytes.length});
   if(op==='virtual'){const b=catalog.buttons.find(b=>b.id===body.button_id);if(body.phase==='press'){virtual=b;event(b);}else if(body.phase==='release')virtual=null;}
   else if(op==='learn')Object.assign(state,{learning:true,candidate_id:0,learn_message:'Uvolněte ovladač a stiskněte vybrané tlačítko.'});
   else if(op==='learn/cancel')Object.assign(state,{learning:false,candidate_id:0});
   else if(op==='backup'){res.setHeader('Content-Type','application/octet-stream');res.end(Buffer.from('IRREM03\0fixture'));return;}
   else if(op==='restore'){data={scope:'buttons',buttons:catalog.buttons.length,commands:commands.length,revision:catalog.revision};}
   else if(op==='settings'){Object.assign(settings,body);++catalog.revision;}
   else if(op==='buttons'||op==='buttons/delete'||op==='commands'||op==='commands/delete'){
    if(body.revision!==catalog.revision){status=409;data={error:'Nastavení se mezitím změnilo.'};}
    else{
     const list=op.startsWith('buttons')?catalog.buttons:commands;
     const i=list.findIndex(x=>x.id===body.id);
     if(op.endsWith('/delete'))list.splice(i,1);
     else if(!body.id)list.push({id:nextId++,key:'',single:0,double:0,long:0,long_ms:1000,double_ms:350,repeat_ms:350,...body,id:nextId-1});
     else Object.assign(list[i],body,body.candidate_id?{key:state.candidate_key}:body.clear_signal?{key:''}:{});
     if(body.candidate_id)state.candidate_id=0;
     ++catalog.revision;
    }
   }
  }
  res.writeHead(status,{'Content-Type':'application/json','Cache-Control':'no-store'});res.end(JSON.stringify(data));
 }catch(e){res.writeHead(400,{'Content-Type':'application/json'});res.end(JSON.stringify({error:e.message}));}
});
server.listen(18739,'127.0.0.1',()=>console.log('IRrem web fixture: http://127.0.0.1:18739 (no real devices)'));
