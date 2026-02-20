// library_getin.js
mergeInto(LibraryManager.library, {
  get_js_input: function (buf, maxlen) {
    // IMPORTANT: not an `async function`!
    // We use Asyncify.handleAsync to bridge WASM <-> async JS.
    return Asyncify.handleAsync(async () => {

      // Flush any buffered output so the user sees the prompt
      try {
        if (typeof _fflush === 'function') _fflush(0);
      } catch (err) {
        console.warn('fflush failed:', err);
      }
      try {
        if (typeof FS !== 'undefined' &&
            FS.streams &&
            FS.streams[1] &&
            FS.streams[1].tty &&
            FS.streams[1].tty.ops &&
            FS.streams[1].tty.ops.fsync) {
          FS.streams[1].tty.ops.fsync(FS.streams[1].tty);
        }
      } catch (err) {
        console.warn('stdout fsync failed:', err);
      }

      let line = '';

      try {
        if (typeof ENVIRONMENT_IS_NODE !== 'undefined' && ENVIRONMENT_IS_NODE) {
          // Node path (if you ever run the same build in Node)
          const readlineSync = require('readline-sync');
          line = readlineSync.question('');
        } else if (typeof window !== 'undefined' && window.awaitInputFromDom) {
          // Browser path: our play.js defines this
          line = await window.awaitInputFromDom();
        } else if (typeof prompt === 'function') {
          // Fallback: blocking browser prompt
          line = prompt('>') || '';
        }
      } catch (e) {
        console.error('get_js_input failed to read line', e);
        line = '';
      }

      if (typeof line !== 'string') line = '';

      const len = Math.min(line.length, maxlen - 1);
      for (let i = 0; i < len; ++i) {
        HEAPU8[buf + i] = line.charCodeAt(i) & 0xff;
      }
      HEAPU8[buf + len] = 0;

      // C/Fortran side ignores return value, but Asyncify needs something
      return 0;
    });
  },
});
