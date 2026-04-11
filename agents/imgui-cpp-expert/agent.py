#!/usr/bin/env python3
"""
ImGui C++ Expert Agent
Specialized dual-domain agent for Dear ImGui and LLVM/Clang development.
Uses Claude Sonnet 4.6 via GitHub Copilot Pro (no API key required).
"""

import os
import subprocess
import sys
from typing import Optional
import asyncio

from openai import OpenAI

COPILOT_API_BASE = "https://api.githubcopilot.com"
MODEL = "claude-sonnet-4.6"


def get_github_token() -> str:
    """Get GitHub token via gh CLI (uses your active Copilot Pro session)."""
    token = os.getenv("GITHUB_TOKEN")
    if token:
        return token
    try:
        result = subprocess.run(
            ["gh", "auth", "token"],
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        raise RuntimeError(
            "GitHub token not found. Either:\n"
            "  1. Set GITHUB_TOKEN env var, or\n"
            "  2. Run: gh auth login"
        )

# System prompt — defines the expert persona
SYSTEM_PROMPT = """You are an expert C++ developer with deep, production-level knowledge of two domains:
**Dear ImGui** and the **LLVM/Clang compiler infrastructure**.

## ImGui Expertise
- Core C++ API: ImGui::Begin()/End(), all widgets, custom widgets, layout helpers (SameLine, Columns, Table, Group, Child)
- Docking System: ImGui::DockSpace(), ImGuiDockNode, persistent dock layouts, ImGuiWindowClass
- Draw Lists & Custom Rendering: ImDrawList API, GetWindowDrawList(), clipping rects, ImDrawCallback, custom atlas textures
- Backend Integration: All imgui_impl_* files (GLFW, SDL2/SDL3, OpenGL2/3, Vulkan, Metal, DX9-12, WGPU)
- ImGui Internals: imgui_internal.h, ImGuiContext, ImGuiWindow, ImGuiID hashing, input/nav state, font atlas pipeline, ImPool

## LLVM/Clang Expertise
- Compiler Passes: FunctionPass, ModulePass, new PassManager, FunctionAnalysisManager, ModuleAnalysisManager
- IR & IRBuilder: llvm::IRBuilder<>, Value, Instruction, BasicBlock, Function, Module, Type hierarchy, verification
- Clang AST: ASTConsumer, RecursiveASTVisitor, ASTMatcher, MatchCallback, Rewriter, SourceManager, QualType, Decl/Stmt/Expr nodes
- clang-tidy / clang-check: ClangTidyCheck subclasses, registerMatchers, fix-it hints, plugin registration
- Clang Tooling: CommonOptionsParser, ClangTool, FrontendAction, ASTFrontendAction, PluginASTAction, CompilerInstance
- MLIR: Dialect definition, Op/Type registration, RewritePattern, ConversionPattern, pass pipelines, TableGen ODS/DRR
- TableGen: .td syntax, def, class, multiclass, defm, backend-specific records
- LLD: Linker script semantics, LinkerScript, LLD plugin API
- Build & CMake: LLVM's CMake macros, llvm-config, component library linking

## Workspace Context

**Repository**: /home/joao/vscode/imgui
**Build System**: CMake (build/) + Makefile (examples/example_glfw_vulkan/)
**Compile DB**: /home/joao/vscode/imgui/build/compile_commands.json (17 indexed files)
**Primary Target**: example_glfw_vulkan (GLFW + Vulkan + imgui_test_engine)
**clangd Config**: .clangd uses examples/example_glfw_vulkan as compilation database
**Python Toolchain**: conda env imgui-cpp (cling 0.9, ninja 1.13.2, cmake 4.3.1, Jupyter + xcpp17)

## Approach

1. **Read first**: Always read the relevant source file (imgui.h, imgui_internal.h, imgui_impl_*.cpp, LLVM/Clang header) before answering
2. **Search patterns**: Look at existing uses in the codebase; imgui_demo.cpp is the richest reference for widget patterns
3. **Backend specifics**: For any ImGui backend question, open matching imgui_impl_<name>.cpp first
4. **Minimal changes**: Respect immediate-mode paradigm (ImGui) and pass isolation model (LLVM)
5. **Build to verify**: Run `ninja -C build example_glfw_vulkan` and execute ./build/example_glfw_vulkan to test ImGui changes
6. **Use clangd tools**: find_definition, get_hover, workspace_symbol_search, get_diagnostics for index-accurate answers
7. **Read CHANGELOG**: For version compatibility or API migration, consult docs/CHANGELOG.txt first

## Code Conventions

**ImGui**:
- Use IMGUI_API for public symbols; prefer ImGui:: namespace
- Use IM_ASSERT, IM_ALLOC/IM_FREE, IM_NEW/IM_DELETE (never raw new/delete)
- Keep UI logic separate from backend code
- No STL in core; no C++ exceptions/RTTI compatibility
- 4-space indentation, K&R braces

**LLVM/Clang**:
- Prefer new PassManager over legacy RegisterPass<>
- Use llvm::errs() / llvm::outs() instead of std::cerr / std::cout
- Use llvm::Expected<T> and llvm::Error; always handle errors
- Use llvm::StringRef / llvm::Twine for string parameters
- Always call verifyFunction / verifyModule after IR transformations
- 2-space indentation, CamelCase for types/functions

## Constraints

- DO NOT change core ImGui public API signatures unless user explicitly wants a fork
- DO NOT add external library dependencies to ImGui (intentionally lightweight, dependency-free)
- DO NOT use C++ exceptions/RTTI in ImGui code (-fno-exceptions -fno-rtti)
- DO NOT generate OS-specific code without #ifdef guard
- DO NOT use legacy LLVM pass manager in new code
- DO NOT ignore llvm::Error return values
- DO NOT mix Makefile build objects with CMake build objects (different include paths, ABI issues)

You have access to the workspace and can read/search files, run build commands, and provide deep code guidance."""


def create_client() -> OpenAI:
    """Create OpenAI-compatible client pointed at GitHub Copilot API."""
    token = get_github_token()
    return OpenAI(
        base_url=COPILOT_API_BASE,
        api_key=token,
        default_headers={
            "Editor-Version": "vscode/1.99.0",
            "Copilot-Integration-Id": "vscode-chat",
        },
    )


async def chat_with_agent(
    user_message: str,
    conversation_history: list[dict],
    client: OpenAI,
) -> tuple[str, list[dict]]:
    """
    Send a message to the ImGui C++ Expert agent and get a response.

    Args:
        user_message: User's input/question
        conversation_history: List of prior messages (role/content dicts)
        client: OpenAI-compatible client (GitHub Copilot endpoint)

    Returns:
        Tuple of (assistant_response, updated_conversation_history)
    """
    # Add user message to history
    conversation_history.append({"role": "user", "content": user_message})

    # Build messages with system prompt prepended
    messages = [{"role": "system", "content": SYSTEM_PROMPT}] + conversation_history

    # Call GitHub Copilot API (OpenAI-compatible)
    response = client.chat.completions.create(
        model=MODEL,
        messages=messages,
        max_tokens=16000,
    )

    assistant_message = response.choices[0].message.content or ""

    # Add assistant response to history
    conversation_history.append({"role": "assistant", "content": assistant_message})

    return assistant_message, conversation_history


async def main_interactive():
    """Run the agent in interactive CLI mode."""
    print("=" * 70)
    print("ImGui C++ Expert Agent")
    print("=" * 70)
    print("Powered by Claude Sonnet 4.6 via GitHub Copilot Pro")
    print()
    print("Specialized in:")
    print("  • Dear ImGui: widgets, layout, docking, backends, internals")
    print("  • LLVM/Clang: compiler passes, IR, AST, clang-tidy, MLIR, TableGen")
    print()
    print("Type 'quit' or 'exit' to end. Type 'clear' to reset conversation.")
    print("=" * 70)
    print()

    client = create_client()
    conversation_history = []

    while True:
        try:
            user_input = input("You: ").strip()

            if not user_input:
                continue

            if user_input.lower() in ("quit", "exit"):
                print("Goodbye!")
                break

            if user_input.lower() == "clear":
                conversation_history = []
                print("[Conversation history cleared]")
                continue

            print("\nAgent (thinking...)")
            response, conversation_history = await chat_with_agent(
                user_input, conversation_history, client
            )
            print(f"\nAgent: {response}\n")

        except KeyboardInterrupt:
            print("\n\nInterrupted. Goodbye!")
            break
        except Exception as e:
            print(f"\nError: {e}")
            print("Please try again. Ensure 'gh auth login' has been run.\n")


async def main_server():
    """Run the agent as an HTTP server (for integration with AI Toolkit)."""
    # This would use FastAPI or similar to serve the agent as HTTP
    # Placeholder for future Azure AI Agent Server integration
    try:
        from fastapi import FastAPI, HTTPException
        from pydantic import BaseModel
    except ImportError:
        print("FastAPI not installed. Install with: pip install fastapi uvicorn")
        sys.exit(1)

    app = FastAPI(title="ImGui C++ Expert Agent")
    client = create_client()
    conversations = {}  # session_id -> conversation_history

    class UserMessage(BaseModel):
        message: str
        session_id: Optional[str] = None

    class AgentResponse(BaseModel):
        response: str
        session_id: str

    @app.post("/chat", response_model=AgentResponse)
    async def chat_endpoint(user_msg: UserMessage):
        """Chat with the ImGui C++ Expert agent."""
        session_id = user_msg.session_id or "default"
        if session_id not in conversations:
            conversations[session_id] = []

        try:
            response, updated_history = await chat_with_agent(
                user_msg.message,
                conversations[session_id],
                client,
            )
            conversations[session_id] = updated_history
            return AgentResponse(response=response, session_id=session_id)
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

    @app.post("/reset")
    async def reset_endpoint(session_id: str = "default"):
        """Reset conversation history for a session."""
        if session_id in conversations:
            conversations[session_id] = []
        return {"status": "reset", "session_id": session_id}

    @app.get("/health")
    async def health():
        return {"status": "ok", "model": MODEL, "endpoint": COPILOT_API_BASE}

    # Run server
    import uvicorn
    print("Starting ImGui C++ Expert Agent HTTP Server on http://0.0.0.0:8000")
    uvicorn.run(app, host="0.0.0.0", port=8000)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="ImGui C++ Expert Agent")
    parser.add_argument(
        "--mode",
        choices=["interactive", "server"],
        default="interactive",
        help="Run mode: interactive (CLI) or server (HTTP)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8000,
        help="Port for server mode (default: 8000)",
    )

    args = parser.parse_args()

    if args.mode == "interactive":
        asyncio.run(main_interactive())
    else:
        asyncio.run(main_server())
