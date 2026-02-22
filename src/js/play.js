import { setupUI } from './ui.js';
import { AdventBridge } from './bridge.js';
import { AdventRenderer } from './renderer.js';

console.log('[ADVENT] play.js version: adding bear');

const assetBaseUrl =
  (window.ColossalAdventureConfig && ColossalAdventureConfig.assetBaseUrl) || '';

const installGlobalExitHandler = () => {
  if (window.__adventExitHandlerInstalled) {
    return;
  }

  window.__adventExitHandlerInstalled = true;
  window.__adventHandleExit = null;

  window.addEventListener('unhandledrejection', (ev) => {
    const reason = ev.reason || ev.detail || null;

    if (reason && reason.name === 'ExitStatus') {
      ev.preventDefault();

      const status = typeof reason.status === 'number' ? reason.status : -1;
      const msg =
        status === 0
          ? '\n*** Adventure finished. ***'
          : `\n[Adventure exited with status ${status}]`;

      if (typeof window.__adventHandleExit === 'function') {
        window.__adventHandleExit(status, msg);
      } else {
        console.log('Colossal Adventure exited:', msg);
      }
    }
  });
};

const resolveModuleFactory = () => {
  if (typeof createAdventModule === 'function') {
    return createAdventModule;
  }
  if (typeof Module === 'function') {
    return Module;
  }
  return null;
};

const runGame = async (root) => {
  const state = {
    gameActive: true,
    pendingResolve: null,
    moduleRef: null,
    currentLocation: 0,
    lastImageKey: null,
  };

  const ui = setupUI(root);
  ui.scrollRootIntoView();
  ui.showTitleImage(assetBaseUrl);
  ui.restoreDwarfPreference();

  const renderer = new AdventRenderer({ ui, assetBaseUrl, state });
  const bridge = new AdventBridge({ ui, renderer, assetBaseUrl, state });
  renderer.setBridge(bridge);

  const handleExit = (status, message) => {
    if (!state.gameActive) {
      return;
    }

    state.gameActive = false;

    if (message) {
      ui.appendLine(message);
    }

    ui.disableInput();
    ui.showRestartButton();

    if (state.pendingResolve) {
      const resolve = state.pendingResolve;
      state.pendingResolve = null;
      resolve('');
    }

    ui.applyLayout();
  };

  window.__adventHandleExit = handleExit;

  ui.formEl.addEventListener('submit', (ev) => bridge.handleFormSubmit(ev));

  ui.restartBtn.addEventListener('click', () => {
    if (state.gameActive) {
      return;
    }
    ui.detachLayoutHandlers();
    void runGame(root);
  });

  ui.appendLine('Initializing adventure...\n');

  const factory = resolveModuleFactory();
  if (!factory) {
    ui.appendLine('[err] No Emscripten module factory found (createAdventModule/Module).');
    handleExit(-1);
    return;
  }

  try {
    const ModuleInstance = await factory(bridge.createModuleConfig());
    console.log('Colossal Adventure: module ready', ModuleInstance);
    bridge.attachModule(ModuleInstance);
  } catch (err) {
    if (err && err.name === 'ExitStatus') {
      if (err.status === 0) {
        handleExit(0, '\n*** Adventure finished. ***');
      } else {
        handleExit(err.status, `\n[Adventure exited with status ${err.status}]`);
      }
      return;
    }

    handleExit(
      -1,
      `\n[Runtime error] ${err && err.message ? err.message : String(err)}`
    );
  }
};

const startAdventure = () => {
  const root =
    document.querySelector('[data-advent-root]') || document.getElementById('advent-root');

  if (!root) {
    console.error('Colossal Adventure: no advent root element found');
    return;
  }

  void runGame(root);
};

installGlobalExitHandler();

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', startAdventure);
} else {
  startAdventure();
}
