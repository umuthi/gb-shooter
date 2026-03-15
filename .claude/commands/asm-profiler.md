Profile the compiled GBZ80 assembly for performance bottlenecks using the .lst files.

Steps:
1. Make sure .lst files exist by running a fresh build:
   ```
   make clean && make
   ```
   The -Wa-l flag in CFLAGS generates src/*.lst automatically.

2. Run the profiler:
   ```
   python3 tools/asm_profiler.py --src src $ARGUMENTS
   ```
   If the user passed --verbose in $ARGUMENTS, pass it through.
   If they specified a function name, use --top 30 to ensure it appears.

3. Display the full report including:
   - Top heaviest functions by instruction count
   - All WARN-level issues (16-bit ops in loops, VRAM writes in loops, division)
   - Recommendations

4. For each WARNING flagged:
   - Explain in plain terms why it is slow on the GBZ80
   - Show the specific lines in the source file that are causing it
   - Suggest a concrete fix

5. If $ARGUMENTS contains "--fix" or the user asks to fix an issue:
   - Make the smallest targeted change to address the flagged bottleneck
   - Re-run make to verify it still compiles
   - Re-run the profiler to confirm the instruction count dropped
