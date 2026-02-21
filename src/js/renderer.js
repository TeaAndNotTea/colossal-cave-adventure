import { ITEM_DEFS, ITEM_SLOTS } from './config.js';

const SPECIAL_OBJECTS = {
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

  renderRoomItems(loc) {
    if (!this.ui.itemLayerEl || !this.bridge || !this.bridge.hasObjectLocationAccess()) {
      return;
    }

    this.clearItemLayer();
    const used = Object.create(null);

    for (const def of ITEM_DEFS) {
      const obj = def.obj | 0;
      if (obj <= 0) {
        continue;
      }

      const place = this.bridge.getObjectPlace(obj);
      const fixed = this.bridge.getObjectFixed(obj);
      const here = place === loc || fixed === loc;
      if (!here) {
        continue;
      }

      const slotIndex = this.pickSlotIndex(def, loc, used);
      used[slotIndex] = true;

      const slot = ITEM_SLOTS[slotIndex] || ITEM_SLOTS[0];
      const img = document.createElement('img');
      img.src = `${this.assetBaseUrl}items/${def.sprite}`;
      img.alt = def.sprite.replace(/\.webp$/i, '');
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
      case 103: {
        const clamProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.clam);
        if (clamProp == null) {
          return null;
        }
        return clamProp === 0 ? '103+clam+closed' : '103+clam+open';
      }
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
      default:
        return null;
    }
  }

  refreshRoomVisuals() {
    if (!this.state.moduleRef || !this.bridge) {
      return;
    }

    const loc = this.bridge.getCurrentLoc();
    if (typeof loc !== 'number' || loc <= 0) {
      return;
    }

    this.state.currentLocation = loc;
    const overrideKey = this.handleSpecialLocations(loc);
    if (overrideKey) {
      this.setRoomImageByKey(overrideKey);
    } else {
      this.updateRoomImage(loc);
    }
    this.renderRoomItems(loc);
  }
}
