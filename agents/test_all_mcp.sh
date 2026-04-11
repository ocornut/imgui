#!/usr/bin/env bash
# test_all_mcp.sh — Global MCP server test suite
# Tests both MCP servers registered in ~/.config/Code/User/mcp.json
# Usage: bash test_all_mcp.sh

set -euo pipefail

NODE="/home/joao/.nvm/versions/node/v24.14.1/bin/node"
PYTHON="/home/joao/miniconda3/envs/imgui-cpp/bin/python"
CPP_REF_DIR="/home/joao/vscode/imgui/agents/cpp-reference"
EXPERT_DIR="/home/joao/vscode/imgui/agents/imgui-cpp-expert"

PASS=0
FAIL=0

header() { echo -e "\n\033[1;34m━━━ $* ━━━\033[0m"; }
ok()     { echo -e "\033[32m  ✓\033[0m $*"; }
err()    { echo -e "\033[31m  ✗\033[0m $*"; }

# ── 1. cpp-reference (Node.js) ─────────────────────────────────────────────────
header "cpp-reference  (Node.js · stdio MCP)"
echo "  server: $CPP_REF_DIR/mcp_server.js"
echo "  runtime: $NODE"
echo ""

TMPOUT=$(mktemp)
if "$NODE" "$CPP_REF_DIR/test_tools.mjs" 2>&1 | tee "$TMPOUT"; then
    SUMMARY=$(grep -E "^──|tests:" "$TMPOUT" | tail -3 || echo "")
    CPP_FAILED=$(grep -oP '\d+(?= failed)' "$TMPOUT" | tail -1 || echo "0")
    CPP_TOTAL=$(grep -oP '^\s+\K\d+(?= tests)' "$TMPOUT" | tail -1 || echo "?")
    if [[ "$CPP_FAILED" == "0" ]]; then
        PASS=$((PASS+1))
        ok "cpp-reference: $CPP_TOTAL tests, 0 failed"
    else
        FAIL=$((FAIL+1))
        err "cpp-reference: $CPP_FAILED test(s) FAILED"
    fi
else
    FAIL=$((FAIL+1))
    err "cpp-reference test runner crashed (rc=$?)"
fi
rm -f "$TMPOUT"

# ── 2. imgui-cpp-expert (Python) ───────────────────────────────────────────────
header "imgui-cpp-expert  (Python · stdio MCP)"
echo "  server: $EXPERT_DIR/mcp_server.py"
echo "  runtime: $PYTHON"
echo ""

TMPOUT2=$(mktemp)
if "$PYTHON" "$EXPERT_DIR/test_tools.py" 2>&1 | tee "$TMPOUT2"; then
    EXP_FAILED=$(grep -oP '\d+(?= failed)' "$TMPOUT2" | tail -1 || echo "0")
    EXP_TOTAL=$(grep -oP '^\s+\K\d+(?= tests)' "$TMPOUT2" | tail -1 || echo "?")
    if [[ "$EXP_FAILED" == "0" ]]; then
        PASS=$((PASS+1))
        ok "imgui-cpp-expert: $EXP_TOTAL tests, 0 failed"
    else
        FAIL=$((FAIL+1))
        err "imgui-cpp-expert: $EXP_FAILED test(s) FAILED"
    fi
else
    FAIL=$((FAIL+1))
    err "imgui-cpp-expert test runner crashed (rc=$?)"
fi
rm -f "$TMPOUT2"

# ── summary ────────────────────────────────────────────────────────────────────
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
SUITES=$((PASS+FAIL))
echo "  Global result: $PASS/$SUITES suites passed"
echo ""
if [[ "$FAIL" -gt 0 ]]; then
    echo -e "  \033[31mFAILED\033[0m  ($FAIL suite(s) with errors)"
    exit 1
else
    echo -e "  \033[32mALL PASS\033[0m"
fi
