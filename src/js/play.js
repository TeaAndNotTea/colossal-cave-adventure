import { setupUI } from './ui.js';
import { AdventBridge } from './bridge.js';
import { AdventRenderer } from './renderer.js';

console.log('[ADVENT] play.js version: final release v1.0');

// Runtime config is injected by the surrounding page/shortcode layer. This
// file treats it as optional so the frontend can still boot with sensible
// defaults when embedded in simpler contexts or local test pages.
const runtimeConfig = window.ColossalAdventureConfig || {};

// Normalizes loosely typed shortcode/data-attribute style config values into a
// real boolean. This allows `true`, `1`, `yes`, etc. to drive the same feature
// flags without every caller having to agree on one representation.
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

// Installs a singleton global handler for unhandled promise rejections produced
// by the Emscripten runtime when the game exits via `ExitStatus`. Emscripten
// surfaces clean exits as rejected promises, so this hook converts them back
// into normal frontend shutdown messaging and delegates to the current session's
// active exit callback if one has been registered.
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

// Resolves whichever Emscripten module factory the generated build exposed.
// Some builds export `createAdventModule`, others expose `Module` directly; the
// rest of the frontend only wants a callable factory and should not care which
// naming convention the generated glue happened to use.
const resolveModuleFactory = () => {
  if (typeof createAdventModule === 'function') {
    return createAdventModule;
  }
  if (typeof Module === 'function') {
    return Module;
  }
  return null;
};

// Main per-root boot routine. This constructs shared frontend state, creates the
// UI/renderer/bridge trio, wires session-scoped event handlers, loads the WASM
// module, and translates both normal exits and runtime failures into UI updates.
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

  // Session-local shutdown path used for normal exits, runtime failures, and
  // explicit end-of-game handling. It freezes input, shows restart affordances,
  // resolves any outstanding input promise so the engine is not left hanging,
  // and reapplies layout in case the newly shown restart button changes height.
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

  // User text input enters the system through the UI form and is delegated to
  // the bridge, which knows whether to treat the text as a debug command or as
  // gameplay input that resolves the current `awaitInputFromDom` promise.
  ui.formEl.addEventListener('submit', (ev) => bridge.handleFormSubmit(ev));

  // Restart tears down layout listeners from the previous UI instance and then
  // boots a fresh session into the same root. `runGame` rebuilds the entire DOM
  // scaffold, so restarting is simpler and safer than trying to reset in place.
  ui.restartBtn.addEventListener('click', () => {
    if (state.gameActive) {
      return;
    }
    ui.detachLayoutHandlers();
    void runGame(root);
  });

  ui.appendLine('Initializing adventure...\n');

  // Module loading is deferred until after the UI shell is visible so the page
  // has immediate feedback even if the `.wasm`/`.data` fetch and initialization
  // take noticeable time.
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

// Finds the DOM node that should host the adventure widget. The data-attribute
// selector is the preferred modern path; the legacy id is retained so older
// integrations continue to work without markup changes.
const findRoot = () => {
  const root = document.querySelector(ROOT_SELECTOR);
  if (root) {
    return root;
  }

  return document.getElementById(LEGACY_ROOT_ID);
};

// Starts the game exactly once for a given root element. The dataset guard is a
// cheap idempotency check that protects against duplicate startup triggers from
// multiple observers or repeated lifecycle events on the hosting page.
const startRoot = (root) => {
  if (!root || root.dataset.adventStarted === '1') {
    return;
  }

  root.dataset.adventStarted = '1';
  void runGame(root);
};

// Defers startup until the root is near the viewport when the browser supports
// `IntersectionObserver`. This avoids eagerly loading the game on long pages
// where the widget may never be seen, while still falling back to immediate
// startup on older browsers that lack intersection observation.
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

// Top-level page entry point: locate the root container and start observing or
// launching it. Keeping this separate from the DOM-ready check makes the boot
// sequence easier to test and clarifies that root discovery is independent from
// module loading.
const startAdventure = () => {
  const root = findRoot();
  if (!root) {
    return;
  }

  observeAndStartRoot(root);
};

// Install the global exit translation once for the whole page, then start the
// adventure widget once the DOM is ready enough for root discovery.
installGlobalExitHandler();

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', startAdventure);
} else {
  startAdventure();
}
