Convert a PNG file to GBDK 2bpp C tile arrays ready to paste into tiles.c.

The user has provided: $ARGUMENTS

Steps:
1. Run the converter script:
   ```
   python3 tools/tile_converter.py $ARGUMENTS
   ```
   If no arguments were given, ask the user for: the PNG file path, whether it is a sprite tile (--sprite) or BG/window tile (--bg), how many columns the spritesheet has (--cols N), and a name prefix (--name).

2. Show the full output including the detected palette mapping and the generated C arrays.

3. Ask the user:
   - Which C array in src/tiles.c should these be appended to? (bg_tiles[] or sprite_tiles[])
   - What tile index(es) should the comment numbers start from? (use --start-index)
   - Should any tile names be added to tiles.h as #define constants?

4. If the user confirms, insert the tile data into the correct array in src/tiles.c, add any new #define lines to src/tiles.h, and update BG_TILES_COUNT or SPR_TILES_COUNT if needed.

5. After editing tiles.c/tiles.h, run:
   ```
   make
   ```
   to verify the ROM still compiles.

6. Optionally run the rom-previewer to show how the new tile looks in context:
   ```
   python3 tools/rom_previewer.py
   ```
