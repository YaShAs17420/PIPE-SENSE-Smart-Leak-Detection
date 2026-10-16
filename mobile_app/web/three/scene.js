import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';


// ============================================================
// PIPE-SENSE THREE.JS SCENE
// ============================================================
//
// Important:
// Flutter's HtmlElementView can replace the HTML container when
// the Flutter widget tree rebuilds (for example, when switching
// Dark / Light theme).
//
// This file intentionally keeps the THREE scene + renderer alive
// and re-attaches the SAME canvas to the new container.
// ============================================================


// ============================================================
// GLOBAL STATE
// ============================================================

let scene = null;
let camera = null;
let renderer = null;
let controls = null;

let container = null;
let resizeObserver = null;
let mutationObserver = null;

let initialized = false;
let initializing = false;
let animationStarted = false;

let currentTheme = 'light';


// ============================================================
// SENSOR STATE
// ============================================================

let currentSensorState = {
  yfFlowRate: 2.6,
  zjFlowRate: 2.5,
  vibration1: 1.0,
  vibration2: 1.1,
  leakDetected: false,
  leakZone: null,
};


// ============================================================
// TIMER
// ============================================================

const timer = new THREE.Timer();

timer.connect(document);


// ============================================================
// THEMES
// ============================================================

const THEMES = {

  light: {

    background: 0xf1f7f4,
    floor: 0xe5eeea,
    platform: 0xf7fbf9,
    backPanel: 0xe1e9e5,

    pipe: 0xf4f6f5,
    pipeDark: 0x242c29,

    water: 0x1aa9df,
    waterLight: 0x71defc,

    green: 0x43bd91,
    blue: 0x27afd7,

    normal: 0x43c795,
    warning: 0xff4f65,

    support: 0xaab8b2,
    grid: 0xbccac4,

    tank: 0x6bcce7,
    tankWater: 0x24b7e7,
  },


  dark: {

    background: 0x07110d,
    floor: 0x101a16,
    platform: 0x17231e,
    backPanel: 0x101914,

    pipe: 0xd5dcda,
    pipeDark: 0x171e1b,

    water: 0x21b8e8,
    waterLight: 0x73e5ff,

    green: 0x58d8a6,
    blue: 0x2bc8ec,

    normal: 0x58d8a6,
    warning: 0xff596c,

    support: 0x68766f,
    grid: 0x34413b,

    tank: 0x319fc1,
    tankWater: 0x1db6e9,
  },

};


function colors() {
  return THEMES[currentTheme];
}


// ============================================================
// LEAK ZONES
// ============================================================

const leakZones = {

  'Zone 1': {

    x: -1.9,

    outlet: null,
    ring: null,
    pool: null,
    splash: null,

    particles: [],
  },


  'Zone 2': {

    x: 1.25,

    outlet: null,
    ring: null,
    pool: null,
    splash: null,

    particles: [],
  },


  'Zone 3': {

    x: 4.4,

    outlet: null,
    ring: null,
    pool: null,
    splash: null,

    particles: [],
  },

};


// ============================================================
// FLOW PARTICLES
// ============================================================

const mainWaterParticles = [];
const returnWaterParticles = [];


// ============================================================
// MATERIAL HELPER
// ============================================================

function material(
  color,
  roughness = 0.45,
  metalness = 0.1,
  transparent = false,
  opacity = 1
) {

  return new THREE.MeshStandardMaterial({

    color,

    roughness,

    metalness,

    transparent,

    opacity,

  });

}


// ============================================================
// PIPE CREATION
// ============================================================

function createPipeBetween(
  start,
  end,
  radius,
  color
) {

  const direction =
    new THREE.Vector3()
      .subVectors(end, start);

  const length =
    direction.length();


  const geometry =
    new THREE.CylinderGeometry(
      radius,
      radius,
      length,
      32
    );


  const mesh =
    new THREE.Mesh(
      geometry,
      material(
        color,
        0.36,
        0.28
      )
    );


  mesh.position
    .copy(start)
    .add(end)
    .multiplyScalar(0.5);


  mesh.quaternion.setFromUnitVectors(

    new THREE.Vector3(0, 1, 0),

    direction.normalize()

  );


  mesh.castShadow = true;

  mesh.receiveShadow = true;


  scene.add(mesh);

  return mesh;
}


// ============================================================
// PIPE RING
// ============================================================

function createPipeRing(
  x,
  y,
  z
) {

  const ring =
    new THREE.Mesh(

      new THREE.TorusGeometry(
        0.40,
        0.045,
        12,
        32
      ),

      material(
        colors().pipeDark,
        0.35,
        0.45
      )

    );


  ring.rotation.y =
    Math.PI / 2;


  ring.position.set(
    x,
    y,
    z
  );


  scene.add(ring);
}


// ============================================================
// ENVIRONMENT
// ============================================================

function createEnvironment() {

  const c = colors();


  // --------------------------
  // Floor
  // --------------------------

  const floor =
    new THREE.Mesh(

      new THREE.PlaneGeometry(
        34,
        22
      ),

      material(
        c.floor,
        0.9,
        0
      )

    );


  floor.rotation.x =
    -Math.PI / 2;


  floor.position.y =
    -0.25;


  floor.receiveShadow = true;


  floor.userData.pipeSenseType =
    'floor';


  scene.add(floor);


  // --------------------------
  // Grid
  // --------------------------

  const grid =
    new THREE.GridHelper(
      32,
      32,
      c.grid,
      c.grid
    );


  grid.position.y =
    -0.235;


  grid.material.transparent =
    true;


  grid.material.opacity =
    currentTheme === 'dark'
      ? 0.28
      : 0.38;


  grid.userData.pipeSenseType =
    'grid';


  scene.add(grid);


  // --------------------------
  // Back panel
  // --------------------------

  const board =
    new THREE.Mesh(

      new THREE.BoxGeometry(
        17,
        7.5,
        0.35
      ),

      material(
        c.backPanel,
        0.95,
        0
      )

    );


  board.position.set(
    1.5,
    3.4,
    -2.4
  );


  board.receiveShadow = true;


  board.userData.pipeSenseType =
    'backPanel';


  scene.add(board);


  // --------------------------
  // Platform
  // --------------------------

  const platform =
    new THREE.Mesh(

      new THREE.BoxGeometry(
        18,
        0.35,
        7
      ),

      material(
        c.platform,
        0.72,
        0.05
      )

    );


  platform.position.set(
    1,
    -0.02,
    0
  );


  platform.castShadow = true;

  platform.receiveShadow = true;


  platform.userData.pipeSenseType =
    'platform';


  scene.add(platform);
}


// ============================================================
// WATER TANK
// ============================================================

function createTank() {

  const c = colors();

  const group =
    new THREE.Group();


  group.position.set(
    -6.4,
    0.45,
    0
  );


  // --------------------------
  // Tank body
  // --------------------------

  const tank =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        1.25,
        1.32,
        2.2,
        48,
        1,
        true
      ),

      new THREE.MeshPhysicalMaterial({

        color: c.tank,

        transparent: true,

        opacity:
          currentTheme === 'dark'
            ? 0.58
            : 0.48,

        roughness: 0.15,

        metalness: 0.02,

        side: THREE.DoubleSide,

      })

    );


  tank.position.y =
    0.9;


  tank.castShadow = true;

  tank.receiveShadow = true;


  tank.userData.pipeSenseType =
    'tank';


  group.add(tank);


  // --------------------------
  // Bottom
  // --------------------------

  const bottom =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        1.32,
        1.32,
        0.18,
        48
      ),

      material(
        c.pipeDark,
        0.42,
        0.45
      )

    );


  bottom.position.y =
    -0.16;


  group.add(bottom);


  // --------------------------
  // Rim
  // --------------------------

  const rim =
    new THREE.Mesh(

      new THREE.TorusGeometry(
        1.27,
        0.08,
        16,
        48
      ),

      material(
        c.pipeDark,
        0.35,
        0.45
      )

    );


  rim.rotation.x =
    Math.PI / 2;


  rim.position.y =
    2.02;


  group.add(rim);


  // --------------------------
  // Tank water
  // --------------------------

  const water =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        1.12,
        1.12,
        1.35,
        48
      ),

      new THREE.MeshPhysicalMaterial({

        color: c.tankWater,

        transparent: true,

        opacity: 0.63,

        roughness: 0.08,

        metalness: 0,

      })

    );


  water.position.y =
    0.65;


  water.userData.pipeSenseType =
    'tankWater';


  group.add(water);


  // --------------------------
  // Water surface
  // --------------------------

  const surface =
    new THREE.Mesh(

      new THREE.CircleGeometry(
        1.12,
        48
      ),

      new THREE.MeshPhysicalMaterial({

        color: c.waterLight,

        transparent: true,

        opacity: 0.7,

        roughness: 0.05,

      })

    );


  surface.rotation.x =
    -Math.PI / 2;


  surface.position.y =
    1.34;


  surface.userData.pipeSenseType =
    'tankSurface';


  group.add(surface);


  // --------------------------
  // Tank legs
  // --------------------------

  for (
    let i = 0;
    i < 3;
    i++
  ) {

    const angle =
      (
        Math.PI * 2 * i
      ) / 3;


    const leg =
      new THREE.Mesh(

        new THREE.CylinderGeometry(
          0.1,
          0.12,
          0.55,
          16
        ),

        material(
          c.pipeDark,
          0.45,
          0.45
        )

      );


    leg.position.set(

      Math.cos(angle) * 0.85,

      -0.37,

      Math.sin(angle) * 0.85

    );


    group.add(leg);
  }


  scene.add(group);
}


// ============================================================
// PUMP
// ============================================================

function createPump() {

  const c = colors();

  const group =
    new THREE.Group();


  group.position.set(
    -4.8,
    0.7,
    0
  );


  // --------------------------
  // Motor
  // --------------------------

  const motor =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.52,
        0.52,
        1.65,
        32
      ),

      material(
        c.pipeDark,
        0.34,
        0.65
      )

    );


  motor.rotation.z =
    Math.PI / 2;


  motor.castShadow = true;


  group.add(motor);


  // --------------------------
  // Rear
  // --------------------------

  const rear =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.57,
        0.57,
        0.18,
        32
      ),

      material(
        c.pipeDark,
        0.32,
        0.7
      )

    );


  rear.rotation.z =
    Math.PI / 2;


  rear.position.x =
    -0.88;


  group.add(rear);


  // --------------------------
  // Pump housing
  // --------------------------

  const housing =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.62,
        0.62,
        0.52,
        32
      ),

      material(
        0x303936,
        0.32,
        0.6
      )

    );


  housing.rotation.z =
    Math.PI / 2;


  housing.position.x =
    0.9;


  group.add(housing);


  // --------------------------
  // Connector
  // --------------------------

  const connector =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.28,
        0.28,
        0.55,
        24
      ),

      material(
        c.pipe,
        0.4,
        0.35
      )

    );


  connector.rotation.z =
    Math.PI / 2;


  connector.position.x =
    1.32;


  group.add(connector);


  // --------------------------
  // Feet
  // --------------------------

  for (
    const x of [-0.55, 0.55]
  ) {

    const foot =
      new THREE.Mesh(

        new THREE.BoxGeometry(
          0.28,
          0.18,
          0.6
        ),

        material(
          c.pipeDark,
          0.45,
          0.45
        )

      );


    foot.position.set(
      x,
      -0.48,
      0
    );


    group.add(foot);
  }


  scene.add(group);


  // --------------------------
  // Pump pipe
  // --------------------------

  createPipeBetween(

    new THREE.Vector3(
      -5.25,
      0.72,
      0
    ),

    new THREE.Vector3(
      -4.0,
      0.72,
      0
    ),

    0.34,

    c.pipe

  );
}


// ============================================================
// MAIN PIPELINE
// ============================================================

function createMainPipeline() {

  const c = colors();


  createPipeBetween(

    new THREE.Vector3(
      -3.85,
      2.75,
      0
    ),

    new THREE.Vector3(
      7.2,
      2.75,
      0
    ),

    0.38,

    c.pipe

  );


  createPipeRing(
    -3.85,
    2.75,
    0
  );


  createPipeRing(
    7.2,
    2.75,
    0
  );
}


// ============================================================
// RETURN PIPELINE
// ============================================================

function createReturnPipeline() {

  const c = colors();


  const points = [

    new THREE.Vector3(
      6.65,
      0.45,
      -0.65
    ),

    new THREE.Vector3(
      5.4,
      0.45,
      -0.65
    ),

    new THREE.Vector3(
      3.3,
      0.55,
      -0.9
    ),

    new THREE.Vector3(
      0.8,
      0.62,
      -1.0
    ),

    new THREE.Vector3(
      -1.8,
      0.72,
      -0.9
    ),

    new THREE.Vector3(
      -3.8,
      0.78,
      -0.6
    ),

    new THREE.Vector3(
      -5.4,
      0.82,
      -0.15
    ),

  ];


  for (
    let i = 0;
    i < points.length - 1;
    i++
  ) {

    createPipeBetween(

      points[i],

      points[i + 1],

      0.25,

      c.pipe

    );
  }


  // --------------------------
  // Supports
  // --------------------------

  const supportPositions = [
    -1.8,
    0.8,
    3.3,
    5.4,
  ];


  supportPositions.forEach(
    (x) => {

      createPipeBetween(

        new THREE.Vector3(
          x,
          0.1,
          0
        ),

        new THREE.Vector3(
          x,
          2.75,
          0
        ),

        0.07,

        c.support

      );

    }
  );
}


// ============================================================
// FLOW SENSOR
// ============================================================

function createFlowSensor(
  x,
  color
) {

  const group =
    new THREE.Group();


  group.position.set(
    x,
    2.75,
    0
  );


  // Body

  const body =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.48,
        0.48,
        0.72,
        32
      ),

      material(
        color,
        0.3,
        0.35
      )

    );


  body.rotation.z =
    Math.PI / 2;


  body.castShadow = true;


  group.add(body);


  // Center

  const center =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.3,
        0.3,
        0.76,
        32
      ),

      material(
        0x202826,
        0.35,
        0.45
      )

    );


  center.rotation.z =
    Math.PI / 2;


  group.add(center);


  // Electronics box

  const box =
    new THREE.Mesh(

      new THREE.BoxGeometry(
        0.48,
        0.32,
        0.38
      ),

      material(
        color,
        0.32,
        0.3
      )

    );


  box.position.y =
    0.48;


  group.add(box);


  // Indicator

  const indicator =
    new THREE.Mesh(

      new THREE.SphereGeometry(
        0.055,
        12,
        12
      ),

      new THREE.MeshBasicMaterial({
        color: 0xff5f73,
      })

    );


  indicator.position.set(
    0.08,
    0.67,
    0
  );


  group.add(indicator);


  scene.add(group);
}


// ============================================================
// VIBRATION SENSOR
// ============================================================

function createVibrationSensor(
  x
) {

  const group =
    new THREE.Group();


  group.position.set(
    x,
    3.55,
    0
  );


  const body =
    new THREE.Mesh(

      new THREE.BoxGeometry(
        0.55,
        0.24,
        0.45
      ),

      material(
        0x222a27,
        0.34,
        0.3
      )

    );


  body.castShadow = true;


  group.add(body);


  const cap =
    new THREE.Mesh(

      new THREE.BoxGeometry(
        0.30,
        0.12,
        0.25
      ),

      material(
        colors().green,
        0.3,
        0.25
      )

    );


  cap.position.y =
    0.16;


  group.add(cap);


  scene.add(group);
}


// ============================================================
// ALL SENSORS
// ============================================================

function createSensors() {

  // YF-S201

  createFlowSensor(
    -2.55,
    colors().green
  );


  // ZJ-S201

  createFlowSensor(
    5.95,
    colors().blue
  );


  // MPU6050 #1

  createVibrationSensor(
    0.1
  );


  // MPU6050 #2

  createVibrationSensor(
    3.25
  );
}


// ============================================================
// LEAK ZONES
// ============================================================

function createLeakZones() {

  Object.entries(
    leakZones
  ).forEach(
    ([zoneName, zone]) => {

      createLeakOutlet(
        zoneName,
        zone
      );

      createLeakParticles(
        zoneName,
        zone
      );

    }
  );
}


// ============================================================
// LEAK OUTLET
// ============================================================

function createLeakOutlet(
  zoneName,
  zone
) {

  const c = colors();


  const group =
    new THREE.Group();


  group.position.set(
    zone.x,
    2.75,
    0
  );


  // Outlet pipe

  const outlet =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.12,
        0.12,
        0.55,
        20
      ),

      material(
        c.pipeDark,
        0.42,
        0.4
      )

    );


  outlet.position.y =
    -0.42;


  group.add(outlet);


  // Outlet head

  const head =
    new THREE.Mesh(

      new THREE.CylinderGeometry(
        0.16,
        0.16,
        0.22,
        20
      ),

      material(
        c.pipe,
        0.4,
        0.25
      )

    );


  head.position.y =
    -0.73;


  group.add(head);


  // Warning ring

  const ring =
    new THREE.Mesh(

      new THREE.TorusGeometry(
        0.20,
        0.035,
        10,
        24
      ),

      material(
        c.normal,
        0.4,
        0.15
      )

    );


  ring.rotation.x =
    Math.PI / 2;


  ring.position.set(
    0,
    -0.78,
    0
  );


  ring.visible = false;


  group.add(ring);


  scene.add(group);


  zone.outlet =
    group;

  zone.ring =
    ring;
}


// ============================================================
// LEAK PARTICLES
// ============================================================

function createLeakParticles(
  zoneName,
  zone
) {

  const c = colors();


  // --------------------------
  // Droplets
  // --------------------------

  for (
    let i = 0;
    i < 30;
    i++
  ) {

    const droplet =
      new THREE.Mesh(

        new THREE.SphereGeometry(

          0.055 +
          Math.random() * 0.04,

          10,

          10

        ),

        new THREE.MeshPhysicalMaterial({

          color: c.waterLight,

          transparent: true,

          opacity: 0.85,

          roughness: 0.05,

          metalness: 0.02,

        })

      );


    droplet.visible =
      false;


    droplet.userData = {

      phase:
        Math.random() *
        Math.PI *
        2,

      speed:
        0.65 +
        Math.random() *
        0.75,

      offset:
        Math.random(),

      spread:
        (
          Math.random() -
          0.5
        ) * 0.24,

    };


    scene.add(
      droplet
    );


    zone.particles.push(
      droplet
    );
  }


  // --------------------------
  // Water pool
  // --------------------------

  const pool =
    new THREE.Mesh(

      new THREE.CircleGeometry(
        0.44,
        40
      ),

      new THREE.MeshPhysicalMaterial({

        color: c.water,

        transparent: true,

        opacity: 0.42,

        roughness: 0.12,

        metalness: 0,

      })

    );


  pool.rotation.x =
    -Math.PI / 2;


  pool.position.set(
    zone.x,
    0.17,
    0.25
  );


  pool.visible =
    false;


  scene.add(pool);


  zone.pool =
    pool;


  // --------------------------
  // Splash ring
  // --------------------------

  const splash =
    new THREE.Mesh(

      new THREE.TorusGeometry(
        0.35,
        0.035,
        8,
        36
      ),

      new THREE.MeshBasicMaterial({

        color: c.waterLight,

        transparent: true,

        opacity: 0.8,

      })

    );


  splash.rotation.x =
    Math.PI / 2;


  splash.position.set(
    zone.x,
    0.19,
    0.25
  );


  splash.visible =
    false;


  scene.add(
    splash
  );


  zone.splash =
    splash;
}


// ============================================================
// MAIN WATER FLOW
// ============================================================

function createWaterFlow() {

  const c = colors();


  for (
    let i = 0;
    i < 80;
    i++
  ) {

    const particle =
      new THREE.Mesh(

        new THREE.SphereGeometry(
          0.055,
          8,
          8
        ),

        new THREE.MeshBasicMaterial({

          color:
            i % 3 === 0
              ? c.waterLight
              : c.water,

        })

      );


    particle.userData = {

      offset:
        Math.random(),

      speed:
        0.035 +
        Math.random() *
        0.025,

      y:
        2.58 +
        Math.random() *
        0.34,

      z:
        (
          Math.random() -
          0.5
        ) * 0.23,

    };


    scene.add(
      particle
    );


    mainWaterParticles.push(
      particle
    );
  }
}


// ============================================================
// RETURN WATER FLOW
// ============================================================

function createReturnWaterFlow() {

  const c = colors();


  for (
    let i = 0;
    i < 28;
    i++
  ) {

    const particle =
      new THREE.Mesh(

        new THREE.SphereGeometry(
          0.04,
          7,
          7
        ),

        new THREE.MeshBasicMaterial({

          color:
            c.water,

        })

      );


    particle.userData = {

      offset:
        Math.random(),

      speed:
        0.012 +
        Math.random() *
        0.012,

    };


    scene.add(
      particle
    );


    returnWaterParticles.push(
      particle
    );
  }
}


// ============================================================
// LEAK ANIMATION
// ============================================================

function updateLeakParticles(
  elapsed
) {

  Object.entries(
    leakZones
  ).forEach(
    ([zoneName, zone]) => {

      const active =
        currentSensorState.leakDetected &&
        currentSensorState.leakZone === zoneName;


      // --------------------------
      // Outlet
      // --------------------------

      if (zone.outlet) {

        zone.outlet.children.forEach(
          (child) => {

            if (
              child.material &&
              child.material.color
            ) {

              if (active) {

                child.material.color.setHex(
                  colors().warning
                );

              }

            }

          }
        );

      }


      // --------------------------
      // Ring
      // --------------------------

      if (zone.ring) {

        zone.ring.visible =
          active;


        if (active) {

          const pulse =
            1 +
            Math.sin(
              elapsed * 5
            ) *
            0.12;


          zone.ring.scale.set(
            pulse,
            pulse,
            pulse
          );


          zone.ring.material.color.setHex(
            colors().warning
          );

        }

      }


      // --------------------------
      // Pool
      // --------------------------

      if (zone.pool) {

        zone.pool.visible =
          active;


        if (active) {

          const pulse =
            1 +
            Math.sin(
              elapsed * 2.8
            ) *
            0.08;


          zone.pool.scale.set(
            pulse,
            pulse,
            1
          );

        }

      }


      // --------------------------
      // Splash
      // --------------------------

      if (zone.splash) {

        zone.splash.visible =
          active;


        if (active) {

          const pulse =
            1 +
            Math.sin(
              elapsed * 5
            ) *
            0.18;


          zone.splash.scale.set(
            pulse,
            pulse,
            pulse
          );

        }

      }


      // --------------------------
      // Droplets
      // --------------------------

      zone.particles.forEach(
        (particle) => {

          particle.visible =
            active;


          if (!active) {
            return;
          }


          const data =
            particle.userData;


          const progress =
            (
              elapsed *
              data.speed +
              data.offset
            ) % 1;


          particle.position.set(

            zone.x +

            data.spread *
            Math.sin(
              progress *
              Math.PI *
              4 +
              data.phase
            ),

            2.15 -
            progress *
            1.95,

            data.spread *
            Math.cos(
              progress *
              Math.PI *
              3 +
              data.phase
            )

          );


          const scale =
            1 -
            progress *
            0.35;


          particle.scale.set(
            scale,
            scale,
            scale
          );

        }
      );

    }
  );
}


// ============================================================
// WATER ANIMATION
// ============================================================

function updateWater(
  elapsed
) {

  // --------------------------
  // Main pipeline
  // --------------------------

  mainWaterParticles.forEach(
    (particle) => {

      const data =
        particle.userData;


      const progress =
        (
          elapsed *
          data.speed +
          data.offset
        ) % 1;


      particle.position.set(

        -3.45 +
        progress *
        10.3,

        data.y,

        data.z

      );

    }
  );


  // --------------------------
  // Return pipeline
  // --------------------------

  returnWaterParticles.forEach(
    (particle) => {

      const data =
        particle.userData;


      const progress =
        (
          elapsed *
          data.speed +
          data.offset
        ) % 1;


      particle.position.set(

        6.5 -
        progress *
        11.5,

        0.48 +
        Math.sin(
          progress *
          Math.PI
        ) *
        0.25,

        -0.75

      );

    }
  );
}


// ============================================================
// SENSOR STATE APPLICATION
// ============================================================

function applySensorState() {

  if (!initialized) {
    return;
  }


  Object.entries(
    leakZones
  ).forEach(
    ([zoneName, zone]) => {

      const active =
        currentSensorState.leakDetected &&
        currentSensorState.leakZone === zoneName;


      if (zone.ring) {
        zone.ring.visible =
          active;
      }


      if (zone.pool) {
        zone.pool.visible =
          active;
      }


      if (zone.splash) {
        zone.splash.visible =
          active;
      }


      zone.particles.forEach(
        (particle) => {

          particle.visible =
            active;

        }
      );

    }
  );
}


// ============================================================
// THEME UPDATE
// ============================================================
//
// IMPORTANT FIX:
//
// We DO NOT recreate the scene.
// We DO NOT recreate the renderer.
// We DO NOT remove the canvas.
//
// Flutter may replace the HtmlElementView container.
// We simply reconnect the existing renderer canvas.
// ============================================================

function setTheme(
  theme
) {

  if (
    theme !== 'dark' &&
    theme !== 'light'
  ) {

    theme = 'light';

  }


  currentTheme =
    theme;


  // Scene may not exist yet.

  if (!scene) {
    return;
  }


  // --------------------------
  // Background
  // --------------------------

  scene.background =
    new THREE.Color(
      colors().background
    );


  // --------------------------
  // Renderer exposure
  // --------------------------

  if (renderer) {

    renderer.toneMappingExposure =
      currentTheme === 'dark'
        ? 1.12
        : 1.05;

  }


  // --------------------------
  // Update known materials
  // --------------------------

  updateThemeMaterials();


  // --------------------------
  // Most important part:
  // make sure the canvas is
  // connected to the current
  // Flutter HTML container.
  // --------------------------

  ensureRendererContainer();


  resizeRenderer();

}


// ============================================================
// THEME MATERIAL UPDATE
// ============================================================

function updateThemeMaterials() {

  if (!scene) {
    return;
  }


  const c = colors();


  scene.traverse(
    (object) => {

      if (
        !object.material
      ) {
        return;
      }


      const type =
        object.userData
          ?.pipeSenseType;


      if (!type) {
        return;
      }


      if (type === 'floor') {

        object.material.color.setHex(
          c.floor
        );

      }


      if (type === 'platform') {

        object.material.color.setHex(
          c.platform
        );

      }


      if (type === 'backPanel') {

        object.material.color.setHex(
          c.backPanel
        );

      }


      if (type === 'tank') {

        object.material.color.setHex(
          c.tank
        );

        object.material.opacity =
          currentTheme === 'dark'
            ? 0.58
            : 0.48;

      }


      if (type === 'tankWater') {

        object.material.color.setHex(
          c.tankWater
        );

      }


      if (type === 'tankSurface') {

        object.material.color.setHex(
          c.waterLight
        );

      }

    }
  );


  // --------------------------
  // Grid
  // --------------------------

  scene.traverse(
    (object) => {

      if (
        object.userData
          ?.pipeSenseType ===
        'grid'
      ) {

        object.material.color.setHex(
          c.grid
        );


        object.material.opacity =
          currentTheme === 'dark'
            ? 0.28
            : 0.38;

      }

    }
  );

}


// ============================================================
// FIND CURRENT FLUTTER CONTAINER
// ============================================================

function findPipeSenseContainer() {

  return document.getElementById(
    'pipe-sense-three-container'
  );

}


// ============================================================
// ENSURE RENDERER IS CONNECTED
// ============================================================
//
// This is the main glitch fix.
//
// If Flutter replaces the HtmlElementView:
//
// old container
//     |
//     +-- canvas
//
// becomes:
//
// new container
//
// The renderer still exists, so we move the existing canvas:
//
// renderer.domElement
//          |
//          V
// new container
// ============================================================

function ensureRendererContainer() {

  if (!renderer) {
    return;
  }


  const currentContainer =
    findPipeSenseContainer();


  if (!currentContainer) {
    return;
  }


  // If Flutter created a new
  // container, update our reference.

  if (
    container !==
    currentContainer
  ) {

    container =
      currentContainer;

  }


  const canvas =
    renderer.domElement;


  if (
    canvas.parentElement !==
    container
  ) {

    // Remove accidental
    // duplicate canvas children.

    while (
      container.firstChild
    ) {

      if (
        container.firstChild !==
        canvas
      ) {

        container.removeChild(
          container.firstChild
        );

      } else {

        break;

      }

    }


    container.appendChild(
      canvas
    );

  }


  // Make sure Flutter's
  // container remains full size.

  container.style.width =
    '100%';

  container.style.height =
    '100%';

  container.style.overflow =
    'hidden';

  container.style.backgroundColor =
    'transparent';


  // Make canvas fill it.

  canvas.style.width =
    '100%';

  canvas.style.height =
    '100%';

  canvas.style.display =
    'block';

}


// ============================================================
// RESIZE
// ============================================================

function resizeRenderer() {

  if (
    !renderer ||
    !camera
  ) {

    return;

  }


  ensureRendererContainer();


  if (!container) {
    return;
  }


  const width =
    container.clientWidth;


  const height =
    container.clientHeight;


  if (
    width < 20 ||
    height < 20
  ) {

    return;

  }


  camera.aspect =
    width / height;


  camera.updateProjectionMatrix();


  renderer.setSize(
    width,
    height,
    false
  );

}


// ============================================================
// RESIZE OBSERVER
// ============================================================

function setupResizeObserver() {

  if (!container) {
    return;
  }


  if (resizeObserver) {

    resizeObserver.disconnect();

  }


  resizeObserver =
    new ResizeObserver(
      () => {

        ensureRendererContainer();

        resizeRenderer();

      }
    );


  resizeObserver.observe(
    container
  );


  window.addEventListener(
    'resize',
    resizeRenderer
  );


  requestAnimationFrame(
    resizeRenderer
  );


  setTimeout(
    resizeRenderer,
    50
  );


  setTimeout(
    resizeRenderer,
    250
  );


  setTimeout(
    resizeRenderer,
    600
  );

}


// ============================================================
// DOM MUTATION OBSERVER
// ============================================================
//
// Flutter can replace the platform-view DOM node.
// This observer catches that replacement.
// ============================================================

function setupMutationObserver() {

  if (mutationObserver) {
    return;
  }


  mutationObserver =
    new MutationObserver(
      () => {

        const newContainer =
          findPipeSenseContainer();


        if (
          newContainer &&
          newContainer !==
          container
        ) {

          container =
            newContainer;


          ensureRendererContainer();

          resizeRenderer();

        }

      }
    );


  mutationObserver.observe(
    document.body,
    {
      childList: true,
      subtree: true,
    }
  );

}


// ============================================================
// ANIMATION LOOP
// ============================================================

function animate() {

  requestAnimationFrame(
    animate
  );


  if (
    !scene ||
    !renderer ||
    !camera
  ) {

    return;

  }


  // Always check whether
  // Flutter replaced the container.

  ensureRendererContainer();


  timer.update();


  const elapsed =
    timer.getElapsed();


  if (controls) {

    controls.update();

  }


  updateWater(
    elapsed
  );


  updateLeakParticles(
    elapsed
  );


  renderer.render(
    scene,
    camera
  );

}


// ============================================================
// CREATE SCENE
// ============================================================

function createScene() {

  if (
    initialized ||
    initializing
  ) {

    return;

  }


  initializing =
    true;


  // --------------------------
  // Find Flutter container
  // --------------------------

  container =
    findPipeSenseContainer();


  if (!container) {

    initializing =
      false;


    requestAnimationFrame(
      createScene
    );


    return;

  }


  const width =
    container.clientWidth;


  const height =
    container.clientHeight;


  if (
    width < 20 ||
    height < 20
  ) {

    initializing =
      false;


    requestAnimationFrame(
      createScene
    );


    return;

  }


  // ==========================================================
  // SCENE
  // ==========================================================

  scene =
    new THREE.Scene();


  scene.background =
    new THREE.Color(
      colors().background
    );


  // ==========================================================
  // CAMERA
  // ==========================================================

  camera =
    new THREE.PerspectiveCamera(

      42,

      width / height,

      0.1,

      100

    );


  // Negative X camera angle.

  camera.position.set(
    -13,
    7.5,
    15
  );


  camera.lookAt(
    0,
    1.7,
    0
  );


  // ==========================================================
  // RENDERER
  // ==========================================================

  renderer =
    new THREE.WebGLRenderer({

      antialias: true,

      alpha: false,

      powerPreference:
        'high-performance',

    });


  renderer.setPixelRatio(

    Math.min(

      window.devicePixelRatio || 1,

      2

    )

  );


  renderer.setSize(
    width,
    height,
    false
  );


  renderer.shadowMap.enabled =
    true;


  renderer.shadowMap.type =
    THREE.PCFShadowMap;


  renderer.outputColorSpace =
    THREE.SRGBColorSpace;


  renderer.toneMapping =
    THREE.ACESFilmicToneMapping;


  renderer.toneMappingExposure =
    currentTheme === 'dark'
      ? 1.12
      : 1.05;


  // --------------------------
  // Canvas styling
  // --------------------------

  renderer.domElement.style.width =
    '100%';

  renderer.domElement.style.height =
    '100%';

  renderer.domElement.style.display =
    'block';


  // --------------------------
  // IMPORTANT
  // --------------------------
  //
  // Do not remove the canvas
  // from the container repeatedly.
  //
  // Just attach it once.
  // --------------------------

  ensureRendererContainer();


  // ==========================================================
  // ORBIT CONTROLS
  // ==========================================================

  controls =
    new OrbitControls(

      camera,

      renderer.domElement

    );


  controls.enableDamping =
    true;


  controls.dampingFactor =
    0.075;


  controls.enablePan =
    true;


  controls.minDistance =
    7;


  controls.maxDistance =
    28;


  controls.minPolarAngle =
    0.45;


  controls.maxPolarAngle =
    1.48;


  controls.target.set(
    0,
    1.5,
    0
  );


  controls.update();


  // ==========================================================
  // LIGHTING
  // ==========================================================

  const hemisphere =
    new THREE.HemisphereLight(

      0xffffff,

      0x65716d,

      currentTheme === 'dark'
        ? 1.35
        : 1.65

    );


  scene.add(
    hemisphere
  );


  const key =
    new THREE.DirectionalLight(

      0xffffff,

      currentTheme === 'dark'
        ? 2.2
        : 2.7

    );


  key.position.set(
    -7,
    12,
    10
  );


  key.castShadow =
    true;


  key.shadow.mapSize.width =
    1024;


  key.shadow.mapSize.height =
    1024;


  key.shadow.camera.left =
    -15;


  key.shadow.camera.right =
    15;


  key.shadow.camera.top =
    15;


  key.shadow.camera.bottom =
    -10;


  key.shadow.camera.near =
    0.5;


  key.shadow.camera.far =
    40;


  scene.add(
    key
  );


  const fill =
    new THREE.DirectionalLight(

      0xbfefff,

      1.15

    );


  fill.position.set(
    10,
    7,
    5
  );


  scene.add(
    fill
  );


  const rim =
    new THREE.PointLight(

      colors().waterLight,

      1.3,

      25

    );


  rim.position.set(
    3,
    5,
    -6
  );


  scene.add(
    rim
  );


  // ==========================================================
  // CREATE MODEL
  // ==========================================================

  createEnvironment();

  createTank();

  createPump();

  createMainPipeline();

  createReturnPipeline();

  createSensors();

  createLeakZones();

  createWaterFlow();

  createReturnWaterFlow();


  // ==========================================================
  // RESIZE / DOM WATCHING
  // ==========================================================

  setupResizeObserver();

  setupMutationObserver();


  // ==========================================================
  // INITIALIZED
  // ==========================================================

  initialized =
    true;


  initializing =
    false;


  applySensorState();


  // ==========================================================
  // START ANIMATION ONLY ONCE
  // ==========================================================

  if (
    !animationStarted
  ) {

    animationStarted =
      true;


    animate();

  }


  requestAnimationFrame(
    () => {

      ensureRendererContainer();

      resizeRenderer();

    }
  );

}


// ============================================================
// UPDATE SENSOR STATE
// ============================================================

function updateSensorState(

  yfFlowRate,

  zjFlowRate,

  vibration1,

  vibration2,

  leakDetected,

  leakZone

) {

  currentSensorState = {

    yfFlowRate:
      Number(yfFlowRate) || 0,

    zjFlowRate:
      Number(zjFlowRate) || 0,

    vibration1:
      Number(vibration1) || 0,

    vibration2:
      Number(vibration2) || 0,

    leakDetected:
      Boolean(leakDetected),

    leakZone:
      leakZone || null,

  };


  if (!initialized) {

    return;

  }


  applySensorState();

}


// ============================================================
// MANUAL ZONE SELECTION DISABLED
// ============================================================

function selectZone() {

  // Intentionally disabled.

  return;

}


// ============================================================
// SET LEAK ZONE
// ============================================================
//
// Kept for compatibility with existing Dart code.
// The application should normally determine the zone.
// ============================================================

function setLeakZone(
  zone
) {

  if (
    !currentSensorState.leakDetected
  ) {

    return;

  }


  if (
    zone !== 'Zone 1' &&
    zone !== 'Zone 2' &&
    zone !== 'Zone 3'
  ) {

    return;

  }


  currentSensorState.leakZone =
    zone;


  applySensorState();

}


// ============================================================
// CLEAR LEAK
// ============================================================

function clearLeak() {

  currentSensorState.leakDetected =
    false;


  currentSensorState.leakZone =
    null;


  applySensorState();

}


// ============================================================
// GET STATE
// ============================================================

function getState() {

  return {

    initialized,

    theme:
      currentTheme,

    sensorState: {
      ...currentSensorState,
    },

  };

}


// ============================================================
// GLOBAL API
// ============================================================
//
// Flutter communicates with Three.js through this object.
// ============================================================

window.pipeSense3D = {

  updateSensorState,

  selectZone,

  setLeakZone,

  clearLeak,

  getState,

  setTheme,

};


// ============================================================
// START
// ============================================================

function startPipeSenseScene() {

  if (
    initialized ||
    initializing
  ) {

    return;

  }


  requestAnimationFrame(
    () => {

      requestAnimationFrame(
        () => {

          createScene();

        }
      );

    }
  );

}


startPipeSenseScene();