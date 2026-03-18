# Colossal Cave Adventure Web edition with graphics
This project is the result of exploration using LLM coding tools to convert the original text adventure game from its DEC PDP-10 origins to run within a modern web browser, retaining all the original quirks. Just for fun I added graphics that track the scene, again created with AI tools (mostly because I suck at drawing.)

# WEBSITE LINK <---ADD ME WHEN LIVE

## Overview
The F2C and image generation are completed. This repository takes the resulting C code, cross compiles to WebAssembly(WASM) which is loaded with a JavaScript wrapper. Additional code handles user interface and graphics. Once issue may be using the provided WASM version of a cut-down libf2c. If this causes an issue, contact me and I will expand the repository to add this part of the flow.

## Platform
Tested only on Fedora Linux Workstation. Other Linux distros should work. It should also work on MacOS and Windows/WSL but that has not been tested.

## Build
Requirements:
- emscripten
- make
- f2c
- node (if required)

Build command:
make deploy

You will need to add some configuration to the top of the Makefile to point to a wordpress container to use the deploy rule. You can also use

make zip

to build a wordpress plugin file that can be uploaded to a server. The target does not have to be wordpress, but will need PHP support, unless you create your own loader.

## Development
advent/         Original Nelson-era Adventure data file (350 point version)<br>
assets/         Graphic assets for background images and item/treasure/actors
docs/           Random documents, images and notes
lib_c/          
plugin_skel/    Title image and PHP source<br>
src/            Source code (C and JS)

## License and Provenance
This project is a modern port and adaptation of the original Colossal Cave Adventure, created by William Crowther (1976) and expanded by Don Woods (1977).

The original program predates modern open-source licensing practices and was historically distributed informally within academic and research communities.

### Original Game Content
The game data (including advent.dat, room descriptions, object definitions, and gameplay logic) is derived from the original Crowther/Woods version.

These materials are included for historical, educational, and preservation purposes.
All rights to the original game content remain with their respective authors.

### This Project
All original work in this repository—including but not limited to:

- C conversion artifacts (via f2c)
- WebAssembly build system and integration
- JavaScript runtime and interface code
- WordPress plugin wrapper and UI
- Graphical assets
- Tooling, scripts, and enhancements

is licensed under the MIT License (see LICENSE file).

### Intent
This project is intended to preserve, modernize, and make accessible a historically significant piece of interactive computing.

The licensing is deliberately permissive to encourage reuse, experimentation, and further adaptation.