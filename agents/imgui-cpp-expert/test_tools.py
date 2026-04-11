#!/usr/bin/env python3
"""
Integration tests for imgui-cpp-expert MCP server tools.
Tests all 3 exposed tools: ask_imgui_expert, reset_session, list_sessions.

Run: python test_tools.py
"""

import asyncio
import sys
import os

# Add agent dir to path so imports work
sys.path.insert(0, os.path.dirname(__file__))

# ── helpers ────────────────────────────────────────────────────────────────────

passed = 0
failed = 0

def ok(label: str, detail: str = ""):
    global passed
    passed += 1
    detail_str = f"\n     {detail}" if detail else ""
    print(f"  \033[32mPASS\033[0m  {label}{detail_str}")

def fail(label: str, reason: str):
    global failed
    failed += 1
    print(f"  \033[31mFAIL\033[0m  {label}  →  {reason}")

def section(name: str):
    print(f"\n=== {name} ===")


# ── import server under test ───────────────────────────────────────────────────

# Import the raw functions, bypassing FastMCP decoration
# We monkey-patch _sessions access directly
import importlib.util, types

spec = importlib.util.spec_from_file_location("mcp_server", os.path.join(os.path.dirname(__file__), "mcp_server.py"))
srv = importlib.util.module_from_spec(spec)
# Pre-populate so the module-level _client and _sessions exist before exec
srv.__name__ = "mcp_server"
spec.loader.exec_module(srv)


# ── test: list_sessions ────────────────────────────────────────────────────────

async def test_list_sessions():
    section("list_sessions")

    srv._sessions.clear()
    result = srv.list_sessions()
    if isinstance(result, list) and len(result) == 0:
        ok("list_sessions() → empty list when no sessions", f"got {result!r}")
    else:
        fail("list_sessions() → empty list", f"got {result!r}")

    # Seed a session manually
    srv._sessions["alpha"] = [{"role": "user", "content": "hi"}]
    srv._sessions["beta"] = []
    result2 = srv.list_sessions()
    if set(result2) == {"alpha", "beta"}:
        ok("list_sessions() → returns all session names", f"got {result2!r}")
    else:
        fail("list_sessions() → all sessions", f"got {result2!r}")


# ── test: reset_session ────────────────────────────────────────────────────────

async def test_reset_session():
    section("reset_session")

    srv._sessions["mytest"] = [{"role": "user", "content": "hello"}]
    result = srv.reset_session("mytest")
    if isinstance(result, str) and "mytest" in result and "reset" in result.lower():
        ok("reset_session('mytest') → confirmation string", f"{result!r}")
    else:
        fail("reset_session('mytest') → confirmation string", f"got {result!r}")

    if "mytest" not in srv._sessions:
        ok("reset_session clears the session from dict")
    else:
        fail("reset_session clears from dict", "session still present")

    # Reset non-existent session (should not raise)
    try:
        r2 = srv.reset_session("nonexistent_xyz")
        ok("reset_session('nonexistent') → no error", f"{r2!r}")
    except Exception as e:
        fail("reset_session('nonexistent') → no error", str(e))


# ── test: ask_imgui_expert ─────────────────────────────────────────────────────

async def test_ask_imgui_expert():
    section("ask_imgui_expert")

    srv._sessions.clear()
    srv._client = None  # force fresh client

    # 1. Simple factual question (short answer expected)
    try:
        answer = await srv.ask_imgui_expert(
            "In one sentence: what does ImGui::Begin() return?",
            session_id="test_ask"
        )
        if isinstance(answer, str) and len(answer) > 20:
            ok("ask_imgui_expert(ImGui question) → non-empty response",
               answer[:120].replace("\n", " "))
        else:
            fail("ask_imgui_expert → non-empty response", f"got {answer!r}")
    except Exception as e:
        fail("ask_imgui_expert(ImGui question)", str(e))

    # 2. Session context kept
    if "test_ask" in srv._sessions and len(srv._sessions["test_ask"]) >= 2:
        ok("ask_imgui_expert → session history saved",
           f"{len(srv._sessions['test_ask'])} messages in 'test_ask'")
    else:
        fail("ask_imgui_expert → session history saved",
             f"sessions={list(srv._sessions.keys())}, len={len(srv._sessions.get('test_ask', []))}")

    # 3. Follow-up stays in context
    try:
        ans2 = await srv.ask_imgui_expert(
            "Any other return value?",
            session_id="test_ask"
        )
        if isinstance(ans2, str) and len(ans2) > 5:
            ok("ask_imgui_expert → follow-up works (session context)",
               ans2[:80].replace("\n", " "))
        else:
            fail("ask_imgui_expert → follow-up", f"got {ans2!r}")
    except Exception as e:
        fail("ask_imgui_expert follow-up", str(e))

    # 4. list_sessions reflects active session
    sessions = srv.list_sessions()
    if "test_ask" in sessions:
        ok("list_sessions after ask → session visible", f"{sessions}")
    else:
        fail("list_sessions after ask → session visible", f"{sessions}")

    # 5. Reset and verify
    srv.reset_session("test_ask")
    if "test_ask" not in srv._sessions:
        ok("reset_session → session removed after ask")
    else:
        fail("reset_session removes session", "still present")


# ── main ──────────────────────────────────────────────────────────────────────

async def main():
    print("imgui-cpp-expert MCP server — tool integration tests")
    print("=" * 50)

    await test_list_sessions()
    await test_reset_session()
    await test_ask_imgui_expert()

    total = passed + failed
    print("\n" + "─" * 50)
    print(f"  {total} tests:  {passed} passed,  {failed} failed")

    if failed:
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())
