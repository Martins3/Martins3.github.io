const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

for (const entry of fs.readdirSync(__dirname, { withFileTypes: true })) {
  if (!entry.isDirectory()) continue;
  const script = path.join(__dirname, entry.name, 'build.cjs');
  if (fs.existsSync(script)) execFileSync(process.execPath, [script, ...process.argv.slice(2)], { stdio: 'inherit' });
}
