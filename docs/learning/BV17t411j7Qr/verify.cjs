// Local integration checks. Start an isolated Chromium/Edge with remote debugging first.
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
async function main() {
 const source = path.join(__dirname, '教程.md');
 const hash = () => crypto.createHash('sha256').update(fs.readFileSync(source)).digest('hex');
 const before = hash(); require('./build.cjs');
 assert.equal(hash(), before, 'Build must not modify Markdown');
 const md = fs.readFileSync(source, 'utf8'); let links = 0;
 for (const match of md.matchAll(/\]\((\.\.\/[^)]+)\)/g)) {
  const file=path.resolve(__dirname,match[1].split('#')[0]);assert.ok(fs.existsSync(file),match[1]);links++;
  const clip=match[1].match(/#t=(\d+),(\d+)$/);if(clip){const probe=JSON.parse(fs.readFileSync(path.join(path.dirname(file),'probe.json'),'utf8').replace(/^\uFEFF/,''));assert.ok(Number(clip[1])<Number(clip[2])&&Number(clip[2])<=Number(probe.format.duration)+1,match[1]);}
 }
 const html = fs.readFileSync(path.join(__dirname, 'index.html'), 'utf8');
 assert.ok(!/localStorage|<textarea|lesson\.json/.test(html), 'No browser writing or JSON content source');
 console.log(`PASS: Markdown unchanged; ${links} local links; read-only page.`);
 const port = Number(process.argv[2] || 9226);
 const target = await fetch(`http://127.0.0.1:${port}/json/new?${encodeURIComponent('file:///'+path.join(__dirname,'index.html').replace(/\\/g,'/'))}`, {method:'PUT'}).then(r=>r.json());
 const socket = new WebSocket(target.webSocketDebuggerUrl); let serial=0; const pending=new Map();
 socket.onmessage=e=>{const result=JSON.parse(e.data);if(!result.id)return;const entry=pending.get(result.id);pending.delete(result.id);if(result.error)entry.reject(Error(result.error.message));else entry.resolve(result.result);};
 await new Promise((resolve,reject)=>{socket.onopen=resolve;socket.onerror=reject;});
 const send=(method,params={})=>new Promise((resolve,reject)=>{const id=++serial;pending.set(id,{resolve,reject});socket.send(JSON.stringify({id,method,params}));});
 const evaluate=async expression=>{const result=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(result.exceptionDetails)throw Error(JSON.stringify(result.exceptionDetails));return result.result.value;};
 try {
  await send('Page.enable');
  await send('Emulation.setDeviceMetricsOverride',{width:1600,height:1100,deviceScaleFactor:1,mobile:false});
  await evaluate('new Promise(resolve=>setTimeout(resolve,500))');
  const initial = await evaluate(`({headings:document.querySelectorAll('#document h2').length,tables:document.querySelectorAll('#document table').length,media:media.length,clips:media.every(m=>m.type==='clip'),text:document.querySelector('h1').textContent})`);
  assert.equal(initial.headings,16); assert.ok(initial.tables>=7); assert.ok(initial.media>30); assert.ok(initial.clips);console.log('PASS: continuous Markdown rendering, tables, video-only entries.',initial);
  assert.equal(await evaluate(`$('part').options.length`),15);
  assert.ok(await evaluate(`$('toc').querySelectorAll('.topics a').length>=90 && Array.from($('toc').querySelectorAll('a')).every(a=>document.getElementById(a.hash.slice(1)))`));
  assert.ok(await evaluate(`$('toc').querySelector('summary a').click();$('toc').querySelector('details').open`));
  assert.ok(await evaluate(`Array.from(document.querySelectorAll('#document a[href^="#part-"]')).every(a=>document.getElementById(a.hash.slice(1)))`));
  for(let part=1;part<=15;part++){
   const id=String(part).padStart(2,'0');
   assert.ok(await evaluate(`$('part').value='${id}';$('part').onchange();new Promise(resolve=>{const until=Date.now()+10000;const check=()=>{if($('video').readyState>=1){$('video').pause();resolve($('video').currentSrc.includes('/p${id}/')&&Number.isFinite($('video').duration));}else if(Date.now()>until)resolve(false);else setTimeout(check,100);};check();})`),'P'+id+' playback');
  }
  console.log('PASS: all 15 P selectable with readable video metadata.');
  await evaluate(`showMedia(media.findIndex(m=>m.start>0));new Promise(resolve=>setTimeout(resolve,200))`);
  const artifact=path.join(os.tmpdir(),'vn-badminton-preview');fs.mkdirSync(artifact,{recursive:true});
  const screenshot=await send('Page.captureScreenshot',{format:'png'});fs.writeFileSync(path.join(artifact,'verified.png'),Buffer.from(screenshot.data,'base64'));
  assert.ok(await evaluate(`$('search').value='联防';$('search').oninput();document.querySelectorAll('#document section:not([hidden])').length>0 && document.querySelectorAll('#document section[hidden]').length>0`));
  assert.ok(await evaluate(`$('search').value='不存在的测试文字';$('search').oninput();!$('empty').hidden`));
  assert.ok(await evaluate(`$('search').value='最后平带切向斜线';$('search').oninput();$('empty').hidden && document.querySelectorAll('#document h3:not([hidden])').length===1 && !document.querySelector('#part-04').closest('section').querySelector('a[href*="bilibili.com"]').closest('.block').hidden`));
  assert.ok(await evaluate(`$('search').value='掌心与拍柄';$('search').oninput();!document.querySelector('#part-01').closest('section').querySelector('[data-media].media-link.clip').closest('.block').hidden`));
  await evaluate(`$('search').value='';$('search').oninput();pane(false)`);assert.ok(await evaluate(`$('layout').classList.contains('text-only')`));
  assert.ok(await evaluate(`document.querySelector('[data-media].media-link.clip').click();!$('layout').classList.contains('text-only')`));
  await evaluate(`showMedia(media.findIndex(m=>m.type==='clip'&&m.start>0),false);new Promise(resolve=>setTimeout(resolve,800))`);
  assert.ok(await evaluate(`!$('video').hidden && $('video').readyState>=1 && Math.abs($('video').currentTime-media[current].start)<1`));
  assert.ok(await evaluate(`$('video').currentTime=media[current].end;$('video').ontimeupdate();$('video').paused && segment===null`));
  assert.ok(await evaluate(`const before=media[current].start;playback.value='bilibili';playback.onchange();new URL(iframe.src).searchParams.get('t')===String(before)&&$('video').hidden&&!$('video').hasAttribute('src')&&$('loop-label').hidden`));
  assert.ok(await evaluate(`playback.value='local';playback.onchange();iframe.hidden&&!iframe.hasAttribute('src')&&!$('video').hidden`));
  assert.ok(await evaluate(`!renderMarkdown('<script>alert(1)</script>').html.includes('<script>') && !inline('[bad](javascript:alert)').includes('href="javascript:')`));
  await send('Emulation.setDeviceMetricsOverride',{width:390,height:844,deviceScaleFactor:1,mobile:true});
  assert.ok(await evaluate(`pane(false);document.documentElement.scrollWidth<=innerWidth+1`));
  console.log('PASS: search, empty state, pane reopen, video seek/end pause, global playback switch, safe rendering, narrow layout.');
  console.log('Screenshot:',path.join(artifact,'verified.png'));
 } finally { await send('Page.close').catch(()=>{}); socket.close(); }
}
main().catch(error=>{console.error(error);process.exitCode=1;});

