import { ACTOR_SLOTS, ITEM_DEFS, ITEM_SLOTS, SPECIAL_OBJECTS } from './config.js';

// The renderer is the thin visual layer between the DOM and the WASM bridge.
// It does not own gameplay state; instead it reads current object/location
// values from the bridge, chooses the correct room image variant, and rebuilds
// overlay layers for items and actors whenever the scene changes.
export class AdventRenderer {
  // Captures the DOM handles, shared mutable UI state, and asset base path that
  // every later render step depends on. The pending-room fields are bookkeeping
  // for asynchronous image loading so a stale room load cannot repaint overlays
  // after the player has already moved somewhere else.
  constructor({ ui, assetBaseUrl, state }) {
    this.ui = ui;
    this.assetBaseUrl = assetBaseUrl;
    this.state = state;
    this.bridge = null;
    this.pendingRoomLayersRenderSeq = 0;
    this.pendingRoomLayersImageEl = null;
    this.pendingRoomLayersLoadHandler = null;
    this.pendingRoomLayersErrorHandler = null;
  }

  // Installs the bridge object that exposes live engine state from the WASM
  // side. The renderer is deliberately inert until this is provided so tests or
  // early boot code can construct it before the module is fully ready.
  setBridge(bridge) {
    this.bridge = bridge;
  }

  // Switches the base room image if the requested asset key differs from the
  // one already displayed. Returning a boolean lets callers distinguish between
  // "room art actually changed" and "same art, only overlays may need refresh".
  setRoomImageByKey(key) {
    if (!this.ui.imageEl || !key || key === this.state.lastImageKey) {
      return false;
    }
    this.state.lastImageKey = key;
    this.ui.imageEl.src = `${this.assetBaseUrl}rooms/${key}.webp`;
    this.ui.imageEl.alt = `Location ${String(key)}`;
    return true;
  }

  // Convenience wrapper for the common case where the room art is keyed exactly
  // by numeric location id. This keeps callers from repeating the number-to-
  // string conversion and centralizes the validity check for room locations.
  updateRoomImage(loc) {
    if (typeof loc !== 'number' || loc <= 0) {
      return;
    }
    this.setRoomImageByKey(String(loc));
  }

  // Removes any outstanding image load/error listeners registered by a prior
  // `updateRoomScene` call. This prevents duplicate overlay renders and avoids
  // handlers firing after the renderer has moved on to a newer room image.
  clearPendingRoomLayersRender() {
    const imgEl = this.pendingRoomLayersImageEl;
    if (!imgEl) {
      return;
    }

    if (this.pendingRoomLayersLoadHandler) {
      imgEl.removeEventListener('load', this.pendingRoomLayersLoadHandler);
    }
    if (this.pendingRoomLayersErrorHandler) {
      imgEl.removeEventListener('error', this.pendingRoomLayersErrorHandler);
    }

    this.pendingRoomLayersImageEl = null;
    this.pendingRoomLayersLoadHandler = null;
    this.pendingRoomLayersErrorHandler = null;
  }

  // Rebuilds both dynamic overlay layers for the current room. Keeping this in
  // one helper makes it easy to trigger item and actor rendering together once
  // the background image is known to be in a stable state.
  renderOverlayLayers(loc) {
    this.renderRoomItems(loc);
    this.renderDwarves(loc);
  }

  // Room backgrounds load asynchronously. When the room image changes we defer
  // overlay rendering until the new image has either loaded or failed, so item
  // and actor sprites stay visually in sync with the backdrop the player sees.
  updateRoomScene(loc, imageKey) {
    if (typeof loc !== 'number' || loc <= 0) {
      return;
    }

    const changed = this.setRoomImageByKey(String(imageKey));

    if (!changed) {
      if (
        this.pendingRoomLayersImageEl === this.ui.imageEl &&
        this.ui.imageEl &&
        !this.ui.imageEl.complete
      ) {
        return;
      }
      this.clearPendingRoomLayersRender();
      this.renderOverlayLayers(loc);
      return;
    }

    this.clearItemLayer();
    this.clearActorLayer();

    const imgEl = this.ui.imageEl;
    if (!imgEl) {
      this.renderOverlayLayers(loc);
      return;
    }

    this.clearPendingRoomLayersRender();

    const seq = (this.pendingRoomLayersRenderSeq += 1);
    const finish = () => {
      if (seq !== this.pendingRoomLayersRenderSeq) {
        return;
      }
      this.clearPendingRoomLayersRender();
      if (this.state.currentLocation !== loc) {
        return;
      }
      this.renderOverlayLayers(loc);
    };

    if (imgEl.complete) {
      finish();
      return;
    }

    this.pendingRoomLayersImageEl = imgEl;
    this.pendingRoomLayersLoadHandler = () => {
      finish();
    };
    this.pendingRoomLayersErrorHandler = () => {
      finish();
    };

    imgEl.addEventListener('load', this.pendingRoomLayersLoadHandler);
    imgEl.addEventListener('error', this.pendingRoomLayersErrorHandler);

    if (imgEl.complete) {
      finish();
    }
  }

  // Produces a deterministic pseudo-random integer from an object/seed and
  // location. The renderer uses this to spread sprites across available slots
  // without introducing frame-to-frame jitter for the same room state.
  hashSlot(obj, loc) {
    let x = ((obj | 0) * 1103515245 + (loc | 0) * 12345) | 0;
    x ^= x >>> 16;
    if (x < 0) {
      x = -x;
    }
    return x;
  }

  // Chooses an inventory-layer slot for a rendered object. It first honors a
  // configured starting slot when the object is in its canonical starting room,
  // then falls back to a deterministic hashed placement with linear probing so
  // multiple items can coexist without overlapping the same slot.
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

  // Clears all currently rendered item sprites from the item overlay layer.
  // This class always rebuilds the layer from scratch because item visibility
  // depends on many pieces of bridge state and diffing would add complexity for
  // little practical benefit at this scale.
  clearItemLayer() {
    if (!this.ui.itemLayerEl) {
      return;
    }
    while (this.ui.itemLayerEl.firstChild) {
      this.ui.itemLayerEl.removeChild(this.ui.itemLayerEl.firstChild);
    }
  }

  // Clears all actor overlays such as dwarves and the bear-peeping flourish.
  // Items and actors live on separate DOM layers so each can be refreshed
  // independently when the surrounding control flow needs that separation.
  clearActorLayer() {
    if (!this.ui.actorLayerEl) {
      return;
    }
    while (this.ui.actorLayerEl.firstChild) {
      this.ui.actorLayerEl.removeChild(this.ui.actorLayerEl.firstChild);
    }
  }

  // Chooses a deterministic slot for actor-style overlays. This mirrors item
  // placement logic but uses the smaller fixed actor staging row instead of the
  // inventory grid because actors are positioned as scene characters, not props.
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

  // Reconstructs the room's item overlay from current game state. This method
  // combines static art metadata from `ITEM_DEFS` with live object placement
  // and prop values from the bridge so it can decide which objects are present,
  // which composite sprites should replace them, and where each visible sprite
  // should sit in the lower overlay area.
  renderRoomItems(loc) {
    if (!this.ui.itemLayerEl || !this.bridge || !this.bridge.hasObjectLocationAccess()) {
      return;
    }

    this.clearItemLayer();
    const used = Object.create(null);
    const rowBuckets = [[], [], []];
    // A few objects need composite or state-sensitive artwork that cannot be
    // expressed by the base item table alone, so we gather the bridge state up
    // front and decide those variants before walking ITEM_DEFS.
    const pillowDef = ITEM_DEFS.find((def) => (def.obj | 0) === 10);
    const pillowCompositeSlotIndex =
      pillowDef && typeof pillowDef.startSlot === 'number' ? (pillowDef.startSlot | 0) : 16;
    const cagePlace = this.bridge.getObjectPlace(4);
    const cageFixed = this.bridge.getObjectFixed(4);
    const rodPlace = this.bridge.getObjectPlace(5);
    const pillowPlace = this.bridge.getObjectPlace(10);
    const pillowFixed = this.bridge.getObjectFixed(10);
    const oysterPlace = this.bridge.getObjectPlace(SPECIAL_OBJECTS.oyster);
    const oysterFixed = this.bridge.getObjectFixed(SPECIAL_OBJECTS.oyster);
    const birdPlace = this.bridge.getObjectPlace(8);
    const birdFixed = this.bridge.getObjectFixed(8);
    const birdProp = this.bridge.getObjectProp(8);
    const bottleProp = this.bridge.getObjectProp(20);
    const dragonProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.dragon);
    const bearProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.bear);
    const chainProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.chain);
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
      used[pillowCompositeSlotIndex] = true;
    }

    // ITEM_DEFS defines which concrete objects have art. We still decide at
    // render time whether each object is present here, whether it should be
    // suppressed in favor of a composite sprite, and which fallback slot it
    // should occupy if its preferred slot is already taken.
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
      if (obj === 62 && dragonProp === 0 && (loc === 119 || loc === 121)) {
        continue;
      }
      if (obj === SPECIAL_OBJECTS.chain && loc === 130) {
        const showLooseChainInBearRoom = bearProp === 2 && chainProp === 0;
        if (!showLooseChainInBearRoom) {
          continue;
        }
      }

      let place = this.bridge.getObjectPlace(obj);
      let fixed = this.bridge.getObjectFixed(obj);
      let here = place === loc || fixed === loc;
      if (obj === SPECIAL_OBJECTS.clam && !here) {
        const oysterHere = oysterPlace === loc || oysterFixed === loc;
        if (oysterHere) {
          place = oysterPlace;
          fixed = oysterFixed;
          here = true;
        }
      }
      if (!here) {
        continue;
      }

      const slotIndex =
        renderPillowVaseFromVase && obj === 58
          ? pillowCompositeSlotIndex
          : this.pickSlotIndex(def, loc, used);
      used[slotIndex] = true;

      const slot = ITEM_SLOTS[slotIndex] || ITEM_SLOTS[0];
      const slotRow =
        typeof slot.row === 'number' && slot.row >= 0 && slot.row <= 2
          ? slot.row | 0
          : Math.max(0, Math.min(2, Math.floor(slotIndex / 8)));
      const slotScale = typeof slot.scale === 'number' && slot.scale > 0 ? slot.scale : 1;
      const itemScalePct =
        typeof def.scalePct === 'number' && def.scalePct > 0 ? def.scalePct : 100;
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
      img.style.transform = 'translate(-50%, -100%)';
      img.style.width = `${String((12 * slotScale * (itemScalePct / 100)).toFixed(2))}%`;
      img.style.height = 'auto';
      img.style.imageRendering = 'auto';
      img.style.filter = 'drop-shadow(0 2px 3px rgba(0,0,0,0.6))';
      img.style.zIndex = String(slotRow + 1);

      rowBuckets[slotRow].push(img);
    }

    // Append back-to-front by row so lower rows naturally paint over upper rows
    // without needing a separate sort pass for every individual sprite.
    for (const bucket of rowBuckets) {
      for (const img of bucket) {
        this.ui.itemLayerEl.appendChild(img);
      }
    }
  }

  // Rebuilds actor overlays for hostile NPCs in the current room. Right now
  // that primarily means dwarves, which are rendered separately from items so
  // their placement and styling can follow scene-character rules instead of the
  // inventory/item grid logic.
  renderDwarves(loc) {
    if (!this.ui.actorLayerEl || !this.bridge) {
      return;
    }

    this.clearActorLayer();

    const dflag = this.bridge.getDwarfFlag();
    const used = Object.create(null);

    // The original game tracks up to five dwarves independently. We render only
    // the ones currently in this room, placing them into deterministic actor
    // slots so they do not jump around between refreshes.
    if (typeof dflag === 'number' && dflag >= 2) {
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
        img.style.width = '24%';
        img.style.height = 'auto';
        img.style.imageRendering = 'auto';
        img.style.filter = 'drop-shadow(0 3px 4px rgba(0,0,0,0.8))';

        this.ui.actorLayerEl.appendChild(img);
      }
    }

    this.renderBearPeepingOverlay();
  }

  // This is a UI-only flourish: once the bear has been carried off-stage we
  // still show a "peeping" overlay in inventory/actor space to match the art
  // direction used elsewhere in the frontend.
  renderBearPeepingOverlay() {
    if (!this.ui.actorLayerEl || !this.bridge || !this.bridge.hasObjectLocationAccess()) {
      return;
    }

    const bearProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.bear);
    const bearPlace = this.bridge.getObjectPlace(SPECIAL_OBJECTS.bear);
    if (bearProp !== 2 || bearPlace !== -1) {
      return;
    }

    const img = document.createElement('img');
    img.src = `${this.assetBaseUrl}items/bear+peeping.webp`;
    img.alt = 'Bear peeping';
    img.style.position = 'absolute';
    img.style.right = '0';
    img.style.bottom = '0';
    img.style.width = '28%';
    img.style.aspectRatio = '741 / 1003';
    img.style.height = 'auto';
    img.style.imageRendering = 'auto';
    img.style.filter = 'drop-shadow(0 4px 6px rgba(0,0,0,0.8))';

    this.ui.actorLayerEl.appendChild(img);
  }

  // Maps a live game location to a room art key when the default `${loc}.webp`
  // rule is not sufficient. This exists because several rooms have stateful
  // backgrounds driven by object props, and some groups of rooms intentionally
  // share a single visual asset even though the underlying location ids differ.
  handleSpecialLocations(loc) {
    if (!this.bridge) {
      return null;
    }

    // Most rooms map directly to `${loc}.webp`. This switch handles the rooms
    // whose backdrop depends on live object state, plus groups of locations that
    // intentionally share a single image asset.
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
      case 117: {
        const trollProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.troll);
        const chasmProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.chasm);
        if (trollProp == null || chasmProp == null) {
          return null;
        }
        if (chasmProp === 1) {
          return '117+broken+bridge';
        }
        let trollHere = null;
        if (this.bridge.hasPlaceAndFixedAccess()) {
          const trollPlace = this.bridge.getObjectPlace(SPECIAL_OBJECTS.troll);
          const trollFixed = this.bridge.getObjectFixed(SPECIAL_OBJECTS.troll);
          if (trollPlace != null && trollFixed != null) {
            trollHere = trollPlace === loc || trollFixed === loc;
          }
        }
        if (trollHere == null) {
          trollHere = trollProp === 0;
        }
        if (trollHere) {
          return '117+bridge+troll';
        }
        return '117+bridge';
      }
      case 119:
      case 121: {
        const dragonProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.dragon);
        if (dragonProp == null) {
          return null;
        }
        return dragonProp === 0 ? '121+live+dragon' : null;
      }
      case 120: {
        const dragonProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.dragon);
        if (dragonProp == null) {
          return null;
        }
        return dragonProp === 2 ? '120+dead+dragon' : null;
      }
      case 122: {
        const trollProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.troll);
        const chasmProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.chasm);
        if (trollProp == null || chasmProp == null) {
          return null;
        }
        if (chasmProp === 1) {
          return '122+broken+bridge';
        }
        let trollHere = null;
        if (this.bridge.hasPlaceAndFixedAccess()) {
          const trollPlace = this.bridge.getObjectPlace(SPECIAL_OBJECTS.troll);
          const trollFixed = this.bridge.getObjectFixed(SPECIAL_OBJECTS.troll);
          if (trollPlace != null && trollFixed != null) {
            trollHere = trollPlace === loc || trollFixed === loc;
          }
        }
        if (trollHere == null) {
          trollHere = trollProp === 0;
        }
        if (trollHere) {
          return '122+bridge+troll';
        }
        return '122+bridge';
      }
      case 130: {
        const bearProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.bear);
        const chainProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.chain);
        if (bearProp == null || chainProp == null || !this.bridge.hasPlaceAndFixedAccess()) {
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
          if (chainProp === 0) {
            return '130+bear+nochain';
          }
          if (chainHere) {
            return '130+tame+bear';
          }
          return null;
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

  // Entry point used by the rest of the frontend to synchronize the visible
  // scene with the current engine state. It handles terminal states like death,
  // validates the current location, applies darkness rules, resolves any room
  // art overrides, and then delegates to `updateRoomScene` for the actual image
  // swap and overlay refresh sequencing.
  refreshRoomVisuals() {
    if (!this.state.moduleRef || !this.bridge) {
      this.clearPendingRoomLayersRender();
      this.clearActorLayer();
      return;
    }

    const loc = this.bridge.getCurrentLoc();
    const isDead = this.bridge.getPlayerDead();
    if (isDead === 1) {
      this.clearPendingRoomLayersRender();
      this.setRoomImageByKey('you+dead');
      this.clearItemLayer();
      this.clearActorLayer();
      return;
    }
    if (typeof loc !== 'number' || loc <= 0) {
      this.clearPendingRoomLayersRender();
      this.clearActorLayer();
      return;
    }

    this.state.currentLocation = loc;

    // Darkness is handled before room-specific overrides because it replaces
    // the entire scene regardless of which nominal room art would otherwise be
    // selected.
    const lampProp = this.bridge.getObjectProp(SPECIAL_OBJECTS.lamp);
    const locCond = this.bridge.getLocationCond(loc);
    const lampPlace = this.bridge.getObjectPlace(SPECIAL_OBJECTS.lamp);
    const roomHasLight = typeof locCond === 'number' && locCond !== -9999 ? (locCond & 1) !== 0 : true;
    const lampIsLightingHere =
      lampProp === 1 && (lampPlace === loc || lampPlace === -1);

    if (!roomHasLight && !lampIsLightingHere) {
      this.updateRoomScene(loc, 'dark');
      return;
    }

    const overrideKey = this.handleSpecialLocations(loc);
    this.updateRoomScene(loc, overrideKey || String(loc));
  }
}
