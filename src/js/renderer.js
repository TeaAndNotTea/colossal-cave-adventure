import { ITEM_DEFS, ITEM_SLOTS } from './config.js';

const ACTOR_SLOTS = [
  { x: 10, y: 82 },
  { x: 30, y: 82 },
  { x: 50, y: 82 },
  { x: 70, y: 82 },
  { x: 90, y: 82 },
];

const SPECIAL_OBJECTS = {
  lamp: 2,
  grate: 3,
  fissur: 12,
  plant: 24,
  snake: 11,
  dragon: 31,
  door: 9,
  clam: 14,
  troll: 33,
  bear: 35,
  chain: 64,
};

export class AdventRenderer {
  constructor({ ui, assetBaseUrl, state }) {
    this.ui = ui;
    this.assetBaseUrl = assetBaseUrl;
    this.state = state;
    this.bridge = null;
  }

  setBridge(bridge) {
    this.bridge = bridge;
  }

  setRoomImageByKey(key) {
    if (!this.ui.imageEl || !key || key === this.state.lastImageKey) {
      return;
    }
    this.state.lastImageKey = key;
    this.ui.imageEl.src = `${this.assetBaseUrl}rooms/${key}.webp`;
    this.ui.imageEl.alt = `Location ${String(key)}`;
  }

  updateRoomImage(loc) {
    if (typeof loc !== 'number' || loc <= 0) {
      return;
    }
    this.setRoomImageByKey(String(loc));
  }

  hashSlot(obj, loc) {
    let x = ((obj | 0) * 1103515245 + (loc | 0) * 12345) | 0;
    x ^= x >>> 16;
    if (x < 0) {
      x = -x;
    }
    return x;
  }

  pickSlotIndex(def, loc, alreadyUsed) {
    const preferred =
      def &&
      def.startLoc > 0 &&
      def.startLoc === loc &&
      typeof def.startSlot === 'number'
        ? def.startSlot | 0
        : -1;

    const n = ITEM_SLOTS.length;
    if (n <= 0) {
      return 0;
    }

    if (preferred >= 0) {
      const p = ((preferred % n) + n) % n;
      if (!alreadyUsed[p]) {
        return p;
      }
    }

    const h = this.hashSlot(def.obj | 0, loc | 0) % n;
    for (let i = 0; i < n; i += 1) {
      const idx = (h + i) % n;
      if (!alreadyUsed[idx]) {
        return idx;
      }
    }

    return h;
  }

  clearItemLayer() {
    if (!this.ui.itemLayerEl) {
      return;
    }
    while (this.ui.itemLayerEl.firstChild) {
      this.ui.itemLayerEl.removeChild(this.ui.itemLayerEl.firstChild);
    }
  }

  clearActorLayer() {
    if (!this.ui.actorLayerEl) {
      return;
    }
    while (this.ui.actorLayerEl.firstChild) {
      this.ui.actorLayerEl.removeChild(this.ui.actorLayerEl.firstChild);
    }
  }

  pickActorSlotIndex(seed, loc, alreadyUsed) {
    const n = ACTOR_SLOTS.length;
    if (n <= 0) {
      return 0;
    }

    const h = this.hashSlot(seed | 0, loc | 0) % n;
    for (let i = 0; i < n; i += 1) {
      const idx = (h + i) % n;
      if (!alreadyUsed[idx]) {
        return idx;
      }
    }

    return h;
  }

  renderRoomItems(loc) {
    if (!this.ui.itemLayerEl || !this.bridge || !this.bridge.hasObjectLocationAccess()) {
      return;
    }

    this.clearItemLayer();
    const used = Object.create(null);
    const cagePlace = this.bridge.getObjectPlace(4);
    const cageFixed = this.bridge.getObjectFixed(4);
    const rodPlace = this.bridge.getObjectPlace(5);
    const pillowPlace = this.bridge.getObjectPlace(10);
    const pillowFixed = this.bridge.getObjectFixed(10);
    const birdPlace = this.bridge.getObjectPlace(8);
    const birdFixed = this.bridge.getObjectFixed(8);
    const birdProp = this.bridge.getObjectProp(8);
    const bottleProp = this.bridge.getObjectProp(20);
    const vasePlace = this.bridge.getObjectPlace(58);
    const vaseFixed = this.bridge.getObjectFixed(58);
    const vaseProp = this.bridge.getObjectProp(58);
    const cageHere = cagePlace === loc || cageFixed === loc;
    const pillowHere = pillowPlace === loc || pillowFixed === loc;
    const birdHere = birdPlace === loc || birdFixed === loc;
    const vaseHere = vasePlace === loc || vaseFixed === loc;
    const rodCarried = rodPlace === -1;
    const birdCaptured = birdProp != null && birdProp !== -9999 && birdProp !== 0;
    const birdDisturbed = this.bridge.getBirdDisturbed() === 1;
    const vaseIntact = vaseProp === 0;
    const showDisturbedBird = birdHere && rodCarried && !birdCaptured && birdDisturbed;
    const showCageWithBird = cageHere && birdHere && birdCaptured;
    const showEmptyBottle = bottleProp === 1;
    const showBrokenVase = vaseProp === 2;
    const showPillowWithVase = vaseHere && vaseIntact && (pillowHere || loc === 96);
    const renderPillowVaseFromVase = showPillowWithVase && !pillowHere;

    if (renderPillowVaseFromVase) {
      used[0] = true;
    }

    for (const def of ITEM_DEFS) {
      const obj = def.obj | 0;
      if (obj <= 0) {
        continue;
      }
      if (showCageWithBird && obj === 8) {
        continue;
      }
      if (showPillowWithVase && pillowHere && obj === 58) {
        continue;
      }

      const place = this.bridge.getObjectPlace(obj);
      const fixed = this.bridge.getObjectFixed(obj);
      const here = place === loc || fixed === loc;
      if (!here) {
        continue;
      }

      const slotIndex =
        renderPillowVaseFromVase && obj === 58 ? 0 : this.pickSlotIndex(def, loc, used);
      used[slotIndex] = true;

      const slot = ITEM_SLOTS[slotIndex] || ITEM_SLOTS[0];
      const sprite =
        showCageWithBird && obj === 4
          ? 'cage+bird.webp'
          : showPillowWithVase && pillowHere && obj === 10
            ? 'pillow+vase.webp'
            : renderPillowVaseFromVase && obj === 58
              ? 'pillow+vase.webp'
            : showDisturbedBird && obj === 8
              ? 'bird+disturbed.webp'
            : showEmptyBottle && obj === 20
              ? 'bottle+empty.webp'
            : showBrokenVase && obj === 58
              ? 'vase+broken.webp'
            : def.sprite;
      const img = document.createElement('img');
      img.src = `${this.assetBaseUrl}items/${sprite}`;
      img.alt = sprite.replace(/\.webp$/i, '');
      img.style.position = 'absolute';
      img.style.left = `${String(slot.x)}%`;
      img.style.top = `${String(slot.y)}%`;
      img.style.transform = 'translate(-50%, -50%)';
      img.style.width = '12%';
      img.style.height = 'auto';
      img.style.imageRendering = 'auto';
      img.style.filter = 'drop-shadow(0 2px 3px rgba(0,0,0,0.6))';

      this.ui.itemLayerEl.appendChild(img);
    }
  }

  renderDwarves(loc) {
    if (!this.ui.actorLayerEl || !this.bridge) {
      return;
    }

    this.clearActorLayer();

    const dflag = this.bridge.getDwarfFlag();
    if (typeof dflag !== 'number' || dflag < 2) {
      return;
    }

    const used = Object.create(null);

    for (let which = 1; which <= 5; which += 1) {
      const dloc = this.bridge.getDwarfLocation(which);
      if (typeof dloc !== 'number' || dloc <= 0 || dloc !== loc) {
        continue;
      }

      const slotIndex = this.pickActorSlotIndex(1000 + which, loc, used);
      used[slotIndex] = true;
      const slot = ACTOR_SLOTS[slotIndex] || ACTOR_SLOTS[0];

      const img = document.createElement('img');
      img.src = `${this.assetBaseUrl}items/dwarf.webp`;
      img.alt = `Dwarf ${String(which)}`;
      img.style.position = 'absolute';
      img.style.left = `${String(slot.x)}%`;
      img.style.top = `${String(slot.y)}%`;
      img.style.transform = 'translate(-50%, -50%)';
      img.style.width = '16%';
      img.style.height = 'auto';
      img.style.imageRendering = 'auto';
      img.style.filter = 'drop-shadow(0 3px 4px rgba(0,0,0,0.8))';

      this.ui.actorLayerEl.appendChild(img);
    }
  }

  handleSpecialLocations(loc) {
    if (!this.bridge) {
      return null;
    }

    switch (loc) {
      case 8: {
        const grateProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.grate);
        if (grateProp == null) {
          return null;
        }
        return grateProp === 0 ? '8+closed' : '8+open';
      }
      case 9: {
        const grateProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.grate);
        if (grateProp == null) {
          return null;
        }
        return grateProp === 0 ? '9+closed' : '9+open';
      }
      case 17: {
        const fissProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.fissur);
        if (fissProp == null) {
          return null;
        }
        return fissProp === 1 ? '17+crystal+bridge' : null;
      }
      case 23: {
        const stage = this.bridge.getObjectProp(SPECIAL_OBJECTS.plant);
        if (stage == null) {
          return null;
        }
        if (stage >= 4) {
          return '23+huge+plant';
        }
        if (stage >= 2) {
          return '23+large+plant';
        }
        return '23';
      }
      case 25: {
        const stage = this.bridge.getObjectProp(SPECIAL_OBJECTS.plant);
        if (stage == null) {
          return null;
        }
        if (stage >= 4) {
          return '25+huge+plant';
        }
        if (stage >= 2) {
          return '25+large+plant';
        }
        return '25+tiny+plant';
      }
      case 19: {
        const snakeProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.snake);
        if (snakeProp == null) {
          return null;
        }
        return snakeProp === 0 ? '19+snake' : null;
      }
      case 27: {
        const fissProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.fissur);
        if (fissProp == null) {
          return null;
        }
        return fissProp === 1 ? '27+crystal+bridge' : null;
      }
      case 94: {
        const doorProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.door);
        if (doorProp == null) {
          return null;
        }
        return doorProp === 0 ? '94+closed+door' : '94+open+door';
      }
      //case 103: {
      // const clamProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.clam);
      //  if (clamProp == null) {
      //    return null;
      //  }
      //  return clamProp === 0 ? '103+clam+closed' : '103+clam+open';
      //}
      case 117: {
        const trollProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.troll);
        if (trollProp == null) {
          return null;
        }
        if (trollProp === 2) {
          return '117+broken+bridge';
        }
        if (trollProp === 1) {
          return '117+bridge';
        }
        if (trollProp === 0) {
          return '117+bridge+troll';
        }
        return null;
      }
      case 120:
        return '120+dead+dragon';
      case 121:
        return '121+live+dragon';
      case 122: {
        const trollProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.troll);
        if (trollProp == null) {
          return null;
        }
        if (trollProp === 2) {
          return '122+broken+bridge';
        }
        if (trollProp === 1) {
          return '122+bridge';
        }
        if (trollProp === 0) {
          return '122+bridge+troll';
        }
        return null;
      }
      case 130: {
        const bearProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.bear);
        if (bearProp == null || !this.bridge.hasPlaceAndFixedAccess()) {
          return null;
        }

        const bp = this.bridge.getObjectPlace(SPECIAL_OBJECTS.bear);
        const bf = this.bridge.getObjectFixed(SPECIAL_OBJECTS.bear);
        const cp = this.bridge.getObjectPlace(SPECIAL_OBJECTS.chain);
        const cf = this.bridge.getObjectFixed(SPECIAL_OBJECTS.chain);

        const bearHere = bp === 130 || bf === 130;
        const chainHere = cp === 130 || cf === 130;

        if (!bearHere && !chainHere) {
          return null;
        }

        if (bearHere) {
          if (bearProp === 0) {
            return '130+fierce+bear';
          }
          if (chainHere) {
            return '130+tame+bear';
          }
          return '130+bear+nochain';
        }

        return null;
      }

      // The maze of twisty little passages
      case 42:
      case 43:
      case 44:
      case 45:
      case 49:
      case 50:
      case 51:
      case 52:
      case 53:
      case 55:
      case 80:
      case 83:
      case 84:
      case 87:
      case 107:
      case 131:
      case 138:
      case 139:
        return '42';

      // These rooms are all dead ends, mapping to the same image
      case 46:
      case 47:
      case 48:
      case 54:
      case 56:
      case 58:
      case 63:
      case 81:
      case 82:
      case 85:
      case 86:
      case 114:
        return '46';

      // Except 140, which hosts the vending machine and has its own special variant
      case 140:
        return '140';

      default:
        return null;
    }
  }

  refreshRoomVisuals() {
    if (!this.state.moduleRef || !this.bridge) {
      this.clearActorLayer();
      return;
    }

    const loc = this.bridge.getCurrentLoc();
    const isDead = this.bridge.getPlayerDead();
    if (isDead === 1) {
      this.setRoomImageByKey('you+dead');
      this.clearItemLayer();
      this.clearActorLayer();
      return;
    }
    if (typeof loc !== 'number' || loc <= 0) {
      this.clearActorLayer();
      return;
    }

    this.state.currentLocation = loc;

    const lampProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.lamp);
    const locCond = this.bridge.getLocationCond(loc);
    const lampPlace = this.bridge.getObjectPlace(SPECIAL_OBJECTS.lamp);
    const roomHasLight = typeof locCond === 'number' && locCond !== -9999 ? (locCond & 1) !== 0 : true;
    const lampIsLightingHere =
      lampProp === 1 && (lampPlace === loc || lampPlace === -1);

    if (!roomHasLight && !lampIsLightingHere) {
      this.setRoomImageByKey('dark');
      this.renderRoomItems(loc);
      this.renderDwarves(loc);
      return;
    }

    const overrideKey = this.handleSpecialLocations(loc);
    if (overrideKey) {
      this.setRoomImageByKey(overrideKey);
    } else {
      this.updateRoomImage(loc);
    }
    this.renderRoomItems(loc);
    this.renderDwarves(loc);
  }
}
