**bridge.js**  This is the WASM<->JS integration layer. It handles the asynchronous input bridge, getting text in and out of the game core without stalling the DOM. It also handles binding the C exports, so we can read game state variables and poke state variables to allow disabling the dwarves, as well as a couple of debug functions to move around the game manually and spawn things.<br>

**config.js** This is where (nearly) all the configuration lives. It contains a lookup table mapping object numbers to filenames for the associated graphical asset. It also codes starting locations, initial image slot assignments and a per-image global scale factor. <br>

**library_getin.js** This is an Emscripten shim function that provides `get_js_input(),` which allows the browser to pause the game when awaiting user input, via the `ASYNCIFY` feature.<br>

**play.js** This is the main loader, as kicked off by the PHP shortcode handler. It locates the root DOM element, sets up runtime variables, creates the UI, renderer and bridge objects, loads the emscripten WASM image, and wires everything up.<br>

**renderer.js** This handles all the graphics. It selects and displays the background image using data from the bridge, overlays item and actor sprites, and handles timing so the image is composited correctly when the game state changes. This code is quite reusable, albeit functionally tied to this application.<br>

**ui.js** The user interface proper. This owns the DOM shell, the layout of all components, and handles resizing (especially important on mobile) elements, and defines the specifics of all elements, such as colours, style hints, and clearances.<br>
