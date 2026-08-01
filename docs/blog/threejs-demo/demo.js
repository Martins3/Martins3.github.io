import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import * as CANNON from 'cannon-es';

// X 表示一块积木。英文字母使用 5x7 点阵；中文使用更大的点阵保留笔画特征。
const FONT = {
  M: ['XX.XX', 'XXXXX', 'X.X.X', 'X...X', 'X...X', 'X...X', 'X...X'],
  A: ['.XXX.', 'XX.XX', 'X...X', 'XXXXX', 'X...X', 'X...X', 'X...X'],
  R: ['XXXX.', 'XX.XX', 'X...X', 'XXXX.', 'XXX..', 'XX.X.', 'X..XX'],
  T: ['XXXXX', '.XXX.', '.XXX.', '.XXX.', '.XXX.', '.XXX.', '.XXX.'],
  I: ['XXXXX', '.XXX.', '.XXX.', '.XXX.', '.XXX.', '.XXX.', 'XXXXX'],
  N: ['X...X', 'XX..X', 'XXX.X', 'X.XXX', 'X..XX', 'X...X', 'X...X'],
  S: ['.XXXX', 'XX...', 'XX...', '.XXX.', '...XX', '...XX', 'XXXX.'],
  3: ['XXXX.', '...XX', '...XX', '.XXXX', '...XX', '...XX', 'XXXX.'],
  刀: [
    '.XXXXXXXX',
    '.XXXXXXXX',
    '....XX.XX',
    '...XX..XX',
    '...XX..XX',
    '..XX...XX',
    '..XX...XX',
    '.XX....XX',
    '.XX...XXX',
    'XX...XXX.'
  ]
};

const TEXT = document.body.dataset.text || 'MARTINS3';
const glyphs = Array.from(TEXT, ch => {
  const rows = FONT[ch];
  if (!rows) throw new Error(`没有为字符“${ch}”定义点阵字模`);
  return { ch, rows, width: rows[0].length, height: rows.length };
});

const BLOCK = 0.7;
const LETTER_GAP = 1.5;
const DEPTH = 2;
const blocks = [];
const balls = [];
let wallWidth = 0;
let wallHeight = 0;

// ---------- three.js 场景 ----------
const scene = new THREE.Scene();
scene.background = new THREE.Color(0xffffff);

const camera = new THREE.PerspectiveCamera(
  60, window.innerWidth / window.innerHeight, 0.1, 200);
camera.position.set(0, 4, 24);

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.shadowMap.enabled = true;
document.body.appendChild(renderer.domElement);

// 左键用于发射，所以旋转视角绑到右键。
const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.mouseButtons = {
  LEFT: null,
  MIDDLE: THREE.MOUSE.DOLLY,
  RIGHT: THREE.MOUSE.ROTATE
};
controls.touches = {
  ONE: THREE.TOUCH.ROTATE,
  TWO: THREE.TOUCH.DOLLY_ROTATE
};
controls.target.set(0, 2, 0);

scene.add(new THREE.AmbientLight(0xffffff, 0.5));
const light = new THREE.DirectionalLight(0xffffff, 1.2);
light.position.set(10, 20, 15);
light.castShadow = true;
light.shadow.camera.left = -25;
light.shadow.camera.right = 25;
light.shadow.camera.top = 25;
light.shadow.camera.bottom = -25;
scene.add(light);

// ---------- cannon-es 物理世界 ----------
const world = new CANNON.World({ gravity: new CANNON.Vec3(0, -9.82, 0) });
world.broadphase = new CANNON.SAPBroadphase(world);
world.allowSleep = true;

const groundPhysMat = new CANNON.Material('ground');
const blockPhysMat = new CANNON.Material('block');
world.addContactMaterial(new CANNON.ContactMaterial(groundPhysMat, blockPhysMat, {
  friction: 0.5,
  restitution: 0.1
}));
world.addContactMaterial(new CANNON.ContactMaterial(blockPhysMat, blockPhysMat, {
  friction: 0.4,
  restitution: 0.05
}));

const groundBody = new CANNON.Body({
  type: CANNON.Body.STATIC,
  shape: new CANNON.Plane(),
  material: groundPhysMat
});
groundBody.quaternion.setFromEuler(-Math.PI / 2, 0, 0);
world.addBody(groundBody);

const groundMesh = new THREE.Mesh(
  new THREE.PlaneGeometry(200, 200),
  new THREE.MeshStandardMaterial({ color: 0xf2f2f2 }));
groundMesh.rotation.x = -Math.PI / 2;
groundMesh.receiveShadow = true;
scene.add(groundMesh);

// ---------- 用积木搭字 ----------
const blockGeo = new THREE.BoxGeometry(BLOCK, BLOCK, BLOCK);
const blockHalf = new CANNON.Vec3(BLOCK / 2, BLOCK / 2, BLOCK / 2);
const blockMat = new THREE.MeshStandardMaterial({ color: 0x34a853, roughness: 0.5 });

function buildWall() {
  const totalCols = glyphs.reduce((sum, glyph) => sum + glyph.width, 0) +
    (glyphs.length - 1) * LETTER_GAP;
  wallWidth = totalCols * BLOCK;
  wallHeight = Math.max(...glyphs.map(glyph => glyph.height)) * BLOCK;
  let cursor = -totalCols / 2;
  const z0 = -(DEPTH - 1) * BLOCK / 2;

  for (const glyph of glyphs) {
    for (let r = 0; r < glyph.height; r++) {
      const row = glyph.rows[r];
      const y = (glyph.height - 1 - r + 0.5) * BLOCK;

      for (let c = 0; c < row.length; c++) {
        if (row[c] !== 'X') continue;
        const x = (cursor + c + 0.5) * BLOCK;

        for (let d = 0; d < DEPTH; d++) {
          const z = z0 + d * BLOCK;
          const mesh = new THREE.Mesh(blockGeo, blockMat);
          mesh.castShadow = true;
          mesh.position.set(x, y, z);
          scene.add(mesh);

          const body = new CANNON.Body({
            mass: 0.5,
            shape: new CANNON.Box(blockHalf),
            material: blockPhysMat,
            position: new CANNON.Vec3(x, y, z),
            sleepSpeedLimit: 0.3,
            sleepTimeLimit: 0.5
          });
          world.addBody(body);
          // 初始睡眠：悬空积木不会自行塌落，被弹丸撞击后才会被唤醒。
          body.sleep();
          blocks.push({ mesh, body, ix: x, iy: y });
        }
      }
    }

    cursor += glyph.width + LETTER_GAP;
  }

  controls.target.y = wallHeight / 2;
  fitCameraToWall();
}

function fitCameraToWall() {
  const verticalFov = THREE.MathUtils.degToRad(camera.fov);
  const horizontalFov = 2 * Math.atan(
    Math.tan(verticalFov / 2) * camera.aspect);
  const padding = 1.15;
  const widthDistance = wallWidth * padding /
    (2 * Math.tan(horizontalFov / 2));
  const heightDistance = wallHeight * padding /
    (2 * Math.tan(verticalFov / 2));
  const distance = Math.max(18, widthDistance, heightDistance);
  const viewDirection = camera.position.clone().sub(controls.target);

  if (viewDirection.lengthSq() === 0) viewDirection.set(0, 0, 1);
  camera.position.copy(controls.target).addScaledVector(
    viewDirection.normalize(), distance);
  camera.updateProjectionMatrix();
  controls.update();
}

buildWall();

// ---------- 弹丸 ----------
const ballGeo = new THREE.SphereGeometry(0.7, 24, 16);
const ballMat = new THREE.MeshStandardMaterial({ color: 0xff3333, roughness: 0.3 });
const raycaster = new THREE.Raycaster();

function launchBall(dir) {
  const start = camera.position.clone().addScaledVector(dir, 2);
  const mesh = new THREE.Mesh(ballGeo, ballMat);
  mesh.castShadow = true;
  mesh.position.copy(start);
  scene.add(mesh);

  const body = new CANNON.Body({
    mass: 12,
    shape: new CANNON.Sphere(0.7),
    material: blockPhysMat,
    position: new CANNON.Vec3(start.x, start.y, start.z),
    velocity: new CANNON.Vec3(dir.x * 55, dir.y * 55, dir.z * 55)
  });
  world.addBody(body);
  balls.push({ mesh, body, born: performance.now() });

  // 最多保留 20 颗弹丸，超出时移除最老的。
  if (balls.length > 20) {
    const old = balls.shift();
    scene.remove(old.mesh);
    world.removeBody(old.body);
  }
}

function shoot(clientX, clientY) {
  const ndc = new THREE.Vector2(
    (clientX / window.innerWidth) * 2 - 1,
    -(clientY / window.innerHeight) * 2 + 1);
  raycaster.setFromCamera(ndc, camera);
  launchBall(raycaster.ray.direction.clone());
}

// 手机手势：单击发射；双击的第二次按下后拖动，相当于按住鼠标右键旋转。
// 单击需要等一个很短的双击判定窗口，避免第一击先误发弹丸。
const DOUBLE_TAP_DELAY = 300;
const TAP_MOVE_LIMIT = 14;
const DOUBLE_TAP_DISTANCE = 56;
let pendingTap = null;
let activeTouch = null;

function pointerDistance(a, b) {
  return Math.hypot(a.clientX - b.x, a.clientY - b.y);
}

function blockTouchFromControls(event) {
  event.preventDefault();
  event.stopImmediatePropagation();
}

renderer.domElement.addEventListener('pointerdown', event => {
  if (event.pointerType !== 'touch') return;

  const isSecondTap = pendingTap !== null &&
    performance.now() - pendingTap.time <= DOUBLE_TAP_DELAY &&
    pointerDistance(event, pendingTap) <= DOUBLE_TAP_DISTANCE;

  if (isSecondTap) {
    clearTimeout(pendingTap.timer);
    pendingTap = null;
  } else if (pendingTap !== null) {
    // 两次点击相距较远时视为两次发射，不吞掉第一发。
    clearTimeout(pendingTap.timer);
    const previousTap = pendingTap;
    pendingTap = null;
    shoot(previousTap.x, previousTap.y);
  }

  activeTouch = {
    pointerId: event.pointerId,
    startX: event.clientX,
    startY: event.clientY,
    moved: false,
    rotating: isSecondTap
  };

  if (!isSecondTap) blockTouchFromControls(event);
}, true);

renderer.domElement.addEventListener('pointermove', event => {
  if (event.pointerType !== 'touch' || event.pointerId !== activeTouch?.pointerId) return;

  if (Math.hypot(
    event.clientX - activeTouch.startX,
    event.clientY - activeTouch.startY) > TAP_MOVE_LIMIT) {
    activeTouch.moved = true;
  }

  if (!activeTouch.rotating) blockTouchFromControls(event);
}, true);

renderer.domElement.addEventListener('pointerup', event => {
  if (event.pointerType !== 'touch' || event.pointerId !== activeTouch?.pointerId) return;

  const touch = activeTouch;
  activeTouch = null;
  if (touch.rotating) return;

  blockTouchFromControls(event);
  if (touch.moved) return;

  const tap = {
    x: event.clientX,
    y: event.clientY,
    time: performance.now(),
    timer: null
  };
  tap.timer = setTimeout(() => {
    if (pendingTap !== tap) return;
    pendingTap = null;
    shoot(tap.x, tap.y);
  }, DOUBLE_TAP_DELAY);
  pendingTap = tap;
}, true);

renderer.domElement.addEventListener('pointercancel', event => {
  if (event.pointerType !== 'touch' || event.pointerId !== activeTouch?.pointerId) return;

  const wasRotating = activeTouch.rotating;
  activeTouch = null;
  if (!wasRotating) blockTouchFromControls(event);
}, true);

renderer.domElement.addEventListener('pointerdown', event => {
  if (event.pointerType !== 'touch' && event.button === 0) {
    shoot(event.clientX, event.clientY);
  }
});

// ?autoshoot 会自动朝文字墙发射一发，供无头浏览器测试。
const debug = new URLSearchParams(location.search).has('autoshoot');
if (debug) {
  setTimeout(() => {
    const target = controls.target.clone();
    launchBall(target.sub(camera.position).normalize());
  }, 800);
}

const infoEl = document.getElementById('info');
const resetButton = document.getElementById('reset');
let lastInfoUpdate = 0;

if (matchMedia('(pointer: coarse)').matches) {
  infoEl.textContent = '单击发射 | 双击后拖动旋转视角';
}

function resetDemo() {
  location.reload();
}

resetButton.addEventListener('click', resetDemo);

window.addEventListener('keydown', event => {
  if (event.key === 'r' || event.key === 'R') resetDemo();
});

window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  fitCameraToWall();
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
});

// ---------- 主循环 ----------
const clock = new THREE.Clock();

function animate() {
  requestAnimationFrame(animate);
  world.step(1 / 60, clock.getDelta(), 3);

  for (const block of blocks) {
    block.mesh.position.copy(block.body.position);
    block.mesh.quaternion.copy(block.body.quaternion);
  }

  const now = performance.now();
  for (let i = balls.length - 1; i >= 0; i--) {
    const ball = balls[i];
    ball.mesh.position.copy(ball.body.position);
    ball.mesh.quaternion.copy(ball.body.quaternion);

    // 飞远或存在超过 10 秒的弹丸会被回收。
    if (now - ball.born > 10000 || ball.body.position.length() > 150) {
      if (debug) {
        console.log('recycle: age=' + (now - ball.born).toFixed(0) +
          ' pos=' + ball.body.position.toArray().map(value => value.toFixed(1)));
      }
      scene.remove(ball.mesh);
      world.removeBody(ball.body);
      balls.splice(i, 1);
    }
  }

  controls.update();
  renderer.render(scene, camera);

  if (debug && now - lastInfoUpdate > 500) {
    lastInfoUpdate = now;
    const awake = blocks.filter(
      block => block.body.sleepState !== CANNON.Body.SLEEPING).length;
    const moved = blocks.filter(block =>
      Math.abs(block.body.position.x - block.ix) > 0.3 ||
      Math.abs(block.body.position.y - block.iy) > 0.3).length;
    const ballInfo = balls.length
      ? balls.map(ball =>
        ball.body.position.y.toFixed(1) + '/' + ball.body.position.z.toFixed(1)).join(',')
      : 'none';
    infoEl.textContent = 'awake=' + awake + ' moved=' + moved +
      ' balls=' + balls.length + ' [' + ballInfo + ']';
  }
}

animate();
