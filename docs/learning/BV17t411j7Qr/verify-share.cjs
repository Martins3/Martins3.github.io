const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
async function main() {
  const original = fs.readFileSync(path.join(__dirname, '教程.md'));
  process.argv.push('--share');require('./build.cjs');
  assert.deepEqual(fs.readFileSync(path.join(__dirname, '教程.md')), original);
  const files = fs.readdirSync(path.join(__dirname, 'share'), {recursive:true});
  assert.ok(!files.some(name => name.endsWith('.mp4')));
  const port = Number(process.argv[2] || 9226);
  const url = 'file:///' + path.join(__dirname, 'share/index.html').replace(/\\/g, '/');
  const target = await fetch(`http://127.0.0.1:${port}/json/new?${encodeURIComponent(url)}`, {method:'PUT'}).then(r=>r.json());
  const socket = new WebSocket(target.webSocketDebuggerUrl), pending = new Map(); let serial = 0;
  socket.onmessage = event => { const response=JSON.parse(event.data); if(!response.id)return; const entry=pending.get(response.id);pending.delete(response.id);response.error?entry.reject(Error(response.error.message)):entry.resolve(response.result); };
  await new Promise((resolve,reject)=>{socket.onopen=resolve;socket.onerror=reject;});
  const send=(method,params={})=>new Promise((resolve,reject)=>{const id=++serial;pending.set(id,{resolve,reject});socket.send(JSON.stringify({id,method,params}));});
  const evaluate=async expression=>{const result=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});assert.ok(!result.exceptionDetails,JSON.stringify(result.exceptionDetails));return result.result.value;};
  try {
    await evaluate('new Promise(resolve=>setTimeout(resolve,500))');
    assert.ok(await evaluate(`playback.value==='bilibili' && media.every(m=>m.type==='clip') && !$('video').getAttribute('src')`));
    assert.ok(await evaluate(`media.every(m=>{const r=sourceReference(m);return r&&r.part>=1&&r.part<=15&&r.time>=0;})`));
    for(let part=1;part<=15;part++) {
      assert.ok(await evaluate(`showMedia(fullParts[${part-1}].index);new URL(iframe.src).searchParams.get('p')==='${part}' && $('video').hidden && !$('video').getAttribute('src')`));
    }
    assert.ok(await evaluate(`const index=media.findIndex(m=>m.type==='clip'&&m.start>0);showMedia(index);new URL(iframe.src).searchParams.get('t')===String(media[index].start) && sourceLink.href===sourceReference(media[index]).page`));
    assert.ok(await evaluate(`playback.value='local';playback.onchange();new Promise(resolve=>{const until=Date.now()+10000;const check=()=>{if($('video').readyState>=1)resolve(iframe.hidden&&!iframe.hasAttribute('src')&&!$('video').hidden&&Math.abs($('video').currentTime-media[current].start)<1);else if(Date.now()>until)resolve(false);else setTimeout(check,100);};check();})`));
    assert.ok(await evaluate(`playback.value='bilibili';playback.onchange();!iframe.hidden&& !$('video').hasAttribute('src') && (pane(false),!iframe.hasAttribute('src'))`));
    console.log('PASS: source Markdown unchanged; video-only entries; no MP4; global source selection; all 15 P and clip timestamps map to Bilibili; iframe unloads when closed.');
  } finally {await send('Page.close').catch(()=>{});socket.close();}
}
main().catch(error=>{console.error(error);process.exitCode=1;});
