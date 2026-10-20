(function (root) {
  const { Cube } = root.CubeLab;
  const trigger = "R U R' U'";
  const cornerCycle = "U R U' L' U R' U' L";
  const twist = "R' D' R D R' D' R D";
  const finish = `${twist} U ${twist} ${twist} U'`;
  const experiments = [
    { label:'1 / 一层搬一圈', goal:'观察目标：始终找白绿红角 A', title:'转的是一层，走的是整块', description:'从复原状态开始，只做 R。白绿红角的三色一起走。连续做四次 R，整圈回到起点。此时只找 A，不要求整面颜色整齐。', moves:['R','R','R','R'], marked:[1,-1,1], question:'在手里也试四次 R。你能只凭三色组合，找回同一块吗？', stops:[0,1,2,3,4] },
    { label:'2 / 四手是一个动作', goal:'观察目标：位置回来，还要检查朝向', title:'把 R U R′ U′ 当作一个小动作', description:'先逐手看一组，再看重复整组的结果。每组的最终效果相同，可以作为一个工具反复使用；不要在组内看到某色就停。第六组回到开始状态。', moves:Array(6).fill(trigger).join(' ').split(' '), marked:[1,-1,1], question:'在第 2、4 组结束时，A 回到原来的角落了吗？再看前、右两色是否对准中心。', stops:[0,4,8,12,16,20,24] },
    { label:'3 / 给动作换工作对象', goal:'观察目标：为什么下两层能保住', title:'F + 四手小动作 + F′', description:'四手动作本来会牵动前右中棱和前右下角。F 先把顶层块搬到这两个工作位置，四手动作改动它们；F′ 再撤销准备，原来的下层块回家。这里 A 是黄绿棱。', moves:['F',...trigger.split(' '),"F'"], marked:[0,1,1], question:'对照“开始”和“完成”：下两层是不是恢复了，而顶层仍留下变化？', stops:[0,1,2,3,4,5,6] }
  ];
  function stateCube(experimentIndex,cursor) { return new Cube().apply(experiments[experimentIndex].moves.slice(0,cursor).join(' ')); }
  function homeCounts(cube) {
    const reference = new Cube(), pieces = new Map();
    reference.stickers.forEach((s,i) => {
      if (s.p.filter(v => v !== 0).length < 2) return;
      const key = s.p.join(',');
      if (!pieces.has(key)) pieces.set(key,{ lower:s.p[1] < 1, home:true });
      const current=cube.stickers[i];
      if (s.p.some((v,k)=>v!==current.p[k]) || s.n.some((v,k)=>v!==current.n[k])) pieces.get(key).home=false;
    });
    const all=[...pieces.values()];
    return { all:all.filter(p=>p.home).length, lower:all.filter(p=>p.lower&&p.home).length };
  }
  function observation(index,cursor) {
    if (cursor===0) return '先认出 A，然后猜一猜：下一手它会走到哪里？';
    if (index===0) return cursor===4 ? '四次 R 后，位置与朝向都回到开始状态。中途它始终是同一个白绿红角。' : '看 A 的三个标记，它们一起移动；白色没有从这块转移到另一块。';
    if (index===1) {
      if (cursor===24) return '六组结束，整个魔方回到做这串动作以前的状态。若起点是打乱的，回来也仍是那个打乱状态。';
      if (cursor%4===0) return `第 ${cursor/4} 组完成。先看 A 所在的角落，再看白色朝向。只回到同一个角落，还不等于完全恢复。`;
      return `正在第 ${Math.floor(cursor/4)+1} 组里。这一手只是中间状态；要做完四手再检查。`;
    }
    if (cursor===1) return 'F 是准备：黄绿棱 A 从前上来到前右中。白绿红角也暂时被搬走，顶层角占据前右下工作位置。';
    if (cursor===5) return '四手小动作已完成，但 F 的准备还没撤销。此时不能用下层是否整齐判断最终效果。';
    if (cursor===6) return 'F′ 撤销了准备。下两层全部恢复；顶层块的位置和部分朝向发生变化。这一组合正是黄色十字公式。';
    return '小动作正在改动换进工作位置的块。先别整理下层，继续按顺序做完。';
  }
  root.ReasonLab={ experiments,stateCube,homeCounts,observation,trigger,cornerCycle,twist,finish };
  if (typeof document==='undefined') return;
  const get=id=>document.getElementById(id), { svg,indicesAt,positionName }=root.CubeVisual;
  let selected=0,cursor=0,yaw=0;
  get('identity-cube').innerHTML=svg(new Cube(),{ marked:indicesAt([1,-1,1]), title:'持续标记白绿红角 A' });
  function render() {
    const e=experiments[selected],cube=stateCube(selected,cursor),counts=homeCounts(cube),marked=indicesAt(e.marked);
    get('reason-tabs').innerHTML=experiments.map((e,i)=>`<button data-experiment="${i}" aria-pressed="${selected===i}">${e.label}</button>`).join('');
    get('reason-goal').textContent=e.goal; get('reason-title').textContent=e.title; get('reason-description').textContent=e.description;
    const group=selected===1?Math.floor(Math.max(0,cursor-1)/4):0;
    const shown=selected===1?e.moves.slice(group*4,group*4+4):e.moves;
    get('reason-formula').innerHTML=(selected===1?`<small>第 ${group+1} / 6 组</small>`:'')+shown.map((m,i)=>{
      const absolute=selected===1?group*4+i:i;
      return `<span class="${absolute<cursor?'done':absolute===cursor?'current':''}">${m.replace("'",'′')}</span>`;
    }).join('');
    get('reason-prev').disabled=cursor===0; get('reason-next').disabled=cursor===e.moves.length;
    get('reason-timeline').innerHTML=e.stops.map(n=>`<button data-state="${n}" aria-pressed="${cursor===n}">${n===0?'开始':selected===1?`第 ${n/4} 组结束`:n===e.moves.length?'完成':`第 ${n} 手后`}</button>`).join('');
    const position=positionName(cube.stickers[marked[0]].p);
    get('reason-observation').textContent=`${cursor} / ${e.moves.length} 手 · A 在${position}。${observation(selected,cursor)}`;
    get('reason-question').textContent=e.question;
    get('reason-solid').innerHTML=svg(cube,{ marked,yaw,dim:true,title:`第 ${cursor} 手后，A 在${position}` });
    get('reason-net').innerHTML=['U','L','F','R','B','D'].map(f=>`<div class="face" data-face="${f}" aria-label="${f} 面">${cube.face(f).map((c,i)=>`<span class="sticker ${c}">${i===4?f:c}</span>`).join('')}</div>`).join('');
    get('reason-view').querySelectorAll('button').forEach(b=>b.setAttribute('aria-pressed',String(Number(b.dataset.yaw)===yaw)));
    if (selected===2) get('reason-question').textContent=`${e.question} 当前下两层 ${counts.lower} / 12 块完全恢复。`;
  }
  get('reason-tabs').addEventListener('click',e=>{const b=e.target.closest('button');if(b){selected=Number(b.dataset.experiment);cursor=0;yaw=0;render();}});
  get('reason-timeline').addEventListener('click',e=>{const b=e.target.closest('button');if(b){cursor=Number(b.dataset.state);render();}});
  get('reason-next').addEventListener('click',()=>{if(cursor<experiments[selected].moves.length){cursor++;render();}});
  get('reason-prev').addEventListener('click',()=>{if(cursor>0){cursor--;render();}});
  get('reason-reset').addEventListener('click',()=>{cursor=0;render();});
  get('reason-view').addEventListener('click',e=>{const b=e.target.closest('button');if(b){yaw=Number(b.dataset.yaw);render();}});
  render();
})(typeof window==='undefined'?globalThis:window);
