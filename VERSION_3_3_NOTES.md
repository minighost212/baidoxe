# Version 3.3 notes

This package extends v3.2 with centralised auxiliary-facility constants, a static motorbike annex, a static two-shaft lift core, a separate B2 world and headless traffic regression tests.

## Important scope distinction
- Motorbikes are static parked props, not moving traffic.
- The elevator is static architecture, not a working lift.
- B2 is a separate assumed demo layout; no real per-floor capacity is claimed.
- The real application still needs a Windows build with OpenGL, GLFW, GLEW and GLM.

See `README_AEON.md`, `SURVEY_AEON.md` and `PROJECT_AUDIT.md` for exact scope and verification limits.
