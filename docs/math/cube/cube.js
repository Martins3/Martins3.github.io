/* Integer coordinates keep every sticker attached to its physical cubie. */
(function (root) {
  const faces = {
    U: { n: [0, 1, 0], right: [1, 0, 0], down: [0, 0, 1], color: 'Y' },
    D: { n: [0, -1, 0], right: [1, 0, 0], down: [0, 0, -1], color: 'W' },
    F: { n: [0, 0, 1], right: [1, 0, 0], down: [0, -1, 0], color: 'G' },
    B: { n: [0, 0, -1], right: [-1, 0, 0], down: [0, -1, 0], color: 'B' },
    R: { n: [1, 0, 0], right: [0, 0, -1], down: [0, -1, 0], color: 'R' },
    L: { n: [-1, 0, 0], right: [0, 0, 1], down: [0, -1, 0], color: 'O' }
  };
  const dot = (a, b) => a.reduce((sum, v, i) => sum + v * b[i], 0);
  function rotate(v, n) {
    const cross = [n[1]*v[2]-n[2]*v[1], n[2]*v[0]-n[0]*v[2], n[0]*v[1]-n[1]*v[0]];
    return v.map((_, i) => n[i]*dot(n,v)-cross[i]);
  }
  class Cube {
    constructor() { this.stickers = Object.values(faces).flatMap(f => Array.from({ length: 9 }, (_, i) => ({
      p: f.n.map((v, k) => v + (i%3-1)*f.right[k] + (Math.floor(i/3)-1)*f.down[k]), n: [...f.n], color: f.color
    }))); }
    move(token) {
      if (!/^[URFDLB](?:['′2])?$/.test(token)) throw new Error('Invalid move: ' + token);
      const n = faces[token[0]].n;
      const count = token.endsWith('2') ? 2 : token.length > 1 ? 3 : 1;
      for (let k = 0; k < count; k++) for (const s of this.stickers) if (dot(s.p,n) === 1) { s.p = rotate(s.p,n); s.n = rotate(s.n,n); }
      return this;
    }
    apply(sequence) { sequence.trim().split(/\s+/).filter(Boolean).forEach(t => this.move(t)); return this; }
    face(name) {
      const f = faces[name], cells = Array(9);
      for (const s of this.stickers) if (dot(s.n,f.n) === 1) cells[(dot(s.p,f.down)+1)*3+dot(s.p,f.right)+1] = s.color;
      return cells;
    }
    signature() { return Object.keys(faces).map(f => this.face(f).join('')).join(''); }
  }
  function inverse(sequence) { return sequence.trim().split(/\s+/).filter(Boolean).reverse().map(t => t.endsWith('2') ? t : t.length > 1 ? t[0] : t+"'").join(' '); }
  root.CubeLab = { Cube, faces, inverse };
})(typeof window === 'undefined' ? globalThis : window);
