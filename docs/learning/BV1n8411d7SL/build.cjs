const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const base = __dirname;
function build() {
  let markdown = fs.readFileSync(path.join(base, 'README.md'), 'utf8');
  const publish = process.argv.includes('--publish');
  const share = publish || process.argv.includes('--share');
  const output = share && !publish ? path.join(base, 'share') : base;
  fs.mkdirSync(output, { recursive: true });
  if (share && !publish) {
    // The export is one directory deeper; keep local mode usable on this machine.
    if (!publish) markdown = markdown.replace(/\]\((\.\.\/[^)]+\.mp4#t=[^)]+)\)/g, (_, url) => `](../${url})`);
    // This is an exported copy; the editable Markdown remains untouched.
    fs.writeFileSync(path.join(output, 'README.md'), markdown.replace(/\]\([^)]*\/(BV\w+)\/p(\d+)\/video\.mp4#t=(\d+(?:\.\d+)?),[^)]+\)/g,
      (_, bvid, part, time) => `](https://www.bilibili.com/video/${bvid}/?p=${Number(part)}&t=${time})`));
  }
  const template = fs.readFileSync(path.join(base, 'template.html'), 'utf8');
  if (!template.includes('/* MARKDOWN_SOURCE */')) throw Error('Missing source slot');
  const source = JSON.stringify(markdown).replace(/</g, '\\u003c').replace(/\u2028/g, '\\u2028').replace(/\u2029/g, '\\u2029');
  let html = template.replace('/* MARKDOWN_SOURCE */', `const markdown = ${source};`);
  if (share) html = html.replace("const defaultOnline=location.protocol!=='file:';", 'const defaultOnline=true;');
  new vm.Script(html.match(/<script>([\s\S]*?)<\/script>/)[1]);
  fs.writeFileSync(path.join(output, 'index.html'), html);
  console.log(`Generated ${path.join(output, 'index.html')}; source Markdown unchanged.`);
}
build();
if (process.argv.includes('--watch')) {
  let timer;
  for (const name of ['README.md', 'template.html']) fs.watch(path.join(base, name), () => {
    clearTimeout(timer);
    timer = setTimeout(() => { try { build(); } catch (error) { console.error(error.message); } }, 120);
  });
  console.log('Watching Markdown and template; refresh after saving.');
}
