const lessons = [
  {
    title: '白色十字', hold: '黄中心朝上，白中心朝下',
    idea: '先把四个白色棱放到黄中心周围，做一朵“小花”。再逐个对齐侧面中心，转 180° 送到底层。这样白十字的侧面也会自动对齐。',
    steps: ['只找有白色的两色棱块，暂时忽略所有角块。目标是让四个白色贴纸朝上，围住黄中心；花瓣的侧面此时不用对齐。', '白棱若在中层：把它所在的侧面转 90°，白色就能到顶层。若顶层目标位置已有花瓣，先转 U 空出位置再送入。', '白棱若在底层且白色朝下：先空出它正上方的位置，再把所在侧面转 180°。若白色在侧面（无论顶层或底层），先将该棱所在侧面转 90° 把它带入中层，再按上一条送上去。每次看白贴纸走到哪里，不要盲转。', '做好小花后，转 U，让一个白棱的另一种颜色对齐它下方的同色中心。将这一面对着自己，做 F2；这片白花瓣就到底部了。', '重复“转 U 对齐侧色 → 对着自己做 F2”，直到四片花瓣都送到底部。看底面检查白十字，再看四个侧面中心上、下的棱是否对色。'],
    check: '底面出现白十字；四个白棱的另一种颜色，都和相邻侧面中心一致。',
    warning: '小花阶段可以随时转 U 保护已有花瓣。送到底层以后，不要再随意转侧面，否则会破坏已经完成的白十字。',
    examples: [['送下一片花瓣', 'F2', false, '预设案例：顶层前方白棱的绿色已经对准前面绿中心。做 F2，看它进入底层。']],
    short: '小花 → U 对齐侧色 → 对应侧面转 180°。'
  },
  {
    title: '白色角块', hold: '保持白底黄顶；目标槽位放在右前下',
    idea: '一个角的三种颜色，决定它的唯一位置。先送到目标槽位的正上方，再用刚刚练过的四手循环把它装进去。',
    steps: ['找到顶层带白色的角块。看另外两种颜色，找出它在底层的目标角落。', '保持白底黄顶，转整个魔方，让目标槽位在右前下。只转 U，把要装入的白角送到右前上，也就是目标槽位正上方。', '完整做 R U R′ U′。每做完四手，检查目标角；需要 1、3 或 5 组，直到它进入右前下、白色朝下且两种侧色对齐中心。', '如果白角已在底层但放错或朝向不对，把它放在右前下，先做一组 R U R′ U′ 带到顶层，再找正确目标重新摆位。', '换下一个白角，先找它的家，再做同样动作。白色面完成后，也要检查四个侧面底下一排。'],
    check: '底面全白；四个侧面的最下排，分别和该面的中心同色。',
    warning: '白底黄顶一直保持。先摆对工作位置，再重复四手；不能把白色朝上当作完成，也不能省掉每组最后的 U′。',
    examples: [['白色在侧面：一组插入', "R U R' U'", false, '白绿红角已经在目标槽位的正上方。这次做一组四手就能装入。'], ['白色朝上：三组插入', Array(3).fill("R U R' U'").join(' '), false, '仍是同一目标槽位，这个朝向需三组四手。每组之后检查白角的位置和朝向。'], ['另一种侧面朝向：五组插入', Array(5).fill("R U R' U'").join(' '), false, '目标角朝向不同，需五组。这里重复的是同一个完整动作，不用另背新公式。']],
    short: '白底；槽位右前下、白角右前上；重复 R U R′ U′。'
  },
  {
    title: '第二层', hold: '保持白底黄顶',
    idea: '第二层不含白色或黄色棱。借顶层作工作区，把目标棱插入左边或右边，同时在公式结束时恢复白色层。',
    steps: ['在顶层找不含黄色的棱块。转 U，让它朝侧面的贴纸对齐同色中心，再把这一面朝向自己。此时目标棱位于前上方。', '看该棱朝上的贴纸：若和右中心同色，做右插公式 U R U′ R′ U′ F′ U F。若和左中心同色，做左插公式 U′ L′ U L U F U′ F′。', '每次做完，检查底层仍完整，目标棱已经装进对应中层。重复，直到四个中层棱归位。', '顶层全是黄色棱，但中层仍错？把错误槽位放在前右（或前左），用对应插入公式把任意顶层棱送进去，挤出错误棱，再对准重新插入。原地翻转的中层棱也这样处理。'],
    check: '白色底面完整，四个侧面下面两排都和各自中心同色。',
    warning: '先对齐前面颜色，再判断向左还是向右。两个公式不能互换；做公式期间不要换整个魔方的拿法。',
    examples: [['向右插入', "U R U' R' U' F' U F", false, '前上棱的绿色朝前、红色朝上，目标在绿中心与右侧红中心之间。'], ['向左插入', "U' L' U L U F U' F'", false, '前上棱的绿色朝前、橙色朝上，目标在绿中心与左侧橙中心之间。']],
    short: '右插 U R U′ R′ U′ F′ U F；左插 U′ L′ U L U F U′ F′。'
  },
  {
    title: '黄色十字', hold: '白底黄顶，只看顶面的四个棱',
    idea: '先让黄色棱朝上，不管侧面是否对色，也不管黄色角块。这一步处理朝向，下一步再处理位置。',
    steps: ['只看顶面上、下、左、右四个棱的黄色，不数角：可能是一个点、一个小折角、一条直线，或已是十字。', '若只有黄中心（点），任选前面做 F R U R′ U′ F′。做完重新判断图案。', '若是折角，转 U，使两条黄臂朝向后方和左方（俯视是左上角的折角），做一次公式。', '若是直线，转 U，让黄线从左到右横放，再做一次公式。已经是十字就跳过。每次做完重新判断并摆正，再继续。'],
    check: '黄中心和四个黄棱形成十字。侧面暂时可以不对色，角块可以不朝上。',
    warning: '识别图案时忽略四个角。点到十字最多三次公式，但每次之间都要重新检查拿法。',
    examples: [['横线变十字', "F R U R' U' F'", false, '预设案例：顶层左右两个黄棱朝上，黄线横放；做完整公式后得到十字。']],
    short: '折角朝后、左；直线横放；F R U R′ U′ F′。'
  },
  {
    title: '黄色棱归位', hold: '白底黄顶；相邻正确棱放在后面和右面',
    idea: '黄色十字已有正确朝向，现在调整四个棱的侧面颜色。转 U 是整体对齐，短公式用来交换棱的位置。',
    steps: ['尝试 U 的四种位置，选侧面黄棱与中心对色最多的一种。若四个都对，直接下一步。正常会找到至少两个对色的棱。', '若两个正确棱相邻，转整个魔方（保持白底），把它们放在后面和右面。此时前、左两个棱需要交换。', '完整做 R U R′ U R U2 R′ U，然后重新检查四面的棱与中心；必要时转 U 对齐。', '若两个正确棱相对，把它们放前后，先做一次公式；再转 U 找到相邻正确的一对，按“后、右正确”的拿法再做。'],
    check: '黄十字还在，四个黄棱的侧色全部与对应中心一致。下两层保持完整。',
    warning: '公式末尾的 U 不能漏。每次先检查对色棱，再确定拿法，不要对同一拿法无判断地一直重复。',
    examples: [['交换前、左的黄棱', "R U R' U R U2 R' U", false, '预设案例：后、右两个黄棱对色。观察公式结束后四个黄棱全部归位。']],
    short: '正确棱放后、右；R U R′ U R U2 R′ U。相对时先做一次。'
  },
  {
    title: '黄色角归位', hold: '白底黄顶；正确位置的角放右前上',
    idea: '角块位置正确，是指它的三种颜色与周围三个中心吻合；黄色不必朝上。这一步只把角放到正确角落。',
    steps: ['逐个查看顶层角的三色。比如黄绿红角，必须在黄、绿、红三个中心交汇处；无论三张贴纸朝哪里。', '找到位置正确的角，转整个魔方，保持白底，把它放在右前上。做 U R U′ L′ U R′ U′ L；这个公式保留右前上角，循环另外三个角。', '保持拿法检查；若未全部到正确位置，再做一次同样公式。', '如果没有任何角位置正确，任选前面先做一次公式，再找正确角，放到右前上处理。如果已经四角归位就跳过。'],
    check: '四个顶层角的三色组合都与各自周围的中心一致。角的黄色可以仍朝侧面。',
    warning: '不要用“黄色朝上”判断角是否归位。此步不要额外转 U 对齐角，那会把已经对好的黄色棱移走。',
    examples: [['保留一个角，循环另外三个', "U R U' L' U R' U' L", false, '预设案例：右前上角位置正确，另外三个需要循环交换。']],
    short: '正确角右前上；U R U′ L′ U R′ U′ L。无正确角先做一次。'
  },
  {
    title: '黄色角翻正', hold: '白底黄顶；从此刻起，不转整个魔方',
    idea: '在固定的右前上角逐个翻正角块。工作区会暂时扰乱下两层，等所有角处理完，这些扰动会抵消。',
    steps: ['把一个黄色未朝上的角放到右前上。这是最后一次选择整个魔方的拿法：此后前面和右面的中心颜色保持不变。', '重复完整的 R′ D′ R D；做 2 或 4 组后，这个角的黄色会朝上。每组必须包含末尾 D，不能在第三手就停。', '只转 U，把另一个黄色未朝上的角送到右前上。不要转整个魔方，也不要为了恢复底层额外做其他转动。', '继续对新角重复完整四手，直到黄色朝上。所有角依次处理完，下两层也会恢复。', '最后只转 U，让顶层的侧色对齐中心。检查六面都成为单一颜色。'],
    check: '六面各自为一种颜色，所有侧面与中心一致。你完成了第一次复原。',
    warning: '最容易出错的地方：看到下两层乱了就停、漏掉每组最后的 D、换角时转整个魔方。请把四手作为一个不可拆开的单位。',
    examples: [['完整收尾：两个角依次翻正', "R' D' R D R' D' R D U R' D' R D R' D' R D R' D' R D R' D' R D U'", false, '预设两个扭转角：第一个做两组；U 换角；第二个做四组；最后 U′ 对齐。你手里的情况可能不同，要看角是否朝上。']],
    short: '固定拿法；右前上角重复 R′ D′ R D；只用 U 换角，最后 U 对齐。'
  }
];
const $ = id => document.getElementById(id);
const explanations = [
  '白绿棱的目标在白中心与绿中心之间。小花阶段把白贴纸统一朝上，U 对齐绿色后，F2 正好把它搬到前下位置：白贴纸到底面，绿贴纸仍在前面。其他已送到底部的白棱不在这个前下槽位，所以不会被这次 F2 带走。',
  '刚才的实验已经看到：R U R′ U′ 让右前角在上下位置之间轮换，并改变朝向。它保留白十字和其他三个底层角；中层暂时还没做，不需要保护。先按另外两色选好右前下槽位，再把白角送到正上方，等循环把它正确装入即可。',
  '先对齐前面颜色，是为了确定目标槽位；上面的颜色决定向左还是向右。插入公式借用顶层，把目标棱带入中层，再恢复暂时打开的第一层。完整公式只影响所选的中层槽位，其他三个中层槽位与白色层不变，所以可以逐个插入。',
  '这套动作把黄棱的朝向从点、折角或横线推进到十字，并在结束时固定下两层。摆正图案，是把需要翻转的棱送到公式真正作用的位置。这里只要求四个黄棱朝上，允许它们的位置与黄角变化，因此不必一次兼顾所有顶层条件。',
  '这套完整公式交换前、左两个顶层棱，保留后、右两个棱，且不改变下两层或黄色棱朝上的条件。因此先把正确的一对放后、右，交换错误的一对就能完成。相对正确的一对先做一次，是为了转换成可用同一交换处理的相邻情况。角块会变化，留给后面处理。',
  '公式固定右前上角的位置，循环另外三个角，同时保留黄棱和下两层。若已有一个角正确，剩下的三个角在合法状态中只能是已正确或两个方向的三角循环；一个方向做一次，另一个做两次即可归位。若没有正确角，先循环一次创造定位依据。这个阶段不要求角的朝向正确。',
  '两组四手记作 T：它原地扭右前上角，保留顶层其他块，同时在下层留下固定的影响。T 做三次整块复原。只用 U 换角不会动下层；当所有角翻正时，角扭转量守恒保证总 T 次数是 3 的倍数，下层影响随之抵消。最后 U 对齐顶层。因此必须固定拿法，并做完每一组四手。'
];
const tasks = [
  ['四个白棱到家，白色朝下','已经送到底部的白棱'],
  ['把当前白角装入右前下槽位','白十字，以及其他三个底层角'],
  ['把当前不含黄色的棱插入中层','整个白色层，以及另外三个中层槽位'],
  ['顶层四个棱的黄色朝上','已经完成的下两层'],
  ['黄棱的侧色对齐四个中心','下两层和黄十字的朝向'],
  ['黄角的三色组合对应正确角落','下两层和已归位的黄棱'],
  ['全部黄角翻正，再对齐顶层','整个收尾完成后，恢复下两层与顶层位置']
];
const { Cube, inverse } = CubeLab;
let lessonIndex = 0, exampleIndex = 0, cursor = 0, cube, moves = [], timer = null, free = false;
const faceNames = { U:'上', D:'下', F:'前', B:'后', L:'左', R:'右' };
const colors = { W:'白', Y:'黄', G:'绿', B:'蓝', R:'红', O:'橙' };
function stop() { clearInterval(timer); timer = null; $('play').textContent = '自动播放'; }
function startCube() {
  stop(); cursor = 0; free = false;
  const example = lessons[lessonIndex].examples[exampleIndex];
  moves = example[1].split(/\s+/); cube = new Cube();
  if (example[2]) for (const s of cube.stickers) {
    s.p = [s.p[0], -s.p[1], -s.p[2]]; s.n = [s.n[0], -s.n[1], -s.n[2]];
  }
  cube.apply(inverse(example[1]));
  $('demo-intro').textContent = example[3]; renderCube();
}
function renderCube(last = '') {
  if (typeof CubeVisual !== 'undefined') $('lesson-solid').innerHTML = CubeVisual.svg(cube, { marked:lessonIndex===1 ? CubeVisual.indicesAt([1,-1,1]) : [], title:lessons[lessonIndex].title+'逐手立体演示' });
  $('net').innerHTML = ['U','L','F','R','B','D'].map(f => `<div class="face ${last[0] === f ? 'active' : ''}" data-face="${f}" aria-label="${faceNames[f]}面">${cube.face(f).map((c,i) => `<span class="sticker ${c}" title="${faceNames[f]}面：${colors[c]}">${i === 4 ? `<span class="face-name">${f}</span>` : c}</span>`).join('')}</div>`).join('');
  $('formula').innerHTML = moves.map((m,i) => `<span class="${i < cursor ? 'done' : i === cursor ? 'current' : ''}">${m.replace("'",'′')}</span>`).join('');
  $('previous').disabled = free || cursor === 0; $('next').disabled = free || cursor === moves.length; $('play').disabled = free || cursor === moves.length;
  const instruction = last ? `${faceNames[last[0]]}面${last.endsWith('2') ? '转 180°' : last.length > 1 ? '逆时针转 90°' : '顺时针转 90°'}（正对该面看）` : '点击“下一动作”，跟着做第一手。';
  $('move-status').textContent = free ? `自由练习：${instruction}` : cursor === moves.length ? `演示完成，共 ${moves.length} 手。检查色块与中心；按“重来”再练一次。` : `${cursor} / ${moves.length} 手 · ${instruction}`;
}
function nextMove() { if (free || cursor >= moves.length) return; const m = moves[cursor++]; cube.move(m); renderCube(m); if (cursor === moves.length) stop(); }
function selectLesson(index, updateHash = true) {
  lessonIndex = Math.max(0, Math.min(6,index)); exampleIndex = 0;
  const l = lessons[lessonIndex];
  $('steps').innerHTML = lessons.map((s,i) => `<button data-step="${i}" ${i === lessonIndex ? 'aria-current="step"' : ''}><b>0${i+1}</b>${s.title}</button>`).join('');
  $('instruction').innerHTML = `<span class="tag">拿法：${l.hold}</span><h2>0${lessonIndex+1} / ${l.title}</h2><div class="task-contract"><p><b>这一步改变</b>${tasks[lessonIndex][0]}</p><p><b>做完必须保留</b>${tasks[lessonIndex][1]}</p></div><p>${l.idea}</p><h3>为什么这一步有效</h3><p class="lesson-reason">${explanations[lessonIndex]}</p><h3>照着做</h3><ol>${l.steps.map(s => `<li>${s}</li>`).join('')}</ol><h3>完成检查</h3><p class="check">${l.check}</p><p class="warning">${l.warning}</p>`;
  $('progress').textContent = `当前步骤 ${lessonIndex+1} / 7`;
  $('example').innerHTML = l.examples.map((e,i) => `<option value="${i}">${e[0]}</option>`).join('');
  $('back-lesson').disabled = lessonIndex === 0; $('forward-lesson').disabled = lessonIndex === 6;
  if (updateHash) history.replaceState(null,'',`#step-${lessonIndex+1}`);
  startCube();
}
$('steps').addEventListener('click',e => { const b = e.target.closest('button'); if (b) selectLesson(Number(b.dataset.step)); });
$('example').addEventListener('change',e => { exampleIndex = Number(e.target.value); startCube(); });
$('next').addEventListener('click',nextMove);
$('previous').addEventListener('click',() => { stop(); if (!free && cursor) { cube.apply(inverse(moves[--cursor])); renderCube(); } });
$('play').addEventListener('click',() => { if (timer) stop(); else { $('play').textContent = '暂停'; timer = setInterval(nextMove,1000); } });
$('reset').addEventListener('click',startCube);
$('back-lesson').addEventListener('click',() => selectLesson(lessonIndex-1));
$('forward-lesson').addEventListener('click',() => selectLesson(lessonIndex+1));
$('free-moves').innerHTML = ['U',"U'",'R',"R'",'F',"F'",'D',"D'",'L',"L'",'B',"B'"].map(m => `<button data-move="${m}">${m.replace("'",'′')}</button>`).join('');
$('free-moves').addEventListener('click',e => { const b = e.target.closest('button'); if (b) { stop(); free = true; cube.move(b.dataset.move); renderCube(b.dataset.move); } });
$('solved').addEventListener('click',() => { stop(); free = true; cube = new Cube(); renderCube(); });
$('cheat-list').innerHTML = lessons.map((l,i) => `<div class="cheat-row"><strong>0${i+1} ${l.title}</strong><div>${l.short}<br><span class="small">拿法：${l.hold}。${l.check}</span></div></div>`).join('');
for (const f of document.querySelectorAll('.iso-face')) f.setAttribute('aria-hidden','true');
window.addEventListener('hashchange',() => { const m = location.hash.match(/^#step-([1-7])$/); if (m) selectLesson(Number(m[1])-1,false); });
window.addEventListener('pagehide',stop);
selectLesson(Number(location.hash.match(/^#step-([1-7])$/)?.[1] || 1)-1,false);
