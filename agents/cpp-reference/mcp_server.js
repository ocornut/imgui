#!/usr/bin/env node
/**
 * C++ Reference MCP Server (JavaScript / Node.js)
 *
 * Tools: search_cpp · lookup_cpp · get_cpp_header
 * Transport: stdio — VS Code discovers this via .vscode/mcp.json
 *
 * Strategy (no external search engine required):
 *   probeAll() fires ~24 candidate cppreference URL patterns in parallel
 *   and keeps every response whose final URL still contains the queried
 *   symbol name.  This works because cppreference follows a predictable
 *   path layout (algorithm/, container/, utility/, memory/, …).
 *
 * Dependencies (install once with `npm install`):
 *   @modelcontextprotocol/sdk  – MCP stdio server
 *   cheerio                    – server-side HTML parsing (like BeautifulSoup)
 *   zod                        – runtime input validation required by MCP SDK
 */

import { McpServer }          from '@modelcontextprotocol/sdk/server/mcp.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { load }               from 'cheerio';
import { z }                  from 'zod';
import { fileURLToPath }      from 'node:url';
import { readFileSync }       from 'node:fs';
import { resolve, dirname }   from 'node:path';

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

const BASE   = 'https://en.cppreference.com/w';
const BASE_C = BASE + '/c';

const HEADERS = {
  'User-Agent':      'Mozilla/5.0 (X11; Linux x86_64; rv:124.0) Gecko/20100101 Firefox/124.0',
  'Accept-Language': 'en-US,en;q=0.9',
  'Accept':          'text/html,application/xhtml+xml',
};

/** C++ namespace path patterns tried in parallel by probeAll(). */
const PATTERNS = [
  'algorithm/{n}',
  'container/{n}',
  'utility/{n}',
  'memory/{n}',
  'string/{n}',
  'string/basic_string/{n}',
  'io/{n}',
  'thread/{n}',
  'mutex/{n}',
  'atomic/{n}',
  'chrono/{n}',
  'filesystem/{n}',
  'numeric/{n}',
  'iterator/{n}',
  'functional/{n}',
  'ranges/{n}',
  'regex/{n}',
  'random/{n}',
  'compare/{n}',
  'concepts/{n}',
  'coroutine/{n}',
  'exception/{n}',
  'language/{n}',
  'types/{n}',
  'error/{n}',
  '{n}',
];

/** C standard library path patterns used as a fallback. */
const PATTERNS_C = [
  'io/{n}',
  'string/byte/{n}',
  'string/wide/{n}',
  'string/multibyte/{n}',
  'stdlib/{n}',
  'math/{n}',
  'numeric/{n}',
  'string/{n}',
  'time/{n}',
  'thread/{n}',
  'atomic/{n}',
  'memory/{n}',
  '{n}',
];

// ---------------------------------------------------------------------------
// ImGui local header + wiki
// ---------------------------------------------------------------------------

/** Absolute path to imgui.h — two directories above agents/cpp-reference/ */
const IMGUI_H          = resolve(dirname(fileURLToPath(import.meta.url)), '../../imgui.h');
/** Workspace root — used to sandbox analyzeImguiFile() to trusted files only. */
const WORKSPACE_ROOT   = resolve(dirname(fileURLToPath(import.meta.url)), '../..');
const IMGUI_WIKI_BASE  = 'https://github.com/ocornut/imgui/wiki';
const IMGUI_WIKI_RAW   = 'https://raw.githubusercontent.com/wiki/ocornut/imgui';

/** Curated list of imgui wiki page slugs (covers nearly all of the wiki). */
const IMGUI_WIKI_PAGES = [
  'FAQ',
  'Getting-Started',
  'Fonts',
  'Tables-and-Columns',
  'Glossary',
  'Debugging-Tips',
  'Tips',
  'Docking',
  'Multi-Viewports',
  'Useful-Extensions',
  'Configuring-Backends',
  'Software-Rasterizer',
  'Color-FAQ',
  'Home',
];

// ---------------------------------------------------------------------------
// In-process response cache — avoids re-fetching within a session
// ---------------------------------------------------------------------------

const _cache    = new Map();   // key → { value, ts }
const CACHE_TTL = 30 * 60_000; // 30 minutes

function cacheGet(key) {
  const e = _cache.get(key);
  if (!e) return null;
  if (Date.now() - e.ts > CACHE_TTL) { _cache.delete(key); return null; }
  return e.value;
}

function cacheSet(key, value) {
  _cache.set(key, { value, ts: Date.now() });
}

// ---------------------------------------------------------------------------
// HTML cleaning
// ---------------------------------------------------------------------------

/**
 * Strip cppreference navigation chrome and return the main content as
 * plain text (up to maxChars characters).
 *
 * @param {string} html
 * @param {number} maxChars
 * @returns {string}
 */
function cleanHtml(html, maxChars = 8000) {
  const $ = load(html);

  // Remove scripts and styles
  $('script, style').remove();

  // Remove cppreference-specific navigation/chrome elements
  for (const cls of [
    't-navbar', 't-nv-begin', 'mw-editsection', 'printfooter',
    'catlinks', 'mw-jump-link', 'noprint', 'toc',
    'mw-indicators', 'mw-body-content-footer',
  ]) {
    $(`.${cls}`).remove();
  }

  for (const id of ['mw-navigation', 'footer', 'mw-head', 'p-logo', 'mw-page-base']) {
    $(`#${id}`).remove();
  }

  // Prefer the main MediaWiki content block
  const main = $('#mw-content-text').length   ? $('#mw-content-text')
             : $('.mw-body-content').length    ? $('.mw-body-content')
             : $('body');

  return main.text()
             .replace(/\n{3,}/g, '\n\n')
             .trim()
             .slice(0, maxChars);
}

// ---------------------------------------------------------------------------
// Parallel URL probing
// ---------------------------------------------------------------------------

/**
 * Fetch one candidate URL and return a result object or null.
 * A hit is genuine when the server returns HTTP 200 (after redirects) AND
 * the final URL still contains `baseName` — guards against 301-to-category-
 * overview false positives (e.g. utility/sort → /utility/).
 *
 * @param {string} url
 * @param {string} baseName   – lower-case symbol leaf name (spaces → _)
 * @param {string} pathFrag   – URL fragment the final URL must contain
 * @returns {Promise<{title:string,url:string,content:string}|null>}
 */
async function probeOne(url, baseName, pathFrag = '/cpp/') {
  try {
    const r = await fetch(url, {
      headers: HEADERS,
      redirect: 'follow',
      signal:   AbortSignal.timeout(10_000),
    });

    if (r.status === 200) {
      const finalUrl = r.url;
      if (finalUrl.toLowerCase().includes(baseName) && finalUrl.includes(pathFrag)) {
        const pathTail = finalUrl.replace(/\/$/, '').split('/').pop().replace(/\.html?$/i, '');
        const html = await r.text();
        return {
          title:   (pathFrag === '/c/' ? '' : 'std::') + pathTail,
          url:     finalUrl,
          content: cleanHtml(html),
        };
      }
    }
  } catch (_) {
    // timeout, DNS failure, network error — skip silently
  }
  return null;
}

/**
 * Probe URL patterns for *symbol* in parallel and return genuine hits as
 * { title, url, content }.  Tries C++ paths first; falls back to C standard
 * library paths when no C++ page is found.
 *
 * @param {string} symbol   – Raw symbol, e.g. "std::sort", "printf"
 * @param {number} limit    – Maximum number of unique results to return
 * @returns {Promise<Array<{title:string, url:string, content:string}>>}
 */
export async function probeAll(symbol, limit = 24) {
  const cacheKey = `${symbol}::${limit}`;
  const cached   = cacheGet(cacheKey);
  if (cached) return cached;

  const clean = symbol.trim()
                      .replace(/^std(::)?/, '')   // strip leading "std::"
                      .replace(/^<|>$/g, '');      // strip angle brackets

  const parts = clean.split('::').map(p => p.trim()).filter(Boolean);
  if (parts.length === 0) return [];

  // Normalise spaces → underscores so they form valid URL path segments
  const tail     = parts.map(p => p.replace(/\s+/g, '_')).join('/');
  const baseName = parts[parts.length - 1].toLowerCase().replace(/\s+/g, '_');

  // --- C++ probe ---
  const cppUrls = PATTERNS.map(pat => `${BASE}/cpp/${pat.replace('{n}', tail)}`);
  const rawCpp  = await Promise.all(cppUrls.map(url => probeOne(url, baseName, '/cpp/')));

  const seen  = new Set();
  const items = [];
  for (const hit of rawCpp) {
    if (hit && items.length < limit && !seen.has(hit.url)) {
      seen.add(hit.url);
      items.push(hit);
    }
  }

  // --- C standard library fallback ---
  if (items.length === 0) {
    const cUrls = PATTERNS_C.map(pat => `${BASE_C}/${pat.replace('{n}', tail)}`);
    const rawC  = await Promise.all(cUrls.map(url => probeOne(url, baseName, '/c/')));
    for (const hit of rawC) {
      if (hit && items.length < limit && !seen.has(hit.url)) {
        seen.add(hit.url);
        items.push(hit);
      }
    }
  }

  cacheSet(cacheKey, items);
  return items;
}

// ---------------------------------------------------------------------------
// MCP server
// ---------------------------------------------------------------------------

const server = new McpServer({
  name:    'cpp-reference',
  version: '1.0.0',
});

// ── search_cpp ──────────────────────────────────────────────────────────────

server.tool(
  'search_cpp',
  'Search cppreference.com for C++ symbols, headers, or language features. ' +
  'Returns a numbered list of matching pages with URLs and short excerpts.',
  {
    query: z.string()
            .describe("Search term, e.g. 'std::vector', 'move semantics', 'constexpr if'"),
    limit: z.number().int().min(1).max(20).default(8)
            .describe('Maximum number of results (1–20, default 8)'),
  },
  async ({ query, limit }) => {
    const n       = Math.max(1, Math.min(limit ?? 8, 20));
    const results = await probeAll(query, n);

    if (results.length === 0) {
      return { content: [{ type: 'text', text:
        `No results found for '${query}'.  ` +
        `Try lookup_cpp('${query}') or get_cpp_header('${query}') directly.`,
      }]};
    }

    const lines = [`cppreference search: '${query}'  (${results.length} results)\n`];
    for (let i = 0; i < results.length; i++) {
      const { title, url, content } = results[i];
      lines.push(`${i + 1}. ${title}`);
      lines.push(`   ${url}`);
      const snippet = (content || '').slice(0, 200).split('\n')[0];
      if (snippet) lines.push(`   ${snippet}`);
      lines.push('');
    }

    return { content: [{ type: 'text', text: lines.join('\n') }] };
  },
);

// ── lookup_cpp ──────────────────────────────────────────────────────────────

server.tool(
  'lookup_cpp',
  "Fetch full documentation for a C++ symbol from cppreference.com. " +
  "Examples: 'std::vector', 'std::sort', 'std::optional', 'RAII', 'move constructor'.",
  {
    symbol: z.string()
             .describe("C++ symbol or topic, e.g. 'std::vector::push_back', 'std::move', 'concepts'"),
  },
  async ({ symbol }) => {
    const hits = await probeAll(symbol, 1);

    if (hits.length > 0) {
      const { url, content } = hits[0];
      return { content: [{ type: 'text', text: `# ${symbol}\nURL: ${url}\n\n${content}` }] };
    }

    return { content: [{ type: 'text', text:
      `No cppreference page found for '${symbol}'.  ` +
      `Tip: try get_cpp_header('${symbol}') if this is a standard header name.`,
    }]};
  },
);

// ── get_cpp_header ──────────────────────────────────────────────────────────

server.tool(
  'get_cpp_header',
  "Fetch the cppreference.com overview for a C++ standard header. " +
  "Examples: '<vector>', '<algorithm>', 'string', 'memory'.",
  {
    header: z.string()
             .describe("Header name with or without angle brackets, e.g. 'vector', '<algorithm>'"),
  },
  async ({ header }) => {
    const { ok, url: finalUrl, text, error } = await fetchCppHeader(header);
    if (!ok) return { content: [{ type: 'text', text: error }] };
    const name = header.trim().replace(/^<|>$/g, '').trim();
    return { content: [{ type: 'text', text: `# <${name}>\nURL: ${finalUrl}\n\n${text}` }] };
  },
);

// ---------------------------------------------------------------------------
// Shared helper — exported so test_tools.mjs can import it without running
// the MCP server connection.
// ---------------------------------------------------------------------------

/**
 * Fetch a C++ standard header overview page from cppreference.com.
 * @param {string} header  – Header name with or without angle brackets
 * @returns {Promise<{ok:boolean, url?:string, text?:string, error?:string}>}
 */
export async function fetchCppHeader(header) {
  const name    = header.trim().replace(/^<|>$/g, '').trim();
  const baseUrl = `${BASE}/cpp/header/${name}`;
  try {
    const r = await fetch(baseUrl, {
      headers: HEADERS,
      redirect: 'follow',
      signal:   AbortSignal.timeout(15_000),
    });
    if (r.status === 404)
      return { ok: false, error: `Header <${name}> not found (404).  Try search_cpp('${name}').` };
    if (!r.ok)
      return { ok: false, error: `HTTP ${r.status} fetching ${baseUrl}` };
    return { ok: true, url: r.url, text: cleanHtml(await r.text()) };
  } catch (err) {
    return { ok: false, error: `Error fetching ${baseUrl}: ${err.message}` };
  }
}

// ---------------------------------------------------------------------------
// ImGui local header + wiki search helpers
// ---------------------------------------------------------------------------

/**
 * Grep imgui.h line-by-line for `query` and return context windows.
 * Pure synchronous, no network.
 *
 * @param {string} query
 * @param {number} maxResults
 * @returns {Array<{line:number, snippet:string}>}
 */
function searchImguiH(query, maxResults = 10) {
  let source;
  try { source = readFileSync(IMGUI_H, 'utf8'); }
  catch (_) { return []; }

  const lines   = source.split('\n');
  const q       = query.toLowerCase();
  const CONTEXT = 5;
  const hits    = [];
  const covered = new Set();

  for (let i = 0; i < lines.length && hits.length < maxResults; i++) {
    if (!lines[i].toLowerCase().includes(q) || covered.has(i)) continue;
    const start = Math.max(0, i - CONTEXT);
    const end   = Math.min(lines.length - 1, i + CONTEXT);
    for (let k = start; k <= end; k++) covered.add(k);
    hits.push({ line: i + 1, snippet: lines.slice(start, end + 1).join('\n') });
  }
  return hits;
}

/**
 * Fetch one imgui wiki page (raw markdown) and return snippets matching `query`.
 * Returns null on network error or no match.
 *
 * @param {string} slug
 * @param {string} query
 * @returns {Promise<{source:string,url:string,snippets:string[]}|null>}
 */
async function searchImguiWikiPage(slug, query) {
  try {
    const r = await fetch(`${IMGUI_WIKI_RAW}/${slug}.md`, {
      headers: HEADERS,
      signal:  AbortSignal.timeout(10_000),
    });
    if (!r.ok) return null;

    const text     = await r.text();
    const q        = query.toLowerCase();
    const lines    = text.split('\n');
    const CONTEXT  = 8;
    const snippets = [];
    const covered  = new Set();

    for (let i = 0; i < lines.length && snippets.length < 3; i++) {
      if (!lines[i].toLowerCase().includes(q) || covered.has(i)) continue;
      const start = Math.max(0, i - CONTEXT);
      const end   = Math.min(lines.length - 1, i + CONTEXT);
      for (let k = start; k <= end; k++) covered.add(k);
      snippets.push(lines.slice(start, end + 1).join('\n'));
    }
    if (!snippets.length) return null;
    return { source: `imgui wiki/${slug}`, url: `${IMGUI_WIKI_BASE}/${slug}`, snippets };
  } catch (_) {
    return null;
  }
}

/**
 * Search imgui.h locally + all curated imgui wiki pages for `query`.
 * Cached for CACHE_TTL like the other tools.
 *
 * @param {string} query
 * @param {number} maxLocal   max results from imgui.h
 * @param {number} maxWiki    max wiki pages to include
 * @returns {Promise<{localHits:Array, wikiHits:Array}>}
 */
export async function searchImgui(query, maxLocal = 5, maxWiki = 3) {
  const cacheKey = `imgui::${query}::${maxLocal}::${maxWiki}`;
  const cached   = cacheGet(cacheKey);
  if (cached) return cached;

  const localHits = searchImguiH(query, maxLocal);
  const wikiHits  = (await Promise.all(
    IMGUI_WIKI_PAGES.map(slug => searchImguiWikiPage(slug, query))
  )).filter(Boolean).slice(0, maxWiki);

  const result = { localHits, wikiHits };
  cacheSet(cacheKey, result);
  return result;
}

// ── search_imgui ─────────────────────────────────────────────────────────────

server.tool(
  'search_imgui',
  'Search Dear ImGui\'s API (imgui.h) and the official imgui GitHub wiki for any ' +
  'parameter, function, flag, enum value, or topic. ' +
  'Returns matching code excerpts from imgui.h and relevant wiki excerpts. ' +
  'Examples: \'WindowPadding\', \'BeginCombo\', \'ImGuiWindowFlags_NoTitleBar\', \'docking\'.',
  {
    query:     z.string()
                .describe(
                  'Parameter, function, flag, or topic to search for, e.g. \'WindowPadding\', ' +
                  '\'BeginCombo\', \'ImGuiWindowFlags_NoTitleBar\', \'docking\''),
    max_local: z.number().int().min(1).max(20).default(5)
                .describe('Max matches from imgui.h (1–20, default 5)'),
    max_wiki:  z.number().int().min(0).max(10).default(3)
                .describe('Max wiki page matches to include (0–10, default 3)'),
  },
  async ({ query, max_local, max_wiki }) => {
    const { localHits, wikiHits } = await searchImgui(
      query, max_local ?? 5, max_wiki ?? 3
    );

    if (!localHits.length && !wikiHits.length) {
      return { content: [{ type: 'text', text:
        `No results found for '${query}' in imgui.h or the imgui wiki.`,
      }]};
    }

    const lines = [`# ImGui search: "${query}"\n`];

    if (localHits.length > 0) {
      lines.push('## imgui.h matches\n');
      for (const { line, snippet } of localHits) {
        lines.push(`### Line ${line} · imgui.h\n\`\`\`cpp\n${snippet}\n\`\`\`\n`);
      }
    }

    if (wikiHits.length > 0) {
      lines.push('## imgui wiki matches\n');
      for (const { source, url, snippets } of wikiHits) {
        lines.push(`### ${source}\nURL: ${url}\n`);
        lines.push(snippets.join('\n\n---\n\n') + '\n');
      }
    }

    return { content: [{ type: 'text', text: lines.join('\n') }] };
  },
);

// ---------------------------------------------------------------------------
// ImGui widget catalog  (parsed from imgui.h, no network)
// ---------------------------------------------------------------------------

/** Section-comment patterns that group IMGUI_API declarations in imgui.h. */
const SECTION_RE = /^    \/\/ ((?:Widgets|Windows|Popups|Tables|Menus|Layout|Clipping|Drag|Disabling|Item|Logging|Color|Inputs|Viewports|Settings)[^\n]*)/;

/**
 * Parse imgui.h and return API functions grouped by section.
 * Optionally filter to sections whose name contains `sectionFilter` (case-insensitive).
 *
 * @param {string|null} sectionFilter
 * @returns {Array<{section:string, items:string[]}>}
 */
export function listImguiWidgets(sectionFilter = null) {
  let source;
  try { source = readFileSync(IMGUI_H, 'utf8'); }
  catch (_) { return []; }

  const lines     = source.split('\n');
  const sections  = [];
  let   curSection = null;

  const wanted = (name) =>
    !sectionFilter || name.toLowerCase().includes(sectionFilter.toLowerCase());

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];

    // Section header inside the ImGui namespace block
    const secM = SECTION_RE.exec(line);
    if (secM) {
      curSection = secM[1].trim().replace(/\s{2,}/g, ' ');
      if (wanted(curSection)) sections.push({ section: curSection, items: [] });
      continue;
    }

    // IMGUI_API declaration — collect full declaration (may span lines, ends at ';')
    if (line.includes('IMGUI_API') && curSection !== null) {
      const last = sections[sections.length - 1];
      if (!last || last.section !== curSection) continue;

      let decl = line.trim();
      let j = i + 1;
      while (!decl.endsWith(';') && j < i + 8 && j < lines.length) {
        decl += ' ' + lines[j].trim();
        j++;
      }
      last.items.push(decl);
    }
  }

  return sections;
}

// ── list_imgui_widgets ────────────────────────────────────────────────────────

server.tool(
  'list_imgui_widgets',
  'Return the complete Dear ImGui widget/API catalog parsed live from imgui.h, ' +
  'grouped by section. Use for designing a UI: know every available widget, its ' +
  'signature, and the section it belongs to. ' +
  "Optionally filter to one section, e.g. section='Buttons', 'Text', 'Combo', " +
  "'Input', 'Drag', 'Slider', 'Trees', 'Tables', 'Popups', 'Menus', 'Clipping'.",
  {
    section: z.string().optional()
              .describe("Case-insensitive section filter, e.g. 'Buttons', 'Input', 'Tables'. " +
                        'Omit to return all sections.'),
  },
  async ({ section }) => {
    const sections = listImguiWidgets(section ?? null);

    if (!sections.length) {
      return { content: [{ type: 'text', text:
        section
          ? `No ImGui sections found matching '${section}'.`
          : 'Could not parse imgui.h — file not found or unreadable.',
      }]};
    }

    const lines = ['# Dear ImGui API Catalog\n',
                   `*(source: imgui.h — ${sections.length} section(s))*\n`];
    for (const { section: name, items } of sections) {
      lines.push(`\n## ${name}\n`);
      if (items.length > 0) {
        lines.push('```cpp');
        lines.push(...items);
        lines.push('```');
      } else {
        lines.push('*(no IMGUI_API declarations in this section)*');
      }
    }

    return { content: [{ type: 'text', text: lines.join('\n') }] };
  },
);

// ---------------------------------------------------------------------------
// ImGui file analyzer  (read a .cpp file, extract the ImGui call hierarchy)
// ---------------------------------------------------------------------------

/**
 * Read a C++ source file (must be inside the workspace) and extract all
 * `ImGui::*` call sites.  Tracks Begin/End nesting so the returned list
 * reflects the GUI hierarchy.
 *
 * @param {string} filePath  Absolute path OR workspace-relative path.
 * @returns {{ file:string, totalCalls:number, calls:Array }|{ error:string }}
 */
export function analyzeImguiFile(filePath) {
  // Security: restrict to workspace root to avoid reading arbitrary files.
  const abs = filePath.startsWith('/')
    ? filePath
    : resolve(WORKSPACE_ROOT, filePath);
  const realRoot = WORKSPACE_ROOT.replace(/\/$/, '');
  if (!abs.startsWith(realRoot + '/') && abs !== realRoot) {
    return { error: `File must be inside the workspace (${WORKSPACE_ROOT}).` };
  }

  let source;
  try { source = readFileSync(abs, 'utf8'); }
  catch (e) { return { error: `Cannot read file: ${e.message}` }; }

  const lines = source.split('\n');
  const calls = [];
  let   depth  = 0;
  const stack  = []; // Begin call stack for nesting

  // Match ImGui::Identifier(  (handles multiline calls by operating line-by-line)
  const RE = /ImGui::(\w+)\s*\(/g;

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    let m;
    RE.lastIndex = 0;

    while ((m = RE.exec(line)) !== null) {
      const name      = m[1];
      const isBegin   = /^Begin/.test(name);
      const isEnd     = /^End/.test(name);
      const isPush    = /^Push/.test(name);
      const isPop     = /^Pop/.test(name);

      // End/Pop: decrease depth before recording so the call shows at parent level
      if (isEnd || isPop) {
        if (stack.length > 0) stack.pop();
        depth = Math.max(0, depth - 1);
      }

      // Extract first string literal argument ("label") from the same line
      const rest     = line.slice(m.index + m[0].length);
      const labelM   = rest.match(/^"([^"]+)"/);
      const label    = labelM ? labelM[1] : null;

      calls.push({
        line:    i + 1,
        depth,
        call:    `ImGui::${name}`,
        label,
        indent:  '  '.repeat(depth),
        rawLine: line.trim().slice(0, 120),
      });

      // Begin/Push: increase depth after recording
      if (isBegin || isPush) {
        depth++;
        stack.push(label ? `${name}("${label}")` : name);
      }
    }
  }

  return { file: abs, totalCalls: calls.length, calls, stack };
}

// ── analyze_imgui_file ────────────────────────────────────────────────────────

server.tool(
  'analyze_imgui_file',
  'Read a C++ source file inside the workspace and extract every `ImGui::*` call, ' +
  'showing the nested Begin/End hierarchy as a visual tree. ' +
  'Use this to understand what GUI a piece of ImGui code produces — without running it. ' +
  'Works with any .cpp / .h file that uses Dear ImGui.',
  {
    file_path: z.string()
                .describe(
                  'Workspace-relative or absolute path to a C++ file, ' +
                  'e.g. "examples/example_glfw_vulkan/main.cpp" or ' +
                  '"/home/joao/vscode/imgui/imgui_demo.cpp"'),
  },
  ({ file_path }) => {
    const result = analyzeImguiFile(file_path);

    if (result.error) {
      return { content: [{ type: 'text', text: `Error: ${result.error}` }] };
    }

    if (result.totalCalls === 0) {
      return { content: [{ type: 'text', text:
        `No ImGui::* calls found in ${result.file}.`,
      }]};
    }

    const lines = [
      `# ImGui call hierarchy — \`${result.file.split('/').slice(-2).join('/')}\`\n`,
      `${result.totalCalls} ImGui API call(s) found\n`,
      '```',
    ];

    for (const { line, indent, call, label, rawLine } of result.calls) {
      const tag  = label ? `  // "${label}"` : '';
      lines.push(`${String(line).padStart(5)}  ${indent}${call}${tag}`);
    }

    lines.push('```');

    if (result.stack.length > 0) {
      lines.push('');
      lines.push(`**Warning**: ${result.stack.length} Begin/Push call(s) without matching End/Pop:`);
      for (const s of result.stack) lines.push(`  - ${s}`);
    }

    return { content: [{ type: 'text', text: lines.join('\n') }] };
  },
);

// ---------------------------------------------------------------------------
// Entry point — only connect when run directly, not when imported
// ---------------------------------------------------------------------------

const isMain = process.argv[1] === fileURLToPath(import.meta.url);
if (isMain) {
  const transport = new StdioServerTransport();
  await server.connect(transport);
}
