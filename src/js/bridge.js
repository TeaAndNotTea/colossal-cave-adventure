export class AdventBridge {
  constructor({ ui, renderer, assetBaseUrl, state }) {
    this.ui = ui;
    this.renderer = renderer;
    this.assetBaseUrl = assetBaseUrl;
    this.state = state;

    this.exports = {
      getLocFn: null,
      setLocFn: null,
      spawnDwarfFn: null,
      spawnAllDwarvesFn: null,
      spawnPirateFn: null,
      getPlayerDeadFn: null,
      getLocCondFn: null,
      getObjPropFn: null,
      getObjPlaceFn: null,
      getObjFixedFn: null,
    };

    this.installInputBridge();
  }

  hasObjectLocationAccess() {
    return typeof this.exports.getObjPlaceFn === 'function';
  }

  hasPlaceAndFixedAccess() {
    return (
      typeof this.exports.getObjPlaceFn === 'function' &&
      typeof this.exports.getObjFixedFn === 'function'
    );
  }

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

  consumePendingResolve() {
    const resolve = this.state.pendingResolve;
    this.state.pendingResolve = null;
    this.ui.disableInput();
    return resolve;
  }

  handleDebugCommand(value) {
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

  handleFormSubmit(ev) {
    ev.preventDefault();
    if (!this.state.pendingResolve || !this.state.gameActive) {
      return;
    }

    const value = (this.ui.inputEl.value || '').replace(/\r?\n/g, '');
    this.ui.inputEl.value = '';

    if (this.handleDebugCommand(value)) {
      return;
    }

    this.ui.appendLine(`> ${value}`);
    const resolve = this.consumePendingResolve();
    resolve(value);
  }

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
        this.exports.getPlayerDeadFn = ModuleInstance.cwrap('get_player_dead', 'number', []);
      } catch (e) {
        console.warn('Colossal Adventure: cwrap(get_player_dead) failed', e);
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
    if (!this.exports.getPlayerDeadFn && typeof ModuleInstance._get_player_dead === 'function') {
      this.exports.getPlayerDeadFn = () => ModuleInstance._get_player_dead() | 0;
    }
    if (!this.exports.getLocCondFn && typeof ModuleInstance._get_location_cond === 'function') {
      this.exports.getLocCondFn = (n) => ModuleInstance._get_location_cond(n | 0) | 0;
    }

    if (!this.exports.getLocFn) {
      console.warn('Colossal Adventure: no get_current_location() export found');
      return false;
    }

    if (!this.exports.setLocFn) {
      console.warn('Colossal Adventure: no set_current_location() export found; :warp disabled');
    }
    if (!this.exports.spawnDwarfFn || !this.exports.spawnPirateFn) {
      console.warn('Colossal Adventure: spawn exports missing; :spawn disabled');
    }
    if (!this.exports.getPlayerDeadFn) {
      console.warn('Colossal Adventure: no get_player_dead() export found; death image logic may be wrong');
    }
    if (!this.exports.getLocCondFn) {
      console.warn('Colossal Adventure: no get_location_cond() export found; dark-room image logic may be wrong');
    }

    return true;
  }

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
