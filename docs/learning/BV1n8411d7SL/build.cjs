const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const base = __dirname;
function build() {
  const markdown = fs.readFileSync(path.join(base, 'README.md'), 'utf8');
  const template = fs.readFileSync(path.join(base, 'template.html'), 'utf8');
  if (!template.includes('/* MARKDOWN_SOURCE */')) throw Error('Missing source slot');
  const source = JSON.stringify(markdown).replace(/</g, '\\u003c').replace(/\u2028/g, '\\u2028').replace(/\u2029/g, '\\u2029');
  const html = template.replace('/* MARKDOWN_SOURCE */', `const markdown = ${source};`);
  new vm.Script(html.match(/<script>([\s\S]*?)<\/script>/)[1]);
  fs.writeFileSync(path.join(base, 'index.html'), html);
  console.log('Generated index.html from README.md; Markdown unchanged.');
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
