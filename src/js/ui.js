export class AdventUI {
  // Builds the DOM-facing UI controller around an existing root element. This
  // class owns no gameplay logic; its job is to create and retain handles to
  // the image panel, layered overlays, transcript console, and input controls
  // so the bridge and renderer can treat the UI as a small, explicit surface.
  constructor(root) {
    this.root = root;

    this.imagePanelEl = null;
    this.imageEl = null;
    this.itemLayerEl = null;
    this.actorLayerEl = null;
    this.consoleEl = null;
    this.formEl = null;
    this.inputEl = null;
    this.restartBtn = null;
    this.disableDwarvesEl = null;
    this.optionsEl = null;
    this.pendingViewportAlignRaf = 0;

    this.setupDom();
    this.attachLayoutHandlers();
  }

  // Injects the complete static DOM scaffold for the frontend and caches the
  // key nodes the rest of the code touches frequently. The markup is created in
  // one place so layout assumptions stay centralized: a room-image panel with
  // two absolute overlay layers, an options strip, a scrolling text console,
  // a single-line input form, and a restart button hidden until needed.
  setupDom() {
    this.root.style.margin = '0';
    this.root.style.padding = '0';
    this.root.style.boxSizing = 'border-box';

    this.root.innerHTML = `
<div id="advent-image-panel" style="width:100%;max-width:640px;margin:0 auto 0.5rem auto;border:1px solid #444;background:#000;display:flex;align-items:center;justify-content:center;overflow:hidden;">
  <div id="advent-image-wrap" style="position:relative;width:100%;">
    <img id="advent-room-image" src="" alt="Adventure location" style="max-width:100%;height:auto;display:block;" />
    <div id="advent-item-layer" style="position:absolute;left:0;top:0;right:0;bottom:0;pointer-events:none;z-index:1;"></div>
    <div id="advent-actor-layer" style="position:absolute;left:0;top:0;right:0;bottom:0;pointer-events:none;z-index:2;"></div>
  </div>
</div>

<div id="advent-options" style="max-width:640px;margin:0 auto 0.5rem auto;padding:0.25rem 0.5rem;border:1px solid #444;background:#111;color:#eee;font-family:monospace;font-size:13px;display:flex;align-items:center;gap:0.75rem;">
  <label style="display:flex;align-items:center;gap:0.4rem;cursor:pointer;user-select:none;">
    <input id="advent-disable-dwarves" type="checkbox" />
    Disable dwarves
  </label>
  <span style="opacity:0.8;">(default off for authenticity)</span>
</div>

<div id="advent-console" style="border:1px solid #444;padding:0.5rem;background:#111;color:#eee;font-family:monospace;font-size:14px;max-height:400px;overflow-y:auto;white-space:pre-wrap;"></div>

<form id="advent-form" style="margin-top:0.5rem;display:flex;gap:0.5rem;">
  <input id="advent-input" type="text" autocomplete="off" style="flex:1;font-family:monospace;font-size:14px;background:#000;color:#eee;border:1px solid #666;padding:0.25rem;" />
  <button type="submit">Send</button>
</form>

<button id="advent-restart" type="button" style="margin-top:0.5rem;display:none;padding:0.3rem 0.7rem;background:#222;color:#eee;border:1px solid #666;cursor:pointer;">
  Restart adventure
</button>`;

    this.imagePanelEl = this.root.querySelector('#advent-image-panel');
    this.imageEl = this.root.querySelector('#advent-room-image');
    this.itemLayerEl = this.root.querySelector('#advent-item-layer');
    this.actorLayerEl = this.root.querySelector('#advent-actor-layer');
    this.consoleEl = this.root.querySelector('#advent-console');
    this.formEl = this.root.querySelector('#advent-form');
    this.inputEl = this.root.querySelector('#advent-input');
    this.restartBtn = this.root.querySelector('#advent-restart');
    this.disableDwarvesEl = this.root.querySelector('#advent-disable-dwarves');
    this.optionsEl = this.root.querySelector('#advent-options');
  }

  // Scrolls the whole adventure widget into view. This is a coarse-grained
  // helper used when higher-level code wants to ensure the UI is visible before
  // or after game interaction, independent of the finer input-focus alignment
  // logic handled elsewhere in this class.
  scrollRootIntoView() {
    if (this.root && typeof this.root.scrollIntoView === 'function') {
      this.root.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
  }

  // Displays the static title art before the game has moved into room-driven
  // rendering. The renderer later replaces this with actual room images, but
  // the UI owns this initial presentation because it is just a direct image swap.
  showTitleImage(baseUrl) {
    if (!this.imageEl) {
      return;
    }
    this.imageEl.src = `${baseUrl}title.webp`;
    this.imageEl.alt = 'Colossal Cave Adventure';
  }

  // Restores the persisted dwarf-toggle checkbox state from local storage. The
  // bridge separately pushes the chosen value into the WASM core; this method
  // only rehydrates the visible control so the page reflects the user's last
  // preference before module attachment finishes.
  restoreDwarfPreference() {
    try {
      const saved = window.localStorage.getItem('advent_disable_dwarves');
      if (saved === '1' && this.disableDwarvesEl) {
        this.disableDwarvesEl.checked = true;
      }
    } catch (e) {}
  }

  // Appends a new line of text to the transcript console and immediately scrolls
  // to the bottom. The rest of the frontend treats the console as append-only,
  // which matches a terminal transcript and avoids DOM fragmentation from many
  // individual line elements.
  appendLine(text = '') {
    if (!this.consoleEl) {
      return;
    }
    this.consoleEl.textContent += `${String(text)}\n`;
    this.consoleEl.scrollTop = this.consoleEl.scrollHeight;
  }

  // Re-enables the command input when the game is awaiting the next player
  // instruction. In addition to focusing the field, this queues a viewport
  // alignment pass so on mobile browsers the image panel remains visible after
  // the on-screen keyboard changes the visual viewport.
  enableInput() {
    if (!this.inputEl) {
      return;
    }
    this.inputEl.disabled = false;
    try {
      this.inputEl.focus({ preventScroll: true });
    } catch (e) {
      this.inputEl.focus();
    }
    this.queueImagePanelTopAlign();
  }

  // Disables the text input while the engine is processing a command or before
  // the game has requested input. This prevents the UI from implying the next
  // command can be submitted when no pending resolver exists on the bridge side.
  disableInput() {
    if (this.inputEl) {
      this.inputEl.disabled = true;
    }
  }

  // Reveals the restart button once the surrounding application decides a manual
  // restart affordance should be shown, typically after the game reaches an end
  // state or the current session can no longer continue normally.
  showRestartButton() {
    if (this.restartBtn) {
      this.restartBtn.style.display = 'inline-block';
    }
  }

  // Returns whether the command field is the document's active focused element.
  // Several viewport-management behaviors are gated on this so the page does
  // not unexpectedly scroll when the player is not actively typing a command.
  isInputFocused() {
    return !!this.inputEl && document.activeElement === this.inputEl;
  }

  // Adjusts page scroll so the top of the room-image panel aligns with the top
  // of the visible viewport rather than the layout viewport. This matters most
  // on mobile browsers where the virtual keyboard shifts the visual viewport and
  // can otherwise push the image partially off-screen while the player types.
  alignImagePanelTopToViewport = () => {
    if (!this.imagePanelEl) {
      return;
    }

    const rect = this.imagePanelEl.getBoundingClientRect();
    const visualViewportTop =
      window.visualViewport && typeof window.visualViewport.offsetTop === 'number'
        ? window.visualViewport.offsetTop
        : 0;
    const delta = rect.top - visualViewportTop;

    if (Math.abs(delta) < 1) {
      return;
    }

    const currentScrollY = window.scrollY || window.pageYOffset || 0;
    const targetScrollY = Math.max(0, currentScrollY + delta);
    window.scrollTo(0, targetScrollY);
  };

  // Schedules a single alignment pass on the next animation frame when typing
  // is active. Deferring with `requestAnimationFrame` allows browser viewport
  // metrics to settle after focus or keyboard transitions and coalesces bursts
  // of related events into one scroll correction.
  queueImagePanelTopAlign = () => {
    if (!this.isInputFocused() || this.pendingViewportAlignRaf) {
      return;
    }

    this.pendingViewportAlignRaf = window.requestAnimationFrame(() => {
      this.pendingViewportAlignRaf = 0;
      if (!this.isInputFocused()) {
        return;
      }
      this.alignImagePanelTopToViewport();
    });
  };

  // Focus-event hook that reuses the queued viewport-alignment path instead of
  // scrolling immediately. Keeping it as a dedicated handler makes add/remove
  // listener bookkeeping straightforward in `attachLayoutHandlers`.
  handleInputFocus = () => {
    this.queueImagePanelTopAlign();
  };

  // Shared handler for visual-viewport resize/scroll changes. These events fire
  // when mobile browser chrome or the on-screen keyboard changes the visible
  // viewport, so the UI recomputes transcript height and then re-checks image
  // alignment to preserve the intended composition.
  handleVisualViewportChange = () => {
    this.applyLayout();
    this.queueImagePanelTopAlign();
  };

  // Recomputes the console height so the image panel, options row, input form,
  // and optional restart button all fit inside the current viewport. The image
  // area is allowed to size naturally from its content; the console absorbs the
  // remaining height so the overall widget stays usable on both desktop and
  // constrained mobile screens.
  applyLayout = () => {
    if (!this.consoleEl || !this.imagePanelEl || !this.formEl) {
      return;
    }

    const vh =
      (window.visualViewport && typeof window.visualViewport.height === 'number'
        ? window.visualViewport.height
        : 0) ||
      window.innerHeight ||
      document.documentElement.clientHeight ||
      0;
    const imageH = this.imagePanelEl.offsetHeight || 0;
    const optionsH = this.optionsEl ? this.optionsEl.offsetHeight || 0 : 0;
    const formH = this.formEl.offsetHeight || 0;
    const restartH =
      this.restartBtn && this.restartBtn.offsetParent !== null
        ? this.restartBtn.offsetHeight || 0
        : 0;

    const padding = 40;
    const available = Math.max(vh - imageH - optionsH - formH - restartH - padding, 100);

    this.consoleEl.style.height = `${available}px`;
    this.consoleEl.style.maxHeight = `${available}px`;
  };

  // Registers the global and element-level listeners that keep the UI sized and
  // aligned as the browser viewport changes. This includes ordinary window
  // resizes, room-image load completion, input focus, and `visualViewport`
  // events for mobile keyboard-aware layout adjustments.
  attachLayoutHandlers() {
    window.addEventListener('resize', this.applyLayout);
    if (this.imageEl) {
      this.imageEl.addEventListener('load', this.applyLayout);
    }
    if (this.inputEl) {
      this.inputEl.addEventListener('focus', this.handleInputFocus);
    }
    if (window.visualViewport) {
      window.visualViewport.addEventListener('resize', this.handleVisualViewportChange);
      window.visualViewport.addEventListener('scroll', this.handleVisualViewportChange);
    }
    window.requestAnimationFrame(this.applyLayout);
  }

  // Removes all layout and viewport listeners installed by `attachLayoutHandlers`
  // and cancels any queued alignment frame. This allows the host application to
  // tear down the UI cleanly without leaving dangling listeners on `window` or
  // on cached DOM elements after the adventure widget is disposed.
  detachLayoutHandlers() {
    if (this.pendingViewportAlignRaf) {
      window.cancelAnimationFrame(this.pendingViewportAlignRaf);
      this.pendingViewportAlignRaf = 0;
    }

    window.removeEventListener('resize', this.applyLayout);
    if (this.imageEl) {
      this.imageEl.removeEventListener('load', this.applyLayout);
    }
    if (this.inputEl) {
      this.inputEl.removeEventListener('focus', this.handleInputFocus);
    }
    if (window.visualViewport) {
      window.visualViewport.removeEventListener('resize', this.handleVisualViewportChange);
      window.visualViewport.removeEventListener('scroll', this.handleVisualViewportChange);
    }
  }
}

// Small factory wrapper used by the rest of the codebase so callers do not need
// to reference the concrete UI class name directly when bootstrapping the app.
export const setupUI = (root) => new AdventUI(root);
