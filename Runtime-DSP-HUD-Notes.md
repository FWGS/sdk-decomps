# Runtime&DSP&HUD Notes

- **Runtime:** Use VC6 compiler 12.00.8168 and LINK 6.00.8168 with the x86 release MSVCRT 6.00.8797.0. Configure private tool copies to import `VC6CRT.dll` instead of `MSVCRT.dll`; keep the original compiler kit unchanged. Compile with the original kit and link with the configured tools. The debug CRT caused assertions.

- **DSP:** Build Release from `dlls/hl.dsp` (124 sources) and `cl_dll/cl_dll.dsp` (70 sources). The client makefile is stale. Preserve per-file defines, PCH settings, library order and both measured object/source-order profiles. Together with audited metadata reproduction, this setup produced an identical server and 49 differing client bytes.

- **HUD external order:** `cdll_int.obj` lists the same 64 undefined HUD methods in different orders across saved checkouts. The v2 guard canonicalizes only that profiled group. Code, data, symbol types, definition order and typed relocation targets remain checked. Both checkout inputs passed; altered code, types and relocation targets were rejected.
