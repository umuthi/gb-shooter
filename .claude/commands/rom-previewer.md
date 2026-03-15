Regenerate the sprite preview PNGs from the current tiles.c and report what changed visually.

Steps:
1. Run the previewer script:
   ```
   python3 tools/rom_previewer.py
   ```

2. Read and display the output. The script reports:
   - How many BG and sprite tiles were parsed from tiles.c
   - Which PNG files changed and how many pixels differ
   - Which PNG files are unchanged

3. For each file marked CHANGED, read and display the updated PNG so the user can see the visual result.

4. If any preview changed unexpectedly (e.g. a tile looks broken or off-palette), flag it and offer to investigate the tile data in tiles.c.

5. If $ARGUMENTS contains "commit" or "--commit", and the previews look correct, stage the updated PNGs and commit:
   ```
   git add sprites_preview/
   git commit -m "Update sprite previews"
   ```
