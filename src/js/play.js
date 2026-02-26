import { setupUI } from './ui.js';
import { AdventBridge } from './bridge.js';
import { AdventRenderer } from './renderer.js';

console.log('[ADVENT] play.js version: adding bear');

const runtimeConfig = window.ColossalAdventureConfig || {};

const parseConfigBool = (value, defaultValue = false) => {
  if (typeof value === 'boolean') {
    return value;
  }

  if (typeof value === 'number') {
    return value !== 0;
  }

  if (typeof value === 'string') {
    const normalized = value.trim().toLowerCase();
    if (['1', 'true', 'yes', 'on'].includes(normalized)) {
      return true;
    }
    if (['0', 'false', 'no', 'off', ''].includes(normalized)) {
      return false;
    }
  }

  return defaultValue;
};

const assetBaseUrl = runtimeConfig.assetBaseUrl || '';
const debugEnabled = parseConfigBool(runtimeConfig.debug, false);

// Global debug flag so module behavior can be toggled from shortcode config.
window.__adventDebugEnabled = debugEnabled;

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
  const bridge = new AdventBridge({ ui, renderer, assetBaseUrl, state, debugEnabled });
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

const LEGACY_ROOT_ID = 'advent-root';
const ROOT_SELECTOR = '[data-advent-root]';

const findRoot = () => {
  const root = document.querySelector(ROOT_SELECTOR);
  if (root) {
    return root;
  }

  return document.getElementById(LEGACY_ROOT_ID);
};

const startRoot = (root) => {
  if (!root || root.dataset.adventStarted === '1') {
    return;
  }

  root.dataset.adventStarted = '1';
  void runGame(root);
};

const observeAndStartRoot = (root) => {
  if (!root) {
    return;
  }

  if (typeof IntersectionObserver !== 'function') {
    startRoot(root);
    return;
  }

  const observer = new IntersectionObserver(
    (entries) => {
      for (const entry of entries) {
        if (!entry.isIntersecting) {
          continue;
        }

        observer.unobserve(entry.target);
        startRoot(entry.target);
      }
    },
    {
      rootMargin: '300px 0px',
      threshold: 0.01,
    }
  );

  observer.observe(root);
};

const startAdventure = () => {
  const root = findRoot();
  if (!root) {
    return;
  }

  observeAndStartRoot(root);
};

installGlobalExitHandler();

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', startAdventure);
} else {
  startAdventure();
}
