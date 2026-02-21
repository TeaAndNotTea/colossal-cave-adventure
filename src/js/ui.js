export class AdventUI {
  constructor(root) {
    this.root = root;

    this.imagePanelEl = null;
    this.imageEl = null;
    this.itemLayerEl = null;
    this.consoleEl = null;
    this.formEl = null;
    this.inputEl = null;
    this.restartBtn = null;
    this.disableDwarvesEl = null;
    this.optionsEl = null;

    this.setupDom();
    this.attachLayoutHandlers();
  }

  setupDom() {
    this.root.style.margin = '0';
    this.root.style.padding = '0';
    this.root.style.boxSizing = 'border-box';

    this.root.innerHTML = `
<div id="advent-image-panel" style="width:100%;max-width:640px;margin:0 auto 0.5rem auto;border:1px solid #444;background:#000;display:flex;align-items:center;justify-content:center;overflow:hidden;">
  <div id="advent-image-wrap" style="position:relative;width:100%;">
    <img id="advent-room-image" src="" alt="Adventure location" style="max-width:100%;height:auto;display:block;" />
    <div id="advent-item-layer" style="position:absolute;left:0;top:0;right:0;bottom:0;pointer-events:none;"></div>
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
    this.consoleEl = this.root.querySelector('#advent-console');
    this.formEl = this.root.querySelector('#advent-form');
    this.inputEl = this.root.querySelector('#advent-input');
    this.restartBtn = this.root.querySelector('#advent-restart');
    this.disableDwarvesEl = this.root.querySelector('#advent-disable-dwarves');
    this.optionsEl = this.root.querySelector('#advent-options');
  }

  scrollRootIntoView() {
    if (this.root && typeof this.root.scrollIntoView === 'function') {
      this.root.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
  }

  showTitleImage(baseUrl) {
    if (!this.imageEl) {
      return;
    }
    this.imageEl.src = `${baseUrl}title.webp`;
    this.imageEl.alt = 'Colossal Cave Adventure';
  }

  restoreDwarfPreference() {
    try {
      const saved = window.localStorage.getItem('advent_disable_dwarves');
      if (saved === '1' && this.disableDwarvesEl) {
        this.disableDwarvesEl.checked = true;
      }
    } catch (e) {}
  }

  appendLine(text = '') {
    if (!this.consoleEl) {
      return;
    }
    this.consoleEl.textContent += `${String(text)}\n`;
    this.consoleEl.scrollTop = this.consoleEl.scrollHeight;
  }

  enableInput() {
    if (!this.inputEl) {
      return;
    }
    this.inputEl.disabled = false;
    this.inputEl.focus();
  }

  disableInput() {
    if (this.inputEl) {
      this.inputEl.disabled = true;
    }
  }

  showRestartButton() {
    if (this.restartBtn) {
      this.restartBtn.style.display = 'inline-block';
    }
  }

  applyLayout = () => {
    if (!this.consoleEl || !this.imagePanelEl || !this.formEl) {
      return;
    }

    const vh = window.innerHeight || document.documentElement.clientHeight || 0;
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

  attachLayoutHandlers() {
    window.addEventListener('resize', this.applyLayout);
    if (this.imageEl) {
      this.imageEl.addEventListener('load', this.applyLayout);
    }
    window.requestAnimationFrame(this.applyLayout);
  }

  detachLayoutHandlers() {
    window.removeEventListener('resize', this.applyLayout);
    if (this.imageEl) {
      this.imageEl.removeEventListener('load', this.applyLayout);
    }
  }
}

export const setupUI = (root) => new AdventUI(root);
