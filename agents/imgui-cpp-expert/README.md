# ImGui C++ Expert Agent

A specialized AI agent for **Dear ImGui** and **LLVM/Clang** development, powered by Claude Sonnet 4.6 with extended thinking (high reasoning effort).

## Features

- **ImGui Expertise**: Widgets, layout, docking, all backends (GLFW, SDL2/3, Vulkan, OpenGL, Metal, DX9-12, WGPU), internals
- **LLVM/Clang Expertise**: Compiler passes, IR, AST, clang-tidy, MLIR, TableGen, LLD linker
- **Extended Thinking**: High-reasoning mode for complex multi-step problems
- **Workspace Integration**: Real understanding of `/home/joao/vscode/imgui` structure, build system, and indexed compile database
- **Interactive CLI**: Multi-turn conversation mode
- **HTTP Server**: Optional FastAPI server for AI Toolkit integration

## Quick Start

### 1. Setup

```bash
cd /home/joao/vscode/imgui/agents/imgui-cpp-expert

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Configure API key
cp .env.template .env
# Edit .env and add your ANTHROPIC_API_KEY (from your Copilot Pro subscription)
```

### 2. Run Interactive Mode

```bash
python agent.py --mode interactive
```

Example conversation:
```
You: How do I add a custom widget to ImGui using ImGui_internal.h patterns?
Agent (thinking...)
Agent: [detailed response with code examples and patterns...]

You: show me the ImGui::DockSpace() API
Agent: ...
```

### 3. Run HTTP Server Mode

```bash
python agent.py --mode server --port 8000
```

The server provides REST endpoints:
- `POST /chat` — Send a message to the agent
- `POST /reset` — Reset conversation history
- `GET /health` — Health check

Example:
```bash
curl -X POST http://localhost:8000/chat \
  -H "Content-Type: application/json" \
  -d '{
    "message": "How do I write a clang-tidy check for custom C++ patterns?",
    "session_id": "session1",
    "use_thinking": true
  }'
```

## Environment Variables

| Variable | Required | Default | Description |
|---|---|---|---|
| `ANTHROPIC_API_KEY` | ✅ Yes | — | Your Anthropic API key (from Copilot Pro) |
| `VERBOSE_THINKING` | Optional | 0 | Set to 1 to show extended thinking process |
| `SERVER_HOST` | Optional | 0.0.0.0 | HTTP server host |
| `SERVER_PORT` | Optional | 8000 | HTTP server port |

## Configuration

The agent is configured with:
- **Model**: Claude 3.5 Sonnet (`claude-3-5-sonnet-20241022`)
- **Reasoning**: Extended thinking enabled with 10,000 token budget
- **Max Output**: 16,000 tokens per response
- **System Prompt**: 1,400+ words covering both ImGui and LLVM/Clang expertise

## Model Performance

**Claude Sonnet 4.6 with High Reasoning** is optimized for:
- Complex compiler architecture questions (passes, IR transformations, optimization)
- Deep ImGui API semantics (docking system, draw list state machines, asset pipelines)
- Multi-step debugging and design pattern analysis
- Code generation with strong reasoning steps shown

## Use Cases

### ImGui

- Widget authoring and UI layout design
- Backend integration (GLFW, Vulkan, SDL3, Metal, DX11, WGPU)
- Docking system setup and persistent layout management
- Custom rendering via ImDrawList API
- Font atlas and text rasterization (FreeType integration)
- Multi-context usage and state isolation
- Performance profiling with imgui_test_engine

### LLVM/Clang

- Writing FunctionPass and ModulePass implementations
- IR transformations with IRBuilder
- Clang AST traversal and pattern matching
- Custom clang-tidy check authoring
- MLIR dialect and op definition
- TableGen record generation
- LLD linker script semantics
- CMake integration for LLVM projects

## Workspace Context

The agent has deep knowledge of:
- **Main repo**: `/home/joao/vscode/imgui/` (ImGui core + backends + examples)
- **Build system**: CMake (build/) and Makefile (examples/example_glfw_vulkan/)
- **Compile database**: `build/compile_commands.json` (17 indexed files)
- **Primary target**: `example_glfw_vulkan` (GLFW + Vulkan + imgui_test_engine)
- **clangd config**: `.clangd` points to `examples/example_glfw_vulkan` compilation database
- **Python toolchain**: conda env `imgui-cpp` with cling 0.9, ninja 1.13.2, cmake 4.3.1, Jupyter + xcpp17
- **ImGui Test Engine**: Full understanding of test context, assertions, and UI automation APIs

## Debugging

### Show Extended Thinking
```bash
VERBOSE_THINKING=1 python agent.py --mode interactive
```

### API Key Issues
If you get `ANTHROPIC_API_KEY not set` error:
1. Ensure `.env` exists in this directory
2. Add your key: `ANTHROPIC_API_KEY=sk-ant-...`
3. Run with: `source .env && python agent.py`

### Rate Limiting
If hitting Anthropic API rate limits:
- Wait 1 minute before retrying
- Reduce `budget_tokens` in `agent.py` (currently 10,000)
- Use a smaller model if needed (e.g., Claude 3 Haiku)

## Next Steps

1. **Set API key** in `.env`
2. **Run interactive mode** to test
3. **Integrate with VS Code** by configuring launch.json for debugging
4. **Deploy to Azure** (optional) as a containerized HTTP server

## License

This agent is designed for the ImGui repository and LLVM/Clang development workflows.
