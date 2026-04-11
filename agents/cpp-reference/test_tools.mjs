/**
 * Live test for all three cpp-reference MCP tools.
 * Run:  node test_tools.mjs
 *
 * Imports probeAll and fetchCppHeader directly from the server module so
 * there's no code duplication.  The server's isMain guard prevents it from
 * connecting to stdio when imported.
 */
import { probeAll, fetchCppHeader, searchImgui, listImguiWidgets, analyzeImguiFile } from './mcp_server.js';

// ── helpers ──────────────────────────────────────────────────────────────────
let passed = 0, failed = 0;
function pass(label) { console.log(`  PASS  ${label}`); passed++; }
function fail(label, detail) { console.log(`  FAIL  ${label}${detail ? '  →  ' + detail : ''}`); failed++; }

// ── search_cpp ───────────────────────────────────────────────────────────────
console.log('\n=== search_cpp ===');
{
  const r = await probeAll('std::vector', 3);
  if (r.length >= 1 && r[0].url.includes('vector')) {
    console.log(`     ${r[0].url}`);
    console.log(`     ${r[0].content.split('\n')[0].slice(0, 80)}`);
    pass('search_cpp("std::vector", limit=3)');
  } else {
    fail('search_cpp("std::vector")', `got ${r.length} results`);
  }
}
{
  const r = await probeAll('std::unique_ptr', 2);
  if (r.length >= 1 && r[0].url.includes('unique_ptr')) {
    console.log(`     ${r[0].url}`);
    pass('search_cpp("std::unique_ptr", limit=2)');
  } else {
    fail('search_cpp("std::unique_ptr")', `got ${r.length} results`);
  }
}
{
  const r = await probeAll('std::unordered_map', 1);
  if (r.length >= 1 && r[0].url.includes('unordered_map')) {
    console.log(`     ${r[0].url}`);
    pass('search_cpp("std::unordered_map")');
  } else {
    fail('search_cpp("std::unordered_map")', `got ${r.length} results`);
  }
}

// ── lookup_cpp ───────────────────────────────────────────────────────────────
console.log('\n=== lookup_cpp ===');
{
  const r = await probeAll('std::sort', 1);
  if (r.length && r[0].url.includes('sort')) {
    console.log(`     ${r[0].url}`);
    console.log(`     ${r[0].content.slice(0, 100).replace(/\n/g,' ')}`);
    pass('lookup_cpp("std::sort")');
  } else {
    fail('lookup_cpp("std::sort")', `got ${r.length} results`);
  }
}
{
  const r = await probeAll('std::vector::push_back', 1);
  if (r.length && r[0].url.includes('push_back')) {
    console.log(`     ${r[0].url}`);
    pass('lookup_cpp("std::vector::push_back")');
  } else {
    fail('lookup_cpp("std::vector::push_back")', `got ${r.length} results`);
  }
}
{
  const r = await probeAll('std::optional', 1);
  if (r.length && r[0].url.includes('optional')) {
    console.log(`     ${r[0].url}`);
    pass('lookup_cpp("std::optional")');
  } else {
    fail('lookup_cpp("std::optional")', `got ${r.length} results`);
  }
}
{
  const r = await probeAll('std::shared_ptr', 1);
  if (r.length && r[0].url.includes('shared_ptr')) {
    console.log(`     ${r[0].url}`);
    pass('lookup_cpp("std::shared_ptr")');
  } else {
    fail('lookup_cpp("std::shared_ptr")', `got ${r.length} results`);
  }
}

// ── get_cpp_header ───────────────────────────────────────────────────────────
console.log('\n=== get_cpp_header ===');
{
  const r = await fetchCppHeader('vector');
  if (r.ok && r.text.includes('vector')) {
    console.log(`     ${r.url}`);
    console.log(`     ${r.text.split('\n')[0].slice(0, 80)}`);
    pass('get_cpp_header("vector")');
  } else {
    fail('get_cpp_header("vector")', r.text.slice(0, 80));
  }
}
{
  const r = await fetchCppHeader('<algorithm>');
  if (r.ok && (r.text.includes('sort') || r.text.includes('algorithm'))) {
    console.log(`     ${r.url}`);
    pass('get_cpp_header("<algorithm>")');
  } else {
    fail('get_cpp_header("<algorithm>")', r.text.slice(0, 80));
  }
}
{
  const r = await fetchCppHeader('memory');
  if (r.ok && r.text.includes('unique_ptr')) {
    console.log(`     ${r.url}`);
    pass('get_cpp_header("memory")');
  } else {
    fail('get_cpp_header("memory")', r.text.slice(0, 80));
  }
}
{
  const r = await fetchCppHeader('nonexistent_xyz_header');
  const msg = r.error ?? r.text ?? '';
  if (!r.ok && (msg.includes('not found') || msg.includes('404') || msg.includes('HTTP'))) {
    console.log(`     (expected not-found)  ${msg.slice(0, 60)}`);
    pass('get_cpp_header("nonexistent") → 404 branch');
  } else {
    fail('get_cpp_header("nonexistent")', msg.slice(0, 80));
  }
}

// ── C library fallback ────────────────────────────────────────────────────────
console.log('\n=== C library fallback ===');
{
  // printf lives on cppreference's fprintf page — use fopen which has its own page
  const r = await probeAll('fopen', 1);
  if (r.length && r[0].url.includes('fopen')) {
    console.log(`     ${r[0].url}`);
    pass('lookup_cpp("fopen") → C fallback');
  } else {
    fail('lookup_cpp("fopen") → C fallback', `got ${r.length} results${r[0] ? ': ' + r[0].url : ''}`);
  }
}
{
  const r = await probeAll('malloc', 1);
  if (r.length && r[0].url.includes('malloc')) {
    console.log(`     ${r[0].url}`);
    pass('lookup_cpp("malloc") → C fallback');
  } else {
    fail('lookup_cpp("malloc") → C fallback', `got ${r.length} results`);
  }
}

// ── search_imgui ─────────────────────────────────────────────────────────────
console.log('\n=== search_imgui ===');
{
  // ImGui::Button — a core function with parameter names
  const r = await searchImgui('Button', 3, 0);
  if (r.localHits.length >= 1 && r.localHits[0].snippet.includes('Button')) {
    const preview = r.localHits[0].snippet.split('\n').find(l => l.includes('Button')) ?? '';
    console.log(`     imgui.h line ${r.localHits[0].line}: ${preview.trim().slice(0, 80)}`);
    pass('searchImgui("Button") → imgui.h hit');
  } else {
    fail('searchImgui("Button") → imgui.h hit', `got ${r.localHits.length} hits`);
  }
}
{
  // An enum flag: NoResize
  const r = await searchImgui('NoResize', 3, 0);
  const found = r.localHits.some(h => h.snippet.includes('NoResize'));
  if (found) {
    pass('searchImgui("NoResize") → enum flag in imgui.h');
  } else {
    fail('searchImgui("NoResize") → enum flag in imgui.h', `got ${r.localHits.length} hits`);
  }
}
{
  // Query that should match nothing
  const r = await searchImgui('nonexistent_xyzzy_param_9z', 3, 0);
  if (r.localHits.length === 0 && r.wikiHits.length === 0) {
    pass('searchImgui("nonexistent") → 0 results');
  } else {
    fail('searchImgui("nonexistent") → 0 results',
         `got local=${r.localHits.length} wiki=${r.wikiHits.length}`);
  }
}
{
  // Wiki query — 'DockSpace' should appear in the Docking page
  const r = await searchImgui('DockSpace', 1, 2);
  if (r.wikiHits.length >= 1) {
    console.log(`     wiki: ${r.wikiHits[0].source}`);
    pass('searchImgui("DockSpace") → wiki hit');
  } else {
    fail('searchImgui("DockSpace") → wiki hit', `got ${r.wikiHits.length} wiki pages`);
  }
}

// ── list_imgui_widgets ───────────────────────────────────────────────────────
console.log('\n=== list_imgui_widgets ===');
{
  // All sections — should have many
  const all = listImguiWidgets();
  if (all.length >= 10) {
    console.log(`     ${all.length} sections found`);
    pass('listImguiWidgets() → all sections returned');
  } else {
    fail('listImguiWidgets() → all sections returned', `got ${all.length}`);
  }
}
{
  // Filter to the "Widgets: Main" section (contains Button/Checkbox/RadioButton etc.)
  const r = listImguiWidgets('Main');
  const found = r.length >= 1 && r[0].items.some(s => s.includes('Button'));
  if (found) {
    console.log(`     "${r[0].section}" — ${r[0].items.length} item(s)`);
    pass('listImguiWidgets("Main") → section with Button APIs');
  } else {
    fail('listImguiWidgets("Main")', `got ${r.length} sections`);
  }
}
{
  // Non-existent section
  const r = listImguiWidgets('xyzzy_nonexistent');
  if (r.length === 0) {
    pass('listImguiWidgets("nonexistent") → 0 sections');
  } else {
    fail('listImguiWidgets("nonexistent")', `got ${r.length}`);
  }
}

// ── analyze_imgui_file ───────────────────────────────────────────────────────
console.log('\n=== analyze_imgui_file ===');
{
  // Analyze imgui_demo.cpp — should have very many calls
  const r = analyzeImguiFile('imgui_demo.cpp');
  if (!r.error && r.totalCalls > 200) {
    console.log(`     ${r.totalCalls} calls found in imgui_demo.cpp`);
    pass('analyzeImguiFile("imgui_demo.cpp") → many calls');
  } else {
    fail('analyzeImguiFile("imgui_demo.cpp")',
         r.error ?? `got ${r.totalCalls} calls`);
  }
}
{
  // Check nesting: demo should have Begin calls with depth > 0 descendants
  const r = analyzeImguiFile('imgui_demo.cpp');
  const hasNested = !r.error && r.calls.some(c => c.depth > 0);
  if (hasNested) {
    pass('analyzeImguiFile nesting depth > 0 present');
  } else {
    fail('analyzeImguiFile nesting', r.error ?? 'no nested calls found');
  }
}
{
  // Security: path outside workspace should be rejected
  const r = analyzeImguiFile('/etc/passwd');
  if (r.error && r.error.includes('workspace')) {
    pass('analyzeImguiFile("/etc/passwd") → sandbox error');
  } else {
    fail('analyzeImguiFile sandbox', r.error ?? 'no error returned');
  }
}

// ── summary ──────────────────────────────────────────────────────────────────
console.log(`\n${'─'.repeat(50)}`);
console.log(`  ${passed + failed} tests:  ${passed} passed,  ${failed} failed`);
process.exit(failed > 0 ? 1 : 0);
