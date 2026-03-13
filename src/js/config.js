const ITEM_SLOT_COLS = 8;
const ITEM_SLOT_ROWS = 3;
const BOTTOM_THIRD_TOP = 100 * (2 / 3);
const BOTTOM_THIRD_HEIGHT = 100 / 3;
const ITEM_ROW_SCALES = [0.82, 1, 1.12];

// Screen-space anchor points for actors rendered into the room overlay.
// The renderer currently uses these as a fixed left-to-right staging row for
// dwarves and any other "actor" sprites that are not tied to room geometry.
// These are data rather than logic so the layout can be tuned without digging
// through renderer behavior.
export const ACTOR_SLOTS = [
  { x: 10, y: 82 },
  { x: 30, y: 82 },
  { x: 50, y: 82 },
  { x: 70, y: 82 },
  { x: 90, y: 82 },
];

// Canonical object ids for rooms and overlays that have special-case visual
// behavior. These ids come from the original adventure object table exposed
// through the WASM bridge, and the renderer needs stable symbolic names for
// them because state-dependent artwork is keyed off object prop/place/fixed
// values rather than generic item definitions.
export const SPECIAL_OBJECTS = {
  lamp: 2,
  grate: 3,
  chasm: 32,
  fissur: 12,
  plant: 24,
  snake: 11,
  dragon: 31,
  door: 9,
  clam: 14,
  oyster: 15,
  troll: 33,
  bear: 35,
  chain: 64,
};

// Inventory slot positions expressed in percentages of the overlay area.
// This is generated from a simple 8x3 grid so item placement stays consistent
// and can be scaled by row to fake a little depth in the lower third of the UI.
export const ITEM_SLOTS = Array.from({ length: ITEM_SLOT_COLS * ITEM_SLOT_ROWS }, (_, idx) => {
  const row = Math.floor(idx / ITEM_SLOT_COLS);
  const col = idx % ITEM_SLOT_COLS;
  const x = ((col + 0.5) / ITEM_SLOT_COLS) * 100;
  const y = BOTTOM_THIRD_TOP + ((row + 1) / ITEM_SLOT_ROWS) * BOTTOM_THIRD_HEIGHT;

  return {
    x,
    y,
    row,
    scale: ITEM_ROW_SCALES[row] || 1,
  };
});

// Visual metadata for concrete inventory/world objects that currently have
// bespoke sprites. `obj` is the canonical game object id; `startLoc` and
// `startSlot` seed the initial presentation before live state is read from the
// engine; `scalePct` compensates for source art that was authored at different
// apparent sizes. Objects omitted here fall back to text-only behavior or have
// not been given custom artwork yet.
export const ITEM_DEFS = [
  { obj: 1, sprite: 'keys.webp', startLoc: 3, startSlot: 19, scalePct: 50 },
  { obj: 2, sprite: 'lamp.webp', startLoc: 3, startSlot: 23, scalePct: 100 },
  { obj: 4, sprite: 'cage.webp', startLoc: 10, startSlot: 16, scalePct: 100 },
  { obj: 5, sprite: 'rod.webp', startLoc: 11, startSlot: 21, scalePct: 100 },
  { obj: 8, sprite: 'bird.webp', startLoc: 13, startSlot: 18, scalePct: 100 },
  { obj: 10, sprite: 'pillow.webp', startLoc: 96, startSlot: 22, scalePct: 100 },
  { obj: 14, sprite: 'clam.webp', startLoc: 103, startSlot: 17, scalePct: 200 },
  { obj: 16, sprite: 'magazine.webp', startLoc: 106, startSlot: 23, scalePct: 100 },
  { obj: 19, sprite: 'food.webp', startLoc: 3, startSlot: 18, scalePct: 100 },
  { obj: 20, sprite: 'bottle+full.webp', startLoc: 3, startSlot: 20, scalePct: 50 },
  { obj: 28, sprite: 'axe.webp', startLoc: 0, startSlot: 16, scalePct: 100 },
  { obj: 39, sprite: 'batteries.webp', startLoc: 0, startSlot: 21, scalePct: 100 },
  { obj: 50, sprite: 'gold.webp', startLoc: 18, startSlot: 17, scalePct: 100 },
  { obj: 51, sprite: 'diamonds.webp', startLoc: 27, startSlot: 22, scalePct: 100 },
  { obj: 52, sprite: 'silver.webp', startLoc: 28, startSlot: 19, scalePct: 100 },
  { obj: 53, sprite: 'jewels.webp', startLoc: 29, startSlot: 23, scalePct: 100 },
  { obj: 54, sprite: 'coins.webp', startLoc: 30, startSlot: 17, scalePct: 100 },
  { obj: 55, sprite: 'chest.webp', startLoc: 0, startSlot: 18, scalePct: 150 },
  { obj: 56, sprite: 'eggs.webp', startLoc: 92, startSlot: 18, scalePct: 120 },
  { obj: 57, sprite: 'trident.webp', startLoc: 95, startSlot: 11, scalePct: 100 },
  { obj: 58, sprite: 'vase.webp', startLoc: 97, startSlot: 4, scalePct: 100 },
  { obj: 59, sprite: 'emerald.webp', startLoc: 100, startSlot: 12, scalePct: 100 },
  { obj: 60, sprite: 'platinum.webp', startLoc: 101, startSlot: 14, scalePct: 100 },
  { obj: 61, sprite: 'pearl.webp', startLoc: 0, startSlot: 22, scalePct: 100 },
  { obj: 62, sprite: 'rug.webp', startLoc: 120, startSlot: 18, scalePct: 300 },
  { obj: 63, sprite: 'spices.webp', startLoc: 127, startSlot: 17, scalePct: 200 },
  { obj: 64, sprite: 'chain.webp', startLoc: 130, startSlot: 6, scalePct: 180 },
];
