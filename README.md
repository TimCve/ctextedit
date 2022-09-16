# GTK2 Text Editor
Experimental plain text editor implementation in C using GTK2. In no way superior to the many text editors included as system defaults in GNU/Linux distros, just an experiment with GTK programming.

### Features (subject to updates)
- Opening existing files (ctrl+o)
- Creating new files (ctrl+n)
- Saving changes to files (ctrl+s)
- Quit the editor with ctrl+q
- Adjust the font family and size in editor

### Pending features
- Single-layer undo/redo
- Searching text for keywords

### Building from source
- Install `GCC`, `GNU make` and `GTK2 development libraries`
- `make` to build text editor
- `make dbg` to build text editor with debugging symbols
- `make clean` to remove all binaries
