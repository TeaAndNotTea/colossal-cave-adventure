console.log("[ADVENT] play.js version: adding bear");
(function () {
  // Base URL to /wp-content/plugins/colossal-adventure/assets/
  var assetBaseUrl =
    (window.ColossalAdventureConfig &&
      ColossalAdventureConfig.assetBaseUrl) ||
    '';

  // --- global hook for clean ExitStatus handling -----------------
  if (!window.__adventExitHandlerInstalled) {
    window.__adventExitHandlerInstalled = true;

    window.__adventHandleExit = null;

    window.addEventListener('unhandledrejection', function (ev) {
      var reason = ev.reason || ev.detail || null;

      if (reason && reason.name === 'ExitStatus') {
        ev.preventDefault();

        var status = typeof reason.status === 'number' ? reason.status : -1;
        var msg =
          status === 0
            ? '\n*** Adventure finished. ***'
            : '\n[Adventure exited with status ' + status + ']';

        if (typeof window.__adventHandleExit === 'function') {
          window.__adventHandleExit(status, msg);
        } else {
          console.log('Colossal Adventure exited:', msg);
        }
      }
    });
  }
  // --------------------------------------------------------------------

  function setupUI(root) {
    // Normal document flow; we’ll size the console with JS, not flex.
    root.style.margin = '0';
    root.style.padding = '0';
    root.style.boxSizing = 'border-box';

    root.innerHTML =
      // Image panel
      '<div id="advent-image-panel" style="' +
      '  width:100%;' +
      '  max-width:640px;' +
      '  margin:0 auto 0.5rem auto;' +
      '  border:1px solid #444;' +
      '  background:#000;' +
      '  display:flex;' +
      '  align-items:center;' +
      '  justify-content:center;' +
      '  overflow:hidden;' +
      '">' +
      '  <div id="advent-image-wrap" style="position:relative; width:100%;">' +
      '    <img id="advent-room-image" src="" alt="Adventure location"' +
      '         style="max-width:100%; height:auto; display:block;" />' +
      '    <div id="advent-item-layer" style="position:absolute; left:0; top:0; right:0; bottom:0; pointer-events:none;"></div>' +
      '  </div>' +
      '</div>' +

      // Options row
      '<div id="advent-options" style="' +
      '  max-width:640px;' +
      '  margin:0 auto 0.5rem auto;' +
      '  padding:0.25rem 0.5rem;' +
      '  border:1px solid #444;' +
      '  background:#111;' +
      '  color:#eee;' +
      '  font-family:monospace;' +
      '  font-size:13px;' +
      '  display:flex;' +
      '  align-items:center;' +
      '  gap:0.75rem;' +
      '">' +
      '  <label style="display:flex; align-items:center; gap:0.4rem; cursor:pointer; user-select:none;">' +
      '    <input id="advent-disable-dwarves" type="checkbox" />' +
      '    Disable dwarves' +
      '  </label>' +
      '  <span style="opacity:0.8;">(default off for authenticity)</span>' +
      '</div>' +

      // Console (we’ll override max-height/height in JS)
      '<div id="advent-console" style="' +
      '  border:1px solid #444;' +
      '  padding:0.5rem;' +
      '  background:#111;' +
      '  color:#eee;' +
      '  font-family:monospace;' +
      '  font-size:14px;' +
      '  max-height:400px;' + // default; JS will replace
      '  overflow-y:auto;' +
      '  white-space:pre-wrap;' +
      '"></div>' +

      // Input form
      '<form id="advent-form" style="margin-top:0.5rem; display:flex; gap:0.5rem;">' +
      '  <input id="advent-input" type="text" autocomplete="off" ' +
      '         style="flex:1; font-family:monospace; font-size:14px;' +
      '                background:#000; color:#eee; border:1px solid #666; padding:0.25rem;" />' +
      '  <button type="submit">Send</button>' +
      '</form>' +

      // Restart button
      '<button id="advent-restart" type="button" ' +
      '        style="margin-top:0.5rem; display:none; padding:0.3rem 0.7rem;' +
      '               background:#222; color:#eee; border:1px solid #666; cursor:pointer;">' +
      '  Restart adventure' +
      '</button>';

    return {
      imagePanelEl: root.querySelector('#advent-image-panel'),
      imageEl: root.querySelector('#advent-room-image'),
      itemLayerEl: root.querySelector('#advent-item-layer'),
      consoleEl: root.querySelector('#advent-console'),
      formEl: root.querySelector('#advent-form'),
      inputEl: root.querySelector('#advent-input'),
      restartBtn: root.querySelector('#advent-restart'),
      disableDwarvesEl: root.querySelector('#advent-disable-dwarves'),
      optionsEl: root.querySelector('#advent-options'),
    };
  }

  function runGame(root) {
    var ui = setupUI(root);

    // Scroll so the image panel sits at the top of the viewport
    if (root && typeof root.scrollIntoView === 'function') {
      root.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }

    var imagePanelEl = ui.imagePanelEl;
    var imageEl = ui.imageEl;
    var itemLayerEl = ui.itemLayerEl;

    // Show title screen by default (before the game reaches location 1)
    if (imageEl) {
      imageEl.src = assetBaseUrl + 'title.webp';
      imageEl.alt = 'Colossal Cave Adventure';
    }

    var consoleEl = ui.consoleEl;
    var formEl = ui.formEl;
    var inputEl = ui.inputEl;
    var restartBtn = ui.restartBtn;
    var disableDwarvesEl = ui.disableDwarvesEl;
    var optionsEl = ui.optionsEl;

    // Default: dwarves enabled => checkbox unchecked
    // (Optional persistence; harmless if blocked by browser privacy mode)
    try {
      var saved = window.localStorage.getItem('advent_disable_dwarves');
      if (saved === '1') disableDwarvesEl.checked = true;
    } catch (e) {}

    var gameActive = true;
    var pendingResolve = null;

    // Set once the WASM module is ready. Used by the print handler so we can
    // refresh the image when object state changes without requiring a location
    // change.
    var ModuleRef = null;

    // Cached WASM accessors
    var getLocFn = null;
    var setLocFn = null;
    var spawnDwarfFn = null;
    var spawnAllDwarvesFn = null;
    var spawnPirateFn = null;
    var getObjPropFn = null;
    var getObjPlaceFn = null;
    var getObjFixedFn = null;

    // object numbers
    var objGrate = 3;
    var objFissur = 12;
    var objPlant = 24;
    var objSnake = 11;
    var objDragon = 31;
    var objDoor  = 9;
    var objClam  = 14;
    var objTroll = 33;
    var objBear  = 35;
    var objChain = 64;

    // Track which image is currently displayed to avoid pointless reloads.
    var lastImageKey = null;

    function appendLine(text) {
      if (text == null) text = '';
      consoleEl.textContent += String(text) + '\n';
      consoleEl.scrollTop = consoleEl.scrollHeight;
    }

    // --- Dynamic layout: make console fit remaining viewport height ----
    function applyLayout() {
      if (!consoleEl || !imagePanelEl || !formEl) return;

      var vh = window.innerHeight || document.documentElement.clientHeight || 0;

      var imageH = imagePanelEl.offsetHeight || 0;
      var optionsH = optionsEl ? optionsEl.offsetHeight || 0 : 0;
      var formH = formEl.offsetHeight || 0;
      var restartH =
        restartBtn && restartBtn.offsetParent !== null
          ? restartBtn.offsetHeight || 0
          : 0;

      // Some extra padding/margins between elements
      var padding = 40;

      var available = vh - imageH - optionsH - formH - restartH - padding;
      if (available < 100) {
        available = 100;
      }

      consoleEl.style.height = available + 'px';
      consoleEl.style.maxHeight = available + 'px';
    }

    window.addEventListener('resize', applyLayout);
    if (imageEl) imageEl.addEventListener('load', applyLayout);
    window.requestAnimationFrame(applyLayout);

    // --- Room image handling -------------------------------------------
    function setRoomImageByKey(key) {
      if (!imageEl) return;
      if (!key) return;
      if (key === lastImageKey) return;
      lastImageKey = key;
      var url = assetBaseUrl + 'rooms/' + key + '.webp';
      imageEl.src = url;
      imageEl.alt = 'Location ' + String(key);
    }

    function updateRoomImage(loc) {
      if (typeof loc !== 'number' || loc <= 0) return;
      setRoomImageByKey(String(loc));
    }

    function getCurrentLoc() {
      if (!getLocFn) return 0;
      try {
        return getLocFn();
      } catch (e) {
        console.warn('get_current_location() failed:', e);
        return 0;
      }
    }

    function tryWarpToLocation(newLoc) {
      if (!setLocFn) return { ok: false, code: 'no-export' };
      var n = (newLoc | 0);
      var rc = -9999;
      try {
        rc = setLocFn(n) | 0;
      } catch (e) {
        console.warn('set_current_location() failed:', e);
        return { ok: false, code: 'exception', err: e };
      }
      if (rc === -1) return { ok: false, code: 'not-ready' };
      if (rc === -2) return { ok: false, code: 'out-of-range' };
      if (rc <= 0) return { ok: false, code: 'failed', rc: rc };
      refreshRoomVisuals();
      return { ok: true, loc: rc | 0 };
    }

    function trySpawn(kind, which) {
      var k = String(kind || '').toLowerCase();
      try {
        if (k === 'pirate') {
          if (!spawnPirateFn) return { ok: false, code: 'no-export' };
          var prc = spawnPirateFn() | 0;
          if (prc < 0) return { ok: false, code: 'failed', rc: prc };
          return { ok: true, who: 'pirate' };
        }
        if (k === 'dwarves') {
          if (!spawnAllDwarvesFn) return { ok: false, code: 'no-export' };
          var arc = spawnAllDwarvesFn() | 0;
          if (arc < 0) return { ok: false, code: 'failed', rc: arc };
          return { ok: true, who: 'dwarves' };
        }
        if (k === 'dwarf') {
          if (!spawnDwarfFn) return { ok: false, code: 'no-export' };
          var w = (which | 0);
          if (w <= 0) w = 1;
          var drc = spawnDwarfFn(w) | 0;
          if (drc < 0) return { ok: false, code: 'failed', rc: drc };
          return { ok: true, who: 'dwarf', which: w };
        }
      } catch (e) {
        console.warn('spawn failed:', e);
        return { ok: false, code: 'exception', err: e };
      }
      return { ok: false, code: 'unknown' };
    }

function getObjectProp(objNum) {
      if (!getObjPropFn) return null;
      try {
        return getObjPropFn(objNum | 0);
      } catch (e) {
        console.warn('get_object_prop() failed:', e);
        return null;
      }
    }

    function getObjectPlace(objNum) {
      if (!getObjPlaceFn) return null;
      try {
        return getObjPlaceFn(objNum | 0);
      } catch (e) {
        console.warn('get_object_place() failed:', e);
        return null;
      }
    }

    function getObjectFixed(objNum) {
      if (!getObjFixedFn) return null;
      try {
        return getObjFixedFn(objNum | 0);
      } catch (e) {
        console.warn('get_object_fixed() failed:', e);
        return null;
      }
    }

    
    // --- Item overlay rendering ---------------------------------------
    // Define the "slots" along the bottom of the image where item sprites can appear.
    // Coordinates are percentages within the image (0..100).
    var ITEM_SLOTS = [
      { x: 10, y: 84 }, { x: 25, y: 84 }, { x: 40, y: 84 }, { x: 55, y: 84 }, { x: 70, y: 84 }
     ];

    // Define collectible items you want rendered, with placeholder sprite filenames.
    // obj: Adventure object number (1..100 in this build)
    // sprite: filename under assets/items/
    // startLoc: the room location where this item begins in the original game (optional)
    // startSlot: preferred slot index when the item is in its startLoc (optional)
    //
    // Item (initial) locations are defined in advent.dat table 7
    //
    var ITEM_DEFS = [
      { obj: 1,  sprite: 'keys.webp',         startLoc: 3, startSlot: 0 },
      { obj: 2,  sprite: 'lamp.webp',         startLoc: 3, startSlot: 1 },
      // 3 Grate, coded in the room image
      { obj: 4,  sprite: 'cage.webp',         startLoc: 10, startSlot: 2 }, // ** need a bird+cage variant **
      { obj: 5,  sprite: 'rod.webp',          startLoc: 11, startSlot: 3 },
      // 6 ROD 2
      // 7 stairs
      { obj: 8,  sprite: 'bird.webp',         startLoc: 13, startSlot: 4 }, // ** need a disturbed bird variant **
      // 9 (Iron) door
      { obj: 10, sprite: 'pillow.webp',       startLoc: 96, startSlot: 0 }, // ** need a vase+pillow variant **
      // 11 Snake
      // 12 Fissue
      // 13 (stone) tablet
      // 14 Clam
      // 15 Oyster
      { obj: 16, sprite: 'magazine.webp',     startLoc: 106, startSlot: 1 },
      // 17 ** Dwarf **
      // 18 ** Dwarves **
      { obj: 19, sprite: 'food.webp',         startLoc: 3, startSlot: 2 },
      { obj: 20, sprite: 'bottle+full.webp',  startLoc: 3, startSlot: 3 }, // ** add oil variant **
      // 21 Water
      // 22 Oil    see 20
      // 23 Mirror
      // 24 Plant
      // 25 Plant 2
      // 26 Stalactite
      // 27 Figure
      { obj: 28, sprite: 'axe.webp',  startLoc: 0, startSlot: 4 },
      // 29 Drawings
      // 30 ** pirate **
      // 31 Dragon
      // 32 Chasm
      // 33 Troll
      // 34 Troll 2
      // 35 Bear
      // 36 Message
      // 37 Volcano
      // 38 (Vending) machine
      { obj: 39, sprite: 'batteries.webp',  startLoc: 0, startSlot: 0 },
      // 40 (Persian) carpet
      //
      // Items 50 -> are treasure items.
      //
      { obj: 50, sprite: 'gold.webp',  startLoc: 18, startSlot: 1},
      { obj: 51, sprite: 'diamonds.webp',  startLoc: 27, startSlot: 2 },
      { obj: 52, sprite: 'silver.webp',  startLoc: 28, startSlot: 3 },
      { obj: 53, sprite: 'jewels.webp',  startLoc: 29, startSlot: 4 },
      { obj: 54, sprite: 'coins.webp',  startLoc: 30, startSlot: 0 },
      { obj: 55, sprite: 'chest.webp',  startLoc: 0, startSlot: 1 },
      { obj: 56, sprite: 'eggs.webp',  startLoc: 92, startSlot: 2 },
      { obj: 57, sprite: 'trident.webp',  startLoc: 95, startSlot: 3 },
      { obj: 58, sprite: 'vase.webp',  startLoc: 97, startSlot: 4 },
      { obj: 59, sprite: 'emerald.webp',  startLoc: 100, startSlot: 0 },
      { obj: 60, sprite: 'platinum.webp',  startLoc: 101, startSlot: 1 },
      { obj: 61, sprite: 'pearl.webp',  startLoc: 0, startSlot: 2 },
      { obj: 62, sprite: 'rug.webp',  startLoc: 119, startSlot: 3 },
      { obj: 63, sprite: 'spices.webp',  startLoc: 127, startSlot: 4 },
      { obj: 64, sprite: 'chain.webp',  startLoc: 130, startSlot: 5 }
    ];

    // Stable hash -> pseudo-random slot selection that is deterministic for (obj, loc).
    function hashSlot(obj, loc) {
      // A tiny integer hash; good enough for UI placement.
      var x = ((obj | 0) * 1103515245 + (loc | 0) * 12345) | 0;
      x ^= (x >>> 16);
      if (x < 0) x = -x;
      return x;
    }

    function pickSlotIndex(def, loc, alreadyUsed) {
      var preferred =
        (def && def.startLoc > 0 && def.startLoc === loc && typeof def.startSlot === 'number')
          ? (def.startSlot | 0)
          : -1;

      var n = ITEM_SLOTS.length;
      if (n <= 0) return 0;

      // If preferred is available, use it.
      if (preferred >= 0) {
        var p = ((preferred % n) + n) % n;
        if (!alreadyUsed[p]) return p;
      }

      // Otherwise pick a deterministic "random" slot, then linearly probe for a free slot.
      var h = hashSlot(def.obj | 0, loc | 0) % n;
      for (var i = 0; i < n; i++) {
        var idx = (h + i) % n;
        if (!alreadyUsed[idx]) return idx;
      }

      // If everything is taken, just stack on the hashed position.
      return h;
    }

    function clearItemLayer() {
      if (!itemLayerEl) return;
      while (itemLayerEl.firstChild) itemLayerEl.removeChild(itemLayerEl.firstChild);
    }

    function renderRoomItems(loc) {
      if (!itemLayerEl) return;
      if (!getObjPlaceFn) return;

      clearItemLayer();

      var used = Object.create(null);

      for (var i = 0; i < ITEM_DEFS.length; i++) {
        var def = ITEM_DEFS[i];
        var obj = def.obj | 0;
        if (obj <= 0) continue;

        var place = getObjectPlace(obj);
        var fixed = getObjectFixed(obj);

        // Show if the object is currently in this location (either movable place or fixed anchor).
        var here = (place === loc) || (fixed === loc);
        if (!here) continue;

        var slotIndex = pickSlotIndex(def, loc, used);
        used[slotIndex] = true;

        var slot = ITEM_SLOTS[slotIndex] || ITEM_SLOTS[0];
        var img = document.createElement('img');
        img.src = assetBaseUrl + 'items/' + def.sprite;
        img.alt = def.sprite.replace(/\.webp$/i, '');
        img.style.position = 'absolute';
        img.style.left = String(slot.x) + '%';
        img.style.top = String(slot.y) + '%';
        img.style.transform = 'translate(-50%, -50%)';
        img.style.width = '12%';
        img.style.height = 'auto';
        img.style.imageRendering = 'auto';
        img.style.filter = 'drop-shadow(0 2px 3px rgba(0,0,0,0.6))';

        itemLayerEl.appendChild(img);
      }
    }

// --- Special-location hook --------------------------------------
    // Returns an image key override (string) or null/undefined to use the
    // default image (rooms/<loc>.webp).
    function handleSpecialLocations(loc) {
      switch (loc) {
        case 8: {
          // Outside Grate
          var grateProp = getObjectProp(objGrate);
          if (grateProp == null) return null;
          return grateProp === 0 ? '8+closed' : '8+open';
        }
        case 9: {
          // Below Grate
          var grateProp = getObjectProp(objGrate);
          if (grateProp == null) return null;
          return grateProp === 0 ? '9+closed' : '9+open';
        }
        case 17: {
          // Fissure / crystal bridge
          var fissProp = getObjectProp(objFissur);
          if (fissProp == null) return null;
          // 0 = no bridge, 1 = crystal bridge present
          return fissProp === 1 ? '17+crystal+bridge' : null;
        }
        case 25: {
          // Bottom of pit / beanstalk plant (primary location)
          var stage = getObjectProp(objPlant);
          if (stage == null) return null;
          // plant stages: 0 (small), 2 (grown), 4 (huge/climbable)
          if (stage >= 4) return '25+huge+plant';
          if (stage >= 2) return '25+large+plant';
          return '25+tiny+plant';
        }
        case 19: {
          // Snake room
          var snakeProp = getObjectProp(objSnake);
          if (snakeProp == null) return null;
          // 0 = snake present, 1 = removed
          return snakeProp === 0 ? '19+snake' : null;
        }
        case 27: { // location is a other side of the fissure, render properties are the same as loc 17
          // Fissure / crystal bridge
          var fissProp = getObjectProp(objFissur);
          if (fissProp == null) return null;
          // 0 = no bridge, 1 = crystal bridge present
          return fissProp === 1 ? '27+crystal+bridge' : null;
        }
        case 94: {
          var doorProp = getObjectProp(objDoor);
          if (doorProp == null) return null;
          return doorProp === 0 ? '94+closed+door' : '94+open+door';
        }
        case 103: {
          // Huge clam room
          // The clam always snaps shut, and when revisited mysteriously turns into an oyster, which can be opened, yields nothing and
          // also snaps shut immediately. The clam is never actually stuck in the open position, so this image swap is currently not
          // triggered. Left as a placeholder for potential transitional image additions in the future.
          //
          if (!objClam) return null;
          var clamProp = getObjectProp(objClam);
          if (clamProp == null) return null;
          // 0 = closed clam, 1 = opened
          return clamProp === 0 ? '103+clam+closed' : '103+clam+open';
        }
        case 117: {
          //
          // Troll at bridge near side
          //
          // The wooden bridge has visible ends in two rooms: 117 and 122
          // so the troll can also be visible in both rooms unless he permenantly is removed.
          // Additionally, the bridge could be in the broken state, if you were foolish enough to try to cross
          // whilst being followed by the tame bear
          //
          var trollProp = getObjectProp(objTroll);
          if (trollProp == null) return null;
          if (trollProp == 2) return '117+broken+bridge';
          if (trollProp == 1) return '117+bridge';
          if (trollProp == 0) return '117+bridge+troll';
        }
        //
        // For this case, the initial room selected is 121, with the rug effectively part of the scene, until the dragon is killed
        // whereupon the location shifts to 120, which shows the dragon as dead, the rug is now a collectable object
        // We don't need to lookup an object property for the dragon as this information is coded in the room location
        // but the code here remains in case we want to animate or support transitional states in a future version.
        //
        case 120: {
            return '120+dead+dragon';
        }
        case 121: {
            return '121+live+dragon';
        }
        case 122: {
          // Troll at bridge far side
          var trollProp = getObjectProp(objTroll);
          if (trollProp == null) return null;
            if (trollProp == 2) return '122+broken+bridge';
            if (trollProp == 1) return '122+bridge';
            if (trollProp == 0) return '122+bridge+troll';
          }
        case 130: {
          // Bear room
          var bearProp = getObjectProp(objBear);
          if (bearProp == null) return null;

          // Determine whether bear/chain are actually in this room using runtime place/fixed arrays.
          // (These are the authoritative "is it here now?" values.)
          var bearHere = false;
          var chainHere = false;

          if (getObjPlaceFn && getObjFixedFn) {
            var bp = getObjectPlace(objBear);
            var bf = getObjectFixed(objBear);
            var cp = getObjectPlace(objChain);
            var cf = getObjectFixed(objChain);

            bearHere = (bp === 130) || (bf === 130);
            chainHere = (cp === 130) || (cf === 130);
          } else {
            // Without place/fixed accessors we can't reliably tell if the bear/chain are present.
            return null;
          }

          // If neither bear nor chain are here, use the base room image: 130.webp
          if (!bearHere && !chainHere) return null;

          // If the bear is here, choose the right variant based on bear prop and whether the chain is still here.
          if (bearHere) {
            // 0 = fierce (unfed)
            if (bearProp === 0) return '130+fierce+bear';

            // Bear is tame (fed). If chain still here, he is still chained.
            if (chainHere) return '130+tame+bear';

            // Bear present but chain gone
            return '130+bear+nochain';
          }

            // Chain without bear (should be rare). Fall back to base room image.
            return null;
        }

        default:
          return null;
      }
    }

    // Refresh the room image based on the *current* location and object state.
    // Called on location changes AND after each printed line.
    function refreshRoomVisuals() {
      if (!ModuleRef) return;
      var loc = getCurrentLoc();
      if (typeof loc !== 'number' || loc <= 0) return;

      // Special-case image overrides. If a handler returns a string key, use it.
      var overrideKey = handleSpecialLocations(loc);
      if (overrideKey) {
        setRoomImageByKey(overrideKey);
      } else {
        updateRoomImage(loc);
      }
      renderRoomItems(loc);
    }

    function startLocationWatcher(ModuleInstance) {
      var lastLoc = null;

      // Cache into outer scope so refreshRoomVisuals() can call it too.
      if (typeof ModuleInstance.cwrap === 'function') {
        try {
          getLocFn = ModuleInstance.cwrap('get_current_location', 'number', []);
        } catch (e) {
          console.warn('Colossal Adventure: cwrap(get_current_location) failed', e);
        }

        try {
          setLocFn = ModuleInstance.cwrap('set_current_location', 'number', ['number']);
        } catch (e) {
          console.warn('Colossal Adventure: cwrap(set_current_location) failed', e);
        }

        try {
          spawnDwarfFn = ModuleInstance.cwrap('spawn_dwarf', 'number', ['number']);
        } catch (e) {
          console.warn('Colossal Adventure: cwrap(spawn_dwarf) failed', e);
        }

        try {
          spawnAllDwarvesFn = ModuleInstance.cwrap('spawn_all_dwarves', 'number', []);
        } catch (e) {
          console.warn('Colossal Adventure: cwrap(spawn_all_dwarves) failed', e);
        }

        try {
          spawnPirateFn = ModuleInstance.cwrap('spawn_pirate', 'number', []);
        } catch (e) {
          console.warn('Colossal Adventure: cwrap(spawn_pirate) failed', e);
        }
      }

      if (!getLocFn && typeof ModuleInstance._get_current_location === 'function') {
        getLocFn = ModuleInstance._get_current_location.bind(ModuleInstance);
      }


      if (!setLocFn && typeof ModuleInstance._set_current_location === 'function') {
        setLocFn = function (v) { return ModuleInstance._set_current_location(v | 0) | 0; };
      }
      if (!spawnDwarfFn && typeof ModuleInstance._spawn_dwarf === 'function') {
        spawnDwarfFn = function (n) { return ModuleInstance._spawn_dwarf(n | 0) | 0; };
      }
      if (!spawnAllDwarvesFn && typeof ModuleInstance._spawn_all_dwarves === 'function') {
        spawnAllDwarvesFn = function () { return ModuleInstance._spawn_all_dwarves() | 0; };
      }
      if (!spawnPirateFn && typeof ModuleInstance._spawn_pirate === 'function') {
        spawnPirateFn = function () { return ModuleInstance._spawn_pirate() | 0; };
      }

      if (!getLocFn) {
        console.warn('Colossal Adventure: no get_current_location() export found');
        return;
      }

      if (!setLocFn) {
        console.warn('Colossal Adventure: no set_current_location() export found; :warp disabled');
      }
      if (!spawnDwarfFn || !spawnPirateFn) {
        console.warn('Colossal Adventure: spawn exports missing; :spawn disabled');
      }


      console.log('Colossal Adventure: starting location watcher');

      function tick() {
        if (!gameActive) return;
        var loc;
        try {
          loc = getLocFn();
        } catch (e) {
          console.warn('get_current_location() failed:', e);
          return;
        }
        if (typeof loc === 'number' && loc > 0 && loc !== lastLoc) {
          lastLoc = loc;
          console.log('Colossal Adventure: location changed to', loc);
          // Use the same refresh logic as the print handler.
          refreshRoomVisuals();
        }
        window.requestAnimationFrame(tick);
      }

      window.requestAnimationFrame(tick);
    }

    // --- Dwarf toggle bridge ------------------------------------------
    function bindDwarfToggle(ModuleInstance) {
      var setDisableFn = null;

      // Your C hook will be: void set_dwarves_disabled(int disabled)
      if (typeof ModuleInstance.cwrap === 'function') {
        try {
          setDisableFn = ModuleInstance.cwrap('set_dwarves_disabled', null, ['number']);
        } catch (e) {
          console.warn('Colossal Adventure: cwrap(set_dwarves_disabled) failed', e);
        }
      }

      if (!setDisableFn && typeof ModuleInstance._set_dwarves_disabled === 'function') {
        setDisableFn = function (v) { ModuleInstance._set_dwarves_disabled(v | 0); };
      }

      if (!setDisableFn) {
        console.warn('Colossal Adventure: no set_dwarves_disabled() export found');
        // Keep UI but do nothing.
        return;
      }

      function applyToWasm() {
        var disabled = disableDwarvesEl && disableDwarvesEl.checked ? 1 : 0;
        try {
          setDisableFn(disabled);
        } catch (e) {
          console.warn('set_dwarves_disabled() failed:', e);
        }

        try {
          window.localStorage.setItem('advent_disable_dwarves', disabled ? '1' : '0');
        } catch (e) {}
      }

      // Apply once immediately, so “disable” takes effect before the first dwarf logic tick.
      applyToWasm();

      if (disableDwarvesEl) {
        disableDwarvesEl.addEventListener('change', function () {
          applyToWasm();
          appendLine(
            disableDwarvesEl.checked
              ? '[option] Dwarves disabled.'
              : '[option] Dwarves enabled.'
          );
        });
      }
    }

    // --- Input bridge -----------------
    window.awaitInputFromDom = function () {
      if (!gameActive) {
        return Promise.resolve('');
      }
      // Each time the engine asks for new input, the previous command
      // has finished executing and all object/room state is up to date.
      // Refresh the room visuals so changes that don't move the player
      // (e.g. the snake disappearing) are reflected immediately.
      refreshRoomVisuals();
      return new Promise(function (resolve) {
        pendingResolve = resolve;
        inputEl.disabled = false;
        inputEl.focus();
      });
    };

    ui.formEl.addEventListener('submit', function (ev) {
      ev.preventDefault();
      if (!pendingResolve || !gameActive) {
        return;
      }

      var value = (inputEl.value || '').replace(/\r?\n/g, '');
      inputEl.value = '';

      
      // --- Debug console commands (handled by JS, not the game) ----------
      //   :warp <N> / :loc <N>         -> force location number, then auto "look"
      //   :spawn dwarf [1-5]           -> spawn a dwarf into the current location
      //   :spawn dwarves               -> spawn all 5 dwarves into the current location
      //   :spawn pirate                -> spawn the pirate into the current location
      //
      // Notes:
      // - We must still resolve the pending input so the engine continues.
      // - We send "look" so you immediately see the room description and
      //   the engine gets a full turn to run creature logic.
      var mWarp = /^\s*:(?:warp|loc)\s+(\d+)\s*$/i.exec(value);
      if (mWarp) {
        appendLine('> ' + value);

        var resolveWarp = pendingResolve;
        pendingResolve = null;
        inputEl.disabled = true;

        var target = parseInt(mWarp[1], 10) | 0;
        var resWarp = tryWarpToLocation(target);
        if (resWarp && resWarp.ok) {
          appendLine('[debug] Warped to location ' + resWarp.loc + '.');
          refreshRoomVisuals();
          resolveWarp('look');
        } else {
          var whyW = resWarp && resWarp.code ? resWarp.code : 'unknown';
          appendLine('[debug] Warp failed (' + whyW + ').');
          resolveWarp('');
        }
        return;
      }

      var mSpawn = /^\s*:spawn\s+(dwarf|dwarves|pirate)(?:\s+(\d+))?\s*$/i.exec(value);
      if (mSpawn) {
        appendLine('> ' + value);

        var resolveSpawn = pendingResolve;
        pendingResolve = null;
        inputEl.disabled = true;

        var kind = (mSpawn[1] || '').toLowerCase();
        var which = mSpawn[2] ? (parseInt(mSpawn[2], 10) | 0) : 0;

        var resSpawn = trySpawn(kind, which);
        if (resSpawn && resSpawn.ok) {
          if (kind === 'dwarf') {
            appendLine('[debug] Spawned dwarf #' + (resSpawn.which | 0) + ' into current location.');
          } else {
            appendLine('[debug] Spawned ' + resSpawn.who + ' into current location.');
          }
          refreshRoomVisuals();
          resolveSpawn('look');
        } else {
          var whyS = resSpawn && resSpawn.code ? resSpawn.code : 'unknown';
          appendLine('[debug] Spawn failed (' + whyS + ').');
          resolveSpawn('');
        }
        return;
      }
      // ------------------------------------------------------------------

appendLine('> ' + value);

      var resolve = pendingResolve;
      pendingResolve = null;

      inputEl.disabled = true;
      resolve(value);
    });

    function handleExit(status, message) {
      if (!gameActive) return;
      gameActive = false;

      if (message) {
        appendLine(message);
      }

      inputEl.disabled = true;
      restartBtn.style.display = 'inline-block';

      if (pendingResolve) {
        var resolve = pendingResolve;
        pendingResolve = null;
        resolve('');
      }

      applyLayout();
    }

    window.__adventHandleExit = handleExit;

    restartBtn.addEventListener('click', function () {
      if (gameActive) return;
      window.removeEventListener('resize', applyLayout);
      runGame(root);
    });

    appendLine('Initializing adventure...\n');

    var factory =
      typeof createAdventModule === 'function'
        ? createAdventModule
        : typeof Module === 'function'
        ? Module
        : null;

    if (!factory) {
      appendLine(
        '[err] No Emscripten module factory found (createAdventModule/Module).'
      );
      handleExit(-1);
      return;
    }

    var moduleConfig = {
      locateFile: function (path) {
        if (path.endsWith('.wasm') || path.endsWith('.data')) {
          return assetBaseUrl + path;
        }
        return path;
      },
      filePackagePrefixURL: assetBaseUrl,

      print: function (text) {
        if (text) {
          appendLine(String(text));
        }
      },
      printErr: function (text) {
        if (text) appendLine('[err] ' + String(text));
      },
    };

    factory(moduleConfig)
      .then(function (ModuleInstance) {
        console.log('Colossal Adventure: module ready', ModuleInstance);
        ModuleRef = ModuleInstance;

        (function initGetObjProp() {
            if (typeof ModuleInstance.cwrap === 'function') {
                try {
                    getObjPropFn = ModuleInstance.cwrap('get_object_prop','number',['number']);
                } catch (e) {}
            }

            if (!getObjPropFn) {
                if (typeof ModuleInstance._get_object_prop === 'function') {
                    getObjPropFn = function (o) {
                        return ModuleInstance._get_object_prop(o | 0);
                    };
                }
            }

            if (!getObjPropFn) {
                console.warn(
                    'Colossal Adventure: get_object_prop() export not found; state images disabled'
                );
            }
        })();

        // Discover getters for place[obj] / fixed[obj] (runtime location / fixed position).
        (function initGetObjPlaceFixed() {
            if (typeof ModuleInstance.cwrap === 'function') {
              try {
                getObjPlaceFn = ModuleInstance.cwrap('get_object_place', 'number', ['number']);
              } catch (e) {}
              try {
                getObjFixedFn = ModuleInstance.cwrap('get_object_fixed', 'number', ['number']);
              } catch (e) {}
            }
            if (!getObjPlaceFn && typeof ModuleInstance._get_object_place === 'function') {
              getObjPlaceFn = function (o) { return ModuleInstance._get_object_place(o | 0); };
            }
            if (!getObjFixedFn && typeof ModuleInstance._get_object_fixed === 'function') {
              getObjFixedFn = function (o) { return ModuleInstance._get_object_fixed(o | 0); };
            }
            if (!getObjPlaceFn || !getObjFixedFn) {
              console.warn('Colossal Adventure: no get_object_place/get_object_fixed exports found; dynamic item images (e.g. bear/chain) may be wrong');
            }
        })();

        // Apply dwarf option ASAP (before the game gets far enough to trigger dwarf logic)
        bindDwarfToggle(ModuleInstance);
        startLocationWatcher(ModuleInstance);
        applyLayout();

        // Initial refresh once we have the module.
        refreshRoomVisuals();
      })
      .catch(function (err) {
        if (err && err.name === 'ExitStatus') {
          if (err.status === 0) {
            handleExit(0, '\n*** Adventure finished. ***');
          } else {
            handleExit(
              err.status,
              '\n[Adventure exited with status ' + err.status + ']'
            );
          }
          return;
        }

        handleExit(
          -1,
          '\n[Runtime error] ' +
            (err && err.message ? err.message : String(err))
        );
      });
  }

  function startAdventure() {
    var root =
      document.querySelector('[data-advent-root]') ||
      document.getElementById('advent-root');

    if (!root) {
      console.error('Colossal Adventure: no advent root element found');
      return;
    }

    runGame(root);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', startAdventure);
  } else {
    startAdventure();
  }
})();