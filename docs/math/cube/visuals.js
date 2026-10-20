(function (root) {
  const { faces, Cube } = root.CubeLab;
  const palette = { W:'#f5f3ec', Y:'#efd66b', G:'#79b990', B:'#88b5df', R:'#e88c80', O:'#ecae70' };
  const equal = (a,b) => a.every((v,i) => v === b[i]);
  function indicesAt(position) { return new Cube().stickers.flatMap((s,i) => equal(s.p,position) ? [i] : []); }
  function positionName(position) {
    const names = [];
    if (position[2]) names.push(position[2] === 1 ? '前' : '后');
    if (position[0]) names.push(position[0] === 1 ? '右' : '左');
    if (position[1]) names.push(position[1] === 1 ? '上' : '下');
    return names.join('') + (position.filter(Boolean).length === 3 ? '角' : '棱');
  }
  function svg(cube, { marked=[], yaw=0, title='魔方立体图', dim=false } = {}) {
    const turn = v => { let p=[...v]; for (let i=0;i<yaw;i++) p=[p[2],p[1],-p[0]]; return p; };
    const project = v => { const p=turn(v); return [180+49*(p[0]-p[2]),153+49*(.46*(p[0]+p[2])-.88*p[1])]; };
    const visible = n => { const p=turn(n); return p[0]+p[1]+p[2] > 0; };
    const polygons = cube.stickers.map((s,i) => ({ s,i,depth:turn(s.p).reduce((a,b)=>a+b,0) })).filter(({s}) => visible(s.n)).sort((a,b)=>a.depth-b.depth).map(({s,i}) => {
      const f = Object.values(faces).find(f => equal(f.n,s.n));
      const center = s.p.map((v,k)=>v+.51*s.n[k]);
      const points = [[-1,-1],[1,-1],[1,1],[-1,1]].map(([a,b]) => project(center.map((v,k)=>v+.46*a*f.right[k]+.46*b*f.down[k])));
      const c=project(center), selected=marked.includes(i);
      return `<g data-sticker="${i}"><polygon points="${points.map(p=>p.map(v=>v.toFixed(1)).join(',')).join(' ')}" fill="${palette[s.color]}" stroke="#20332d" stroke-width="${selected?3:1.8}" opacity="${dim&&!selected?.45:1}"/>${selected ? `<circle cx="${c[0]}" cy="${c[1]}" r="10" fill="#20332d"/><text x="${c[0]}" y="${c[1]+4}" text-anchor="middle" fill="white" font-size="12" font-weight="700">A</text>` : s.p.filter(Boolean).length === 1 ? `<text x="${c[0]}" y="${c[1]+5}" text-anchor="middle" fill="#20332d" font-size="15" font-weight="700">${Object.keys(faces).find(name=>equal(faces[name].n,s.n))}</text>` : ''}</g>`;
    }).join('');
    return `<svg class="cube-svg" viewBox="0 0 360 320" role="img" aria-label="${title}"><ellipse cx="180" cy="283" rx="96" ry="12" fill="#20332d" opacity=".07"/>${polygons}</svg>`;
  }
  root.CubeVisual = { svg, indicesAt, positionName };
})(typeof window === 'undefined' ? globalThis : window);
