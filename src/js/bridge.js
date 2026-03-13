export class AdventBridge {
  // Creates the JS-side integration layer that sits between the Emscripten
  // module, the DOM-oriented UI helper, and the renderer. This class owns the
  // mutable references to exported WASM functions because those exports may not
  // exist until after the module finishes loading, and some are optional based
  // on which C helpers were compiled into the current build.
  constructor({ ui, renderer, assetBaseUrl, state, debugEnabled = false }) {
    this.ui = ui;
    this.renderer = renderer;
    this.assetBaseUrl = assetBaseUrl;
    this.state = state;
    this.debugEnabled = !!debugEnabled;

    this.exports = {
      getLocFn: null,
      setLocFn: null,
      spawnDwarfFn: null,
      spawnAllDwarvesFn: null,
      spawnPirateFn: null,
      getDwarfFlagFn: null,
      getDwarfLocationFn: null,
      getPlayerDeadFn: null,
      getBirdDisturbedFn: null,
      getLocCondFn: null,
      getObjPropFn: null,
      getObjPlaceFn: null,
      getObjFixedFn: null,
    };

    this.installInputBridge();
  }

  // Reports whether the current module exposes object placement queries. The
  // renderer uses this as a capability check before attempting item overlays
  // that depend on live object locations rather than static room artwork.
  hasObjectLocationAccess() {
    return typeof this.exports.getObjPlaceFn === 'function';
  }

  // Reports whether both moveable-object and fixed-location accessors exist.
  // Some renderer paths need both because Adventure objects can be present
  // either as moveable items (`place`) or as fixed scenery (`fixed`).
  hasPlaceAndFixedAccess() {
    return (
      typeof this.exports.getObjPlaceFn === 'function' &&
      typeof this.exports.getObjFixedFn === 'function'
    );
  }

  // Safe wrapper around the engine's current-location accessor. The bridge
  // normalizes a missing export or thrown WASM exception into `0`, which the
  // rest of the frontend already treats as "no valid location available yet".
  getCurrentLoc() {
    if (!this.exports.getLocFn) {
      return 0;
    }
    try {
      return this.exports.getLocFn();
    } catch (e) {
      console.warn('get_current_location() failed:', e);
      return 0;
    }
  }

  // Safe wrapper for per-object `prop` state, which the original game uses for
  // many binary or staged object conditions such as open/closed, alive/dead, or
  // plant growth. Returning `null` on failure lets renderer callers distinguish
  // "export unavailable" from real numeric prop values.
  getObjectProp(objNum) {
    if (!this.exports.getObjPropFn) {
      return null;
    }
    try {
      return this.exports.getObjPropFn(objNum | 0);
    } catch (e) {
      console.warn('get_object_prop() failed:', e);
      return null;
    }
  }

  // Safe wrapper for the location condition bitfield. The renderer mainly uses
  // this for darkness/light checks, but the bridge keeps the raw numeric value
  // because other frontend features may eventually need additional condition bits.
  getLocationCond(locNum) {
    if (!this.exports.getLocCondFn) {
      return null;
    }
    try {
      return this.exports.getLocCondFn(locNum | 0);
    } catch (e) {
      console.warn('get_location_cond() failed:', e);
      return null;
    }
  }

  // Safe wrapper for the engine's "player dead" flag. This is isolated here so
  // death-screen logic does not need to know whether the underlying WASM export
  // came from `cwrap`, a raw underscore export, or failed entirely.
  getPlayerDead() {
    if (!this.exports.getPlayerDeadFn) {
      return null;
    }
    try {
      return this.exports.getPlayerDeadFn();
    } catch (e) {
      console.warn('get_player_dead() failed:', e);
      return null;
    }
  }

  // Safe wrapper for the dwarf activation flag. In the original game this flag
  // controls whether dwarves are still dormant, active, or otherwise engaged,
  // and the renderer only wants to draw dwarf overlays once they are relevant.
  getDwarfFlag() {
    if (!this.exports.getDwarfFlagFn) {
      return null;
    }
    try {
      return this.exports.getDwarfFlagFn();
    } catch (e) {
      console.warn('get_dwarf_flag() failed:', e);
      return null;
    }
  }

  // Safe wrapper returning the current location for a specific dwarf slot. The
  // game tracks dwarves as numbered entities, so the renderer queries each one
  // independently when deciding which dwarf sprites belong in the current room.
  getDwarfLocation(which) {
    if (!this.exports.getDwarfLocationFn) {
      return null;
    }
    try {
      return this.exports.getDwarfLocationFn(which | 0);
    } catch (e) {
      console.warn('get_dwarf_location() failed:', e);
      return null;
    }
  }

  // Safe wrapper around the bird-disturbed helper. This exposes a piece of game
  // state that is awkward to infer from generic object tables alone, so the JS
  // renderer can choose the correct bird artwork without duplicating C logic.
  getBirdDisturbed() {
    if (!this.exports.getBirdDisturbedFn) {
      return null;
    }
    try {
      return this.exports.getBirdDisturbedFn();
    } catch (e) {
      console.warn('get_bird_disturbed() failed:', e);
      return null;
    }
  }

  // Safe wrapper returning the main `place` value for an object. The bridge
  // preserves the engine's numeric conventions, including `-1` for carried
  // inventory items, so higher-level code can make precise Adventure-specific
  // decisions about visibility and ownership.
  getObjectPlace(objNum) {
    if (!this.exports.getObjPlaceFn) {
      return null;
    }
    try {
      return this.exports.getObjPlaceFn(objNum | 0);
    } catch (e) {
      console.warn('get_object_place() failed:', e);
      return null;
    }
  }

  // Safe wrapper returning the `fixed` location for an object, used for scenery
  // instances that are attached to a room independent of the moveable object
  // table. Renderer code combines this with `getObjectPlace` for full coverage.
  getObjectFixed(objNum) {
    if (!this.exports.getObjFixedFn) {
      return null;
    }
    try {
      return this.exports.getObjFixedFn(objNum | 0);
    } catch (e) {
      console.warn('get_object_fixed() failed:', e);
      return null;
    }
  }

  // Debug-only helper that asks the engine to move the player directly to a new
  // location. The bridge converts the native return code into a structured JS
  // result object so UI/debug code does not need to know the C-side sentinel
  // conventions, and refreshes visuals immediately on success.
  tryWarpToLocation(newLoc) {
    if (!this.exports.setLocFn) {
      return { ok: false, code: 'no-export' };
    }

    const n = newLoc | 0;
    let rc = -9999;

    try {
      rc = this.exports.setLocFn(n) | 0;
    } catch (e) {
      console.warn('set_current_location() failed:', e);
      return { ok: false, code: 'exception', err: e };
    }

    if (rc === -1) {
      return { ok: false, code: 'not-ready' };
    }
    if (rc === -2) {
      return { ok: false, code: 'out-of-range' };
    }
    if (rc <= 0) {
      return { ok: false, code: 'failed', rc };
    }

    this.renderer.refreshRoomVisuals();
    return { ok: true, loc: rc | 0 };
  }

  // Debug helper for injecting mobile NPCs into the current room. The engine
  // exposes separate helpers for pirate, one dwarf, or all dwarves, so this
  // method centralizes the command decoding and normalizes success/failure into
  // one result shape for the debug command handler.
  trySpawn(kind, which) {
    const k = String(kind || '').toLowerCase();

    try {
      if (k === 'pirate') {
        if (!this.exports.spawnPirateFn) {
          return { ok: false, code: 'no-export' };
        }
        const prc = this.exports.spawnPirateFn() | 0;
        if (prc < 0) {
          return { ok: false, code: 'failed', rc: prc };
        }
        return { ok: true, who: 'pirate' };
      }

      if (k === 'dwarves') {
        if (!this.exports.spawnAllDwarvesFn) {
          return { ok: false, code: 'no-export' };
        }
        const arc = this.exports.spawnAllDwarvesFn() | 0;
        if (arc < 0) {
          return { ok: false, code: 'failed', rc: arc };
        }
        return { ok: true, who: 'dwarves' };
      }

      if (k === 'dwarf') {
        if (!this.exports.spawnDwarfFn) {
          return { ok: false, code: 'no-export' };
        }
        let w = which | 0;
        if (w <= 0) {
          w = 1;
        }
        const drc = this.exports.spawnDwarfFn(w) | 0;
        if (drc < 0) {
          return { ok: false, code: 'failed', rc: drc };
        }
        return { ok: true, who: 'dwarf', which: w };
      }
    } catch (e) {
      console.warn('spawn failed:', e);
      return { ok: false, code: 'exception', err: e };
    }

    return { ok: false, code: 'unknown' };
  }

  // Consumes the pending input resolver installed by `awaitInputFromDom`. This
  // is the handoff point between the engine's async "wait for a command" cycle
  // and the DOM form handler, and it always disables the input field so the UI
  // reflects that the game is no longer waiting for keyboard input.
  consumePendingResolve() {
    const resolve = this.state.pendingResolve;
    this.state.pendingResolve = null;
    this.ui.disableInput();
    return resolve;
  }

  // Parses and services frontend-only debug commands before normal game input
  // reaches the engine. These commands exist entirely on the JS side, so when a
  // match succeeds this method resolves the pending input promise itself rather
  // than forwarding the original text into the Adventure parser.
  handleDebugCommand(value) {
    if (!this.debugEnabled) {
      return false;
    }

    const mWarp = /^\s*:(?:warp|loc)\s+(\d+)\s*$/i.exec(value);
    if (mWarp) {
      this.ui.appendLine(`> ${value}`);

      const resolveWarp = this.consumePendingResolve();
      const target = parseInt(mWarp[1], 10) | 0;
      const resWarp = this.tryWarpToLocation(target);

      if (resWarp && resWarp.ok) {
        this.ui.appendLine(`[debug] Warped to location ${resWarp.loc}.`);
        this.renderer.refreshRoomVisuals();
        resolveWarp('look');
      } else {
        const whyW = resWarp && resWarp.code ? resWarp.code : 'unknown';
        this.ui.appendLine(`[debug] Warp failed (${whyW}).`);
        resolveWarp('');
      }

      return true;
    }

    const mSpawn = /^\s*:spawn\s+(dwarf|dwarves|pirate)(?:\s+(\d+))?\s*$/i.exec(value);
    if (mSpawn) {
      this.ui.appendLine(`> ${value}`);

      const resolveSpawn = this.consumePendingResolve();
      const kind = (mSpawn[1] || '').toLowerCase();
      const which = mSpawn[2] ? parseInt(mSpawn[2], 10) | 0 : 0;
      const resSpawn = this.trySpawn(kind, which);

      if (resSpawn && resSpawn.ok) {
        if (kind === 'dwarf') {
          this.ui.appendLine(
            `[debug] Spawned dwarf #${resSpawn.which | 0} into current location.`
          );
        } else {
          this.ui.appendLine(`[debug] Spawned ${resSpawn.who} into current location.`);
        }
        this.renderer.refreshRoomVisuals();
        resolveSpawn('look');
      } else {
        const whyS = resSpawn && resSpawn.code ? resSpawn.code : 'unknown';
        this.ui.appendLine(`[debug] Spawn failed (${whyS}).`);
        resolveSpawn('');
      }

      return true;
    }

    return false;
  }

  // Form submit handler that turns user-entered text into the next line of game
  // input. It guards against stray submits when the engine is not currently
  // blocked on input, strips newlines to preserve the single-command model, and
  // routes debug commands through the JS-side handler before resolving gameplay
  // input back to the WASM core.
  handleFormSubmit(ev) {
    ev.preventDefault();
    if (!this.state.pendingResolve || !this.state.gameActive) {
      return;
    }

    const value = (this.ui.inputEl.value || '').replace(/\r?\n/g, '');
    this.ui.inputEl.value = '';

    if (this.debugEnabled && this.handleDebugCommand(value)) {
      return;
    }

    this.ui.appendLine(`> ${value}`);
    const resolve = this.consumePendingResolve();
    resolve(value);
  }

  // Installs the global async function that the Emscripten-facing glue calls
  // whenever the C game wants another command from the player. This is the key
  // inversion point in the architecture: the WASM side awaits a promise, while
  // the DOM side later resolves that promise from `handleFormSubmit`.
  installInputBridge() {
    window.awaitInputFromDom = async () => {
      if (!this.state.gameActive) {
        return '';
      }

      this.renderer.refreshRoomVisuals();

      return await new Promise((resolve) => {
        this.state.pendingResolve = resolve;
        this.ui.enableInput();
      });
    };
  }

  // Builds the Emscripten Module configuration used at load time. `locateFile`
  // and `filePackagePrefixURL` make generated `.wasm` and `.data` assets resolve
  // relative to the chosen asset base, while `print` and `printErr` funnel game
  // output into the UI and trigger visual refreshes after each emitted line.
  createModuleConfig() {
    return {
      locateFile: (path) => {
        if (path.endsWith('.wasm') || path.endsWith('.data')) {
          return `${this.assetBaseUrl}${path}`;
        }
        return path;
      },
      filePackagePrefixURL: this.assetBaseUrl,
      print: (text) => {
        if (text) {
          this.ui.appendLine(String(text));
          this.renderer.refreshRoomVisuals();
        }
      },
      printErr: (text) => {
        if (text) {
          this.ui.appendLine(`[err] ${String(text)}`);
          this.renderer.refreshRoomVisuals();
        }
      },
    };
  }

  // Discovers the core engine exports needed for room tracking, death state,
  // debug movement/spawn helpers, and other high-level game queries. It first
  // prefers `cwrap` for ergonomic typed wrappers, then falls back to raw `_foo`
  // exports when necessary so the frontend remains compatible with multiple
  // Emscripten build shapes.
  initCoreAccessors(ModuleInstance) {
    if (typeof ModuleInstance.cwrap === 'function') {
      try {
        this.exports.getLocFn = ModuleInstance.cwrap('get_current_location', 'number', []);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(get_current_location) failed', e);
      }

      try {
        this.exports.setLocFn = ModuleInstance.cwrap('set_current_location', 'number', ['number']);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(set_current_location) failed', e);
      }

      try {
        this.exports.spawnDwarfFn = ModuleInstance.cwrap('spawn_dwarf', 'number', ['number']);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(spawn_dwarf) failed', e);
      }

      try {
        this.exports.spawnAllDwarvesFn = ModuleInstance.cwrap('spawn_all_dwarves', 'number', []);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(spawn_all_dwarves) failed', e);
      }

      try {
        this.exports.spawnPirateFn = ModuleInstance.cwrap('spawn_pirate', 'number', []);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(spawn_pirate) failed', e);
      }

      try {
        this.exports.getDwarfFlagFn = ModuleInstance.cwrap('get_dwarf_flag', 'number', []);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(get_dwarf_flag) failed', e);
      }

      try {
        this.exports.getDwarfLocationFn = ModuleInstance.cwrap('get_dwarf_location', 'number', ['number']);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(get_dwarf_location) failed', e);
      }

      try {
        this.exports.getPlayerDeadFn = ModuleInstance.cwrap('get_player_dead', 'number', []);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(get_player_dead) failed', e);
      }

      try {
        this.exports.getBirdDisturbedFn = ModuleInstance.cwrap('get_bird_disturbed', 'number', []);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(get_bird_disturbed) failed', e);
      }

      try {
        this.exports.getLocCondFn = ModuleInstance.cwrap('get_location_cond', 'number', ['number']);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(get_location_cond) failed', e);
      }
    }

    if (!this.exports.getLocFn && typeof ModuleInstance._get_current_location === 'function') {
      this.exports.getLocFn = ModuleInstance._get_current_location.bind(ModuleInstance);
    }

    if (!this.exports.setLocFn && typeof ModuleInstance._set_current_location === 'function') {
      this.exports.setLocFn = (v) => ModuleInstance._set_current_location(v | 0) | 0;
    }
    if (!this.exports.spawnDwarfFn && typeof ModuleInstance._spawn_dwarf === 'function') {
      this.exports.spawnDwarfFn = (n) => ModuleInstance._spawn_dwarf(n | 0) | 0;
    }
    if (
      !this.exports.spawnAllDwarvesFn &&
      typeof ModuleInstance._spawn_all_dwarves === 'function'
    ) {
      this.exports.spawnAllDwarvesFn = () => ModuleInstance._spawn_all_dwarves() | 0;
    }
    if (!this.exports.spawnPirateFn && typeof ModuleInstance._spawn_pirate === 'function') {
      this.exports.spawnPirateFn = () => ModuleInstance._spawn_pirate() | 0;
    }
    if (!this.exports.getDwarfFlagFn && typeof ModuleInstance._get_dwarf_flag === 'function') {
      this.exports.getDwarfFlagFn = () => ModuleInstance._get_dwarf_flag() | 0;
    }
    if (!this.exports.getDwarfLocationFn && typeof ModuleInstance._get_dwarf_location === 'function') {
      this.exports.getDwarfLocationFn = (n) => ModuleInstance._get_dwarf_location(n | 0) | 0;
    }
    if (!this.exports.getPlayerDeadFn && typeof ModuleInstance._get_player_dead === 'function') {
      this.exports.getPlayerDeadFn = () => ModuleInstance._get_player_dead() | 0;
    }
    if (!this.exports.getBirdDisturbedFn && typeof ModuleInstance._get_bird_disturbed === 'function') {
      this.exports.getBirdDisturbedFn = () => ModuleInstance._get_bird_disturbed() | 0;
    }
    if (!this.exports.getLocCondFn && typeof ModuleInstance._get_location_cond === 'function') {
      this.exports.getLocCondFn = (n) => ModuleInstance._get_location_cond(n | 0) | 0;
    }

    if (!this.exports.getLocFn) {
      console.warn('Colossal Adventure: no get_current_location() export found');
      return false;
    }

    if (this.debugEnabled && !this.exports.setLocFn) {
      console.warn('Colossal Adventure: no set_current_location() export found; :warp disabled');
    }
    if (this.debugEnabled && (!this.exports.spawnDwarfFn || !this.exports.spawnPirateFn)) {
      console.warn('Colossal Adventure: spawn exports missing; :spawn disabled');
    }
    if (!this.exports.getDwarfFlagFn || !this.exports.getDwarfLocationFn) {
      console.warn('Colossal Adventure: dwarf location exports missing; dwarf image overlay may be wrong');
    }
    if (!this.exports.getPlayerDeadFn) {
      console.warn('Colossal Adventure: no get_player_dead() export found; death image logic may be wrong');
    }
    if (!this.exports.getBirdDisturbedFn) {
      console.warn('Colossal Adventure: no get_bird_disturbed() export found; bird disturbed image logic may be wrong');
    }
    if (!this.exports.getLocCondFn) {
      console.warn('Colossal Adventure: no get_location_cond() export found; dark-room image logic may be wrong');
    }

    return true;
  }

  // Discovers object-table accessors used by the renderer's stateful artwork and
  // dynamic item placement logic. These are optional at runtime, so the bridge
  // records whichever variants are available and emits warnings instead of hard
  // failing when a reduced build omits them.
  initObjectAccessors(ModuleInstance) {
    if (typeof ModuleInstance.cwrap === 'function') {
      try {
        this.exports.getObjPropFn = ModuleInstance.cwrap('get_object_prop', 'number', ['number']);
      } catch (e) {}

      try {
        this.exports.getObjPlaceFn = ModuleInstance.cwrap('get_object_place', 'number', ['number']);
      } catch (e) {}

      try {
        this.exports.getObjFixedFn = ModuleInstance.cwrap('get_object_fixed', 'number', ['number']);
      } catch (e) {}
    }

    if (!this.exports.getObjPropFn && typeof ModuleInstance._get_object_prop === 'function') {
      this.exports.getObjPropFn = (o) => ModuleInstance._get_object_prop(o | 0);
    }
    if (!this.exports.getObjPlaceFn && typeof ModuleInstance._get_object_place === 'function') {
      this.exports.getObjPlaceFn = (o) => ModuleInstance._get_object_place(o | 0);
    }
    if (!this.exports.getObjFixedFn && typeof ModuleInstance._get_object_fixed === 'function') {
      this.exports.getObjFixedFn = (o) => ModuleInstance._get_object_fixed(o | 0);
    }

    if (!this.exports.getObjPropFn) {
      console.warn('Colossal Adventure: get_object_prop() export not found; state images disabled');
    }

    if (!this.exports.getObjPlaceFn || !this.exports.getObjFixedFn) {
      console.warn(
        'Colossal Adventure: no get_object_place/get_object_fixed exports found; dynamic item images (e.g. bear/chain) may be wrong'
      );
    }
  }

  // Wires the "disable dwarves" UI option to the corresponding WASM export and
  // persists the selected value in local storage. This keeps the checkbox, the
  // native game flag, and future page loads in sync from one integration point.
  bindDwarfToggle(ModuleInstance) {
    let setDisableFn = null;

    if (typeof ModuleInstance.cwrap === 'function') {
      try {
        setDisableFn = ModuleInstance.cwrap('set_dwarves_disabled', null, ['number']);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(set_dwarves_disabled) failed', e);
      }
    }

    if (!setDisableFn && typeof ModuleInstance._set_dwarves_disabled === 'function') {
      setDisableFn = (v) => {
        ModuleInstance._set_dwarves_disabled(v | 0);
      };
    }

    if (!setDisableFn) {
      console.warn('Colossal Adventure: no set_dwarves_disabled() export found');
      return;
    }

    const applyToWasm = () => {
      const disabled = this.ui.disableDwarvesEl && this.ui.disableDwarvesEl.checked ? 1 : 0;
      try {
        setDisableFn(disabled);
      } catch (e) {
        console.warn('set_dwarves_disabled() failed:', e);
      }

      try {
        window.localStorage.setItem('advent_disable_dwarves', disabled ? '1' : '0');
      } catch (e) {}
    };

    applyToWasm();

    if (this.ui.disableDwarvesEl) {
      this.ui.disableDwarvesEl.addEventListener('change', () => {
        applyToWasm();
        this.ui.appendLine(
          this.ui.disableDwarvesEl.checked
            ? '[option] Dwarves disabled.'
            : '[option] Dwarves enabled.'
        );
      });
    }
  }

  // Starts a lightweight animation-frame poller that watches for room changes on
  // the WASM side. The Adventure core can change location as a side effect of
  // commands without explicitly notifying JS, so this watcher is the mechanism
  // that keeps the renderer synchronized with movement initiated inside the game.
  startLocationWatcher() {
    let lastLoc = null;

    console.log('Colossal Adventure: starting location watcher');

    const tick = () => {
      if (!this.state.gameActive) {
        return;
      }

      let loc;
      try {
        loc = this.exports.getLocFn();
      } catch (e) {
        console.warn('get_current_location() failed:', e);
        return;
      }

      if (typeof loc === 'number' && loc > 0 && loc !== lastLoc) {
        lastLoc = loc;
        this.state.currentLocation = loc;
        console.log('Colossal Adventure: location changed to', loc);
        this.renderer.refreshRoomVisuals();
      }

      window.requestAnimationFrame(tick);
    };

    window.requestAnimationFrame(tick);
  }

  // Finalizes attachment of a loaded Emscripten module to the frontend. This is
  // the bridge's main bootstrapping entry point: it stores the module ref,
  // resolves whatever exports are available, binds option toggles, starts the
  // location watcher if possible, reapplies layout, and performs an initial
  // render so the UI reflects the loaded game state immediately.
  attachModule(ModuleInstance) {
    this.state.moduleRef = ModuleInstance;

    const hasCoreLoc = this.initCoreAccessors(ModuleInstance);
    this.initObjectAccessors(ModuleInstance);
    this.bindDwarfToggle(ModuleInstance);

    if (hasCoreLoc) {
      this.startLocationWatcher();
    }

    this.ui.applyLayout();
    this.renderer.refreshRoomVisuals();
  }
}
