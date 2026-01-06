# aria_shell Tasks

**Last Updated**: 2025-12-26

This file tracks available work for contributors. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## ⚠️ ECOSYSTEM INTEGRATION REQUIREMENTS

**CRITICAL**: Before implementing any shell feature, check for integration dependencies:

📋 **Master Integration Map**: `../aria_ecosystem/INTEGRATION_MAP.md`

**Required Ecosystem Components for aria_shell**:

1. **ecosystem/04_WindowsBootstrap** (CRITICAL - Cross-Platform)
   - Maps Windows HANDLE pointers to FD 3-5 using `__ARIA_FD_MAP` environment variable
   - Must use STARTUPINFOEX + PROC_THREAD_ATTRIBUTE_HANDLE_LIST
   - Required for every spawned process on Windows
   - Spec: `../aria_ecosystem/.internal/research/responses/04_WindowsBootstrap.txt` (462 lines)
   - **Status**: NOT IMPLEMENTED

2. **ecosystem/05_ZeroCopyBridge** (HIGH - Performance)
   - Implements `splice()` for stddati/stddato pipes
   - API: `aria::io::pump(input, output)` in stdlib
   - Prevents pipe deadlock in high-throughput scenarios
   - **NOTE**: Research mentions "threaded draining" but zero-copy "incomplete"
   - Spec: `../aria_ecosystem/.internal/research/responses/05_ZeroCopyBridge.txt` (333 lines)
   - **Status**: Threaded draining exists, splice() NOT IMPLEMENTED

3. **ecosystem/06_TerminalProbe** (MEDIUM - UX)
   - Detects Kitty Keyboard Protocol support via escape sequence negotiation
   - Fallback to Alt+Enter if detection fails
   - Enables Ctrl+Enter for multiline REPL input
   - Spec: `../aria_ecosystem/.internal/research/responses/06_TerminalProbe.txt` (267 lines)
   - **Status**: NOT IMPLEMENTED

4. **ecosystem/07_TelemetryDaemon** (MEDIUM - Observability)
   - Background daemon consuming stddbg (FD 3)
   - JSON log aggregation and forwarding
   - Must not block shell on slow log consumers
   - Spec: `../aria_ecosystem/.internal/research/responses/07_TelemetryDaemon.txt` (277 lines)
   - **Status**: NOT IMPLEMENTED

**Implementation Order**:
1. WindowsBootstrap (blocks Windows support entirely)
2. ZeroCopyBridge (performance critical for data plane)
3. TerminalProbe (UX improvement)
4. TelemetryDaemon (observability enhancement)

**Integration Rules**:
- ✅ Six-stream separation is mandatory (FD 0-2, 3-5)
- ✅ Must preserve FD 3-5 across all child processes
- ✅ Use `aria::io::pump()` for high-throughput transfers
- ❌ Never mix binary data into stdout (use stddato FD 5)
- ❌ Never emit JSON telemetry to stderr (use stddbg FD 3)

See `INTEGRATION_MAP.md` for complete details.

---

## Task Format

Each task includes:
- **ID**: Unique identifier
- **Status**: AVAILABLE, CLAIMED, IN_PROGRESS, COMPLETED
- **Claimed By**: GitHub username (if claimed)
- **Spec**: Reference to specification document
- **Complexity**: LOW, MEDIUM, HIGH
- **Tier**: 1 (first-time), 2 (proven), 3 (core team)
- **Description**: What needs to be done
- **Acceptance Criteria**: How we know it's complete
- **Files**: Affected files

---

## Available Tasks

### Task Format Example

```
ID: AS-001
Status: AVAILABLE
Spec: aria_ecosystem/specs/REPL.md, Section 4.1
Complexity: MEDIUM
Tier: 2
Description: Implement command history with persistence
Acceptance Criteria:
  - Commands saved to ~/.aria_history
  - ↑/↓ keys navigate history
  - History persists across sessions
  - Duplicate consecutive entries filtered
  - Includes comprehensive tests
Files: src/history/history.aria, tests/test_history.aria
```

---

## Tier 1 Tasks (First-Time Contributors)

*No tasks yet. Will be populated as the project develops.*

---

## Tier 2 Tasks (Proven Contributors)

*No tasks yet. Will be populated as the project develops.*

---

## Tier 3 Tasks (Core Team Only)

*No tasks yet. Will be populated as the project develops.*

---

## Completed Tasks

*No completed tasks yet.*

---

## How to Claim a Task

1. Comment on this file in a PR or GitHub Discussion
2. State your approach and estimated timeline
3. Wait for maintainer acknowledgment
4. Create feature branch and start work
5. Reference task ID in commits and PR

---

*Tasks will be added as the project roadmap is broken down into actionable items.*
