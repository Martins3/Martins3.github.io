// Local integration checks. Start an isolated Chromium/Edge with remote debugging first.
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
async function main() {
 const source = path.join(__dirname, 'README.md');
 const hash = () => crypto.createHash('sha256').update(fs.readFileSync(source)).digest('hex');
 const before = hash(); require('./build.cjs');
 assert.equal(hash(), before, 'Build must not modify Markdown');
 const md = fs.readFileSync(source, 'utf8'); let links = 0;
 for (const match of md.matchAll(/\]\((\.\.\/[^)]+)\)/g)) {
  assert.ok(fs.existsSync(path.resolve(__dirname, match[1].split('#')[0])), match[1]); links++;
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
  const initial = await evaluate(`({headings:document.querySelectorAll('#document h2').length,tables:document.querySelectorAll('#document table').length,media:media.length,image:$('picture').naturalWidth,text:document.querySelector('h1').textContent})`);
  assert.equal(initial.headings,6); assert.equal(initial.tables,3); assert.ok(initial.media>30); assert.ok(initial.image>0);console.log('PASS: continuous Markdown rendering, tables, image loading.',initial);
  const artifact=path.join(os.tmpdir(),'vn-lesson-v2-preview');fs.mkdirSync(artifact,{recursive:true});
  const screenshot=await send('Page.captureScreenshot',{format:'png'});fs.writeFileSync(path.join(artifact,'verified.png'),Buffer.from(screenshot.data,'base64'));
  assert.ok(await evaluate(`$('search').value='疯猪';$('search').oninput();document.querySelectorAll('#document section:not([hidden])').length>0 && document.querySelectorAll('#document section[hidden]').length>0`));
  assert.ok(await evaluate(`$('search').value='不存在的测试文字';$('search').oninput();!$('empty').hidden`));
  await evaluate(`$('search').value='';$('search').oninput();pane(false)`);assert.ok(await evaluate(`$('layout').classList.contains('text-only')`));
  assert.ok(await evaluate(`document.querySelector('[data-media]').click();!$('layout').classList.contains('text-only')`));
  await evaluate(`showMedia(media.findIndex(m=>m.type==='clip'),false);new Promise(resolve=>setTimeout(resolve,800))`);
  assert.ok(await evaluate(`!$('video').hidden && $('video').readyState>=1 && Math.abs($('video').currentTime-media[current].start)<1`));
  assert.ok(await evaluate(`$('video').currentTime=media[current].end;$('video').ontimeupdate();$('video').paused && segment===null`));
  assert.ok(await evaluate(`showMedia(0);$('picture').click();$('zoom').open`));await evaluate(`$('zoom').close()`);
  assert.ok(await evaluate(`!renderMarkdown('<script>alert(1)</script>').html.includes('<script>') && !inline('[bad](javascript:alert)').includes('href="javascript:')`));
  await send('Emulation.setDeviceMetricsOverride',{width:390,height:844,deviceScaleFactor:1,mobile:true});
  assert.ok(await evaluate(`pane(false);document.documentElement.scrollWidth<=innerWidth+1`));
  console.log('PASS: search, empty state, pane reopen, video seek/end pause, image zoom, safe rendering, narrow layout.');
  console.log('Screenshot:',path.join(artifact,'verified.png'));
 } finally { await send('Page.close').catch(()=>{}); socket.close(); }
}
main().catch(error=>{console.error(error);process.exitCode=1;});
