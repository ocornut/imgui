#!/usr/bin/env python3
"""
ImGui C++ Expert — MCP Server
Exposes the agent as an MCP tool that VS Code Copilot Chat can invoke.
Run: python mcp_server.py
VS Code will call it as a subprocess via .vscode/mcp.json
"""

import asyncio
import sys
sys.path.insert(0, __file__.rsplit("/", 1)[0])  # add agent dir to path

from mcp.server.fastmcp import FastMCP
from agent import create_client, chat_with_agent, SYSTEM_PROMPT, MODEL

mcp = FastMCP(
    name="imgui-cpp-expert",
    instructions=SYSTEM_PROMPT,
)

# One shared client (token fetched once at startup)
_client = None
_sessions: dict[str, list[dict]] = {}


def _get_client():
    global _client
    if _client is None:
        _client = create_client()
    return _client


@mcp.tool(description="Ask the ImGui C++ Expert a question. Maintains session context across calls.")
async def ask_imgui_expert(
    question: str,
    session_id: str = "default",
) -> str:
    """
    Ask the ImGui / LLVM+Clang expert agent a question.

    Args:
        question:   The C++/ImGui/LLVM question to ask
        session_id: Optional session name to keep conversation context (default: 'default')

    Returns:
        The expert's response as plain text (may include Markdown)
    """
    client = _get_client()
    if session_id not in _sessions:
        _sessions[session_id] = []

    response, updated = await chat_with_agent(question, _sessions[session_id], client)
    _sessions[session_id] = updated
    return response


@mcp.tool(description="Reset conversation history for a session (start fresh).")
def reset_session(session_id: str = "default") -> str:
    """Clear conversation history for the given session."""
    _sessions.pop(session_id, None)
    return f"Session '{session_id}' reset."


@mcp.tool(description="List active conversation sessions.")
def list_sessions() -> list[str]:
    """Return names of all active (non-empty) sessions."""
    return list(_sessions.keys())


if __name__ == "__main__":
    mcp.run(transport="stdio")
