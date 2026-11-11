// dear imgui: text selection extension, tests
// For the Dear ImGui Test Engine (https://github.com/ocornut/imgui_test_engine):
// compile this file into an application using the engine and call RegisterTests_TextSelect(engine).
// The application calls ImGui::TextSelectInstall() before ImGuiTestEngine_Start(): the engine runs a test's GuiFunc
// from its own NewFrame hook, and the extension's hook has to run before it.
//
// The tests act as a user: they move the mouse, press keys and compare what reaches the clipboard. They read no
// internal state, so the sections "selecting" to "widgets" describe behavior another implementation can be checked
// against as they are. The section "extension api" is specific to imgui_text_select.h.
//
// Expected strings assume the default (monospace) font and style: a gap of more than 3 spaces between two texts on
// a line copies as a tab, a smaller one as a space, and a vertical gap of more than a line as an empty line.

#include "imgui.h"
#include "imgui_text_select.h"
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_te_internal.h"   // ImGuiTestInputs
#include <stdio.h>
#include <string.h>

static const char* TEXT_SELECT_TEST_UNCHANGED = "<clipboard unchanged>";

struct TextSelectTestVars
{
    ImVec2  Pos[8];             // Screen position (top-left) of the first glyph of the texts a test aims at
    ImVec2  RectMin, RectMax;
    ImVec2  WindowPos, WindowSize;
    float   ScrollY = 0.0f;
    int     Clicks = 0;
    int     Step = 0;
    bool    Open = false;
    bool    Selected = false;
    bool    HasSelection = false;
    float   Float = 0.5f;
    char    Buf[32] = "editable";
};

#define TEXT_SELECT_TEST_VARS()     TextSelectTestVars& vars = ctx->GetVars<TextSelectTestVars>()

static void TextSelectTestBegin(TextSelectTestVars& vars, float width = 420.0f)
{
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
    ImGui::Begin("Text Select Test", NULL, ImGuiWindowFlags_NoSavedSettings);
    vars.WindowPos = ImGui::GetWindowPos();
    vars.WindowSize = ImGui::GetWindowSize();
}

static void TextSelectTestText(ImVec2* out_pos, const char* text)
{
    if (out_pos != NULL)
        *out_pos = ImGui::GetCursorScreenPos();
    ImGui::TextUnformatted(text);
}

static const char* TEXT_SELECT_TEST_CELLS[3][3] = { { "Apple", "3", "1.25" }, { "Banana", "12", "0.50" }, { "Cherry", "7", "3.00" } };
static const char* TEXT_SELECT_TEST_TABLE_TEXT = "Name\tQty\tPrice\nApple\t3\t1.25\nBanana\t12\t0.50\nCherry\t7\t3.00";

// Pos[row] is the first cell of a row, Pos[4 + row] its last cell
static void TextSelectTestTable(TextSelectTestVars& vars, ImGuiTableFlags flags = 0, bool hide_qty = false)
{
    if (!ImGui::BeginTable("table", 3, ImGuiTableFlags_Borders | flags))
        return;
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Qty");
    ImGui::TableSetupColumn("Price");
    if (hide_qty)
        ImGui::TableSetColumnEnabled(1, false);
    ImGui::TableHeadersRow();
    for (int row = 0; row < 3; row++)
        for (int column = 0; column < 3; column++)
        {
            ImGui::TableNextColumn();
            TextSelectTestText(column == 0 ? &vars.Pos[row] : column == 2 ? &vars.Pos[4 + row] : NULL, TEXT_SELECT_TEST_CELLS[row][column]);
        }
    ImGui::EndTable();
}

static void TextSelectTestRows(ImGuiTestContext* ctx, TextSelectTestVars& vars)
{
    if (ctx->IsFirstGuiFrame())
        ImGui::SetNextWindowScroll(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("rows", ImVec2(200.0f, 120.0f));
    for (int row = 0; row < 50; row++)
    {
        char label[16];
        snprintf(label, sizeof(label), "Row %02d", row);
        TextSelectTestText(row < 8 ? &vars.Pos[row] : NULL, label);
    }
    vars.ScrollY = ImGui::GetScrollY();
    ImGui::EndChild();
    vars.RectMin = ImGui::GetItemRectMin();
    vars.RectMax = ImGui::GetItemRectMax();
    ImGui::Dummy(ImVec2(0.0f, 60.0f));
}

// The point n characters into a single-line text, at mid line height
static ImVec2 TextSelectTestPos(const ImVec2& text_pos, const char* text, int n, float offset_x = 0.0f)
{
    return ImVec2(text_pos.x + ImGui::CalcTextSize(text, text + n).x + offset_x, text_pos.y + ImGui::GetFontSize() * 0.5f);
}

static void TextSelectTestReset(ImGuiTestContext* ctx)
{
    ctx->Yield(2);
    ctx->KeyPress(ImGuiKey_Escape);
}

static void TextSelectTestDrag(ImGuiTestContext* ctx, const ImVec2& from, const ImVec2& to)
{
    ctx->MouseMoveToPos(from);
    ctx->MouseDown(0);
    ctx->MouseMoveToPos(to);
    ctx->MouseUp(0);
}

// Waits first, so that the clicks never continue an earlier click sequence
static void TextSelectTestClicks(ImGuiTestContext* ctx, const ImVec2& pos, int count)
{
    ctx->SleepNoSkip(0.5f, 0.1f);
    ctx->MouseMoveToPos(pos);
    ctx->MouseClickMulti(0, count);
}

// As TextSelectTestClicks(), then drags with the button of the last click held.
// ctx->MouseDown() would start a new click sequence, so the last press is set directly.
static void TextSelectTestClicksAndDrag(ImGuiTestContext* ctx, const ImVec2& pos, int count, const ImVec2& to)
{
    ctx->SleepNoSkip(0.5f, 0.1f);
    ctx->MouseMoveToPos(pos);
    ctx->MouseClickMulti(0, count - 1);
    ctx->Inputs->MouseButtonsValue |= 1;
    ctx->Yield();
    ctx->MouseMoveToPos(to);
    ctx->MouseUp(0);
}

static void TextSelectTestSelectAll(ImGuiTestContext* ctx, const ImVec2& pos)
{
    ctx->MouseMoveToPos(pos);
    ctx->MouseClick(0);
    ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
}

// What the copy shortcut puts on the clipboard, or TEXT_SELECT_TEST_UNCHANGED
static const char* TextSelectTestCopy(ImGuiTestContext* ctx, ImGuiKeyChord key_chord = ImGuiMod_Ctrl | ImGuiKey_C)
{
    ImGui::SetClipboardText(TEXT_SELECT_TEST_UNCHANGED);
    ctx->KeyPress(key_chord);
    ctx->Yield(2);
    return ImGui::GetClipboardText();
}

void RegisterTests_TextSelect(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    //-----------------------------------------------------------------------------------------------------------
    // Selecting
    //-----------------------------------------------------------------------------------------------------------

    // A drag selects the glyphs between its two points, whichever way it goes: the selection is glyph accurate
    // and has no direction. Both copy shortcuts copy it.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_drag");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "Hello brave new world");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        const char* text = "Hello brave new world";
        TextSelectTestReset(ctx);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], text, 6), TextSelectTestPos(vars.Pos[0], text, 15));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "brave new");
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx, ImGuiMod_Ctrl | ImGuiKey_Insert), "brave new");
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], text, 21), TextSelectTestPos(vars.Pos[0], text, 12));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "new world");
    };

    // A selection over several items copies one line per row, top to bottom, whichever way it was dragged.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_drag_lines");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "Line one");
        TextSelectTestText(&vars.Pos[1], "Line two");
        TextSelectTestText(&vars.Pos[2], "Line three");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        const ImVec2 p0 = TextSelectTestPos(vars.Pos[0], "Line one", 5);
        const ImVec2 p1 = TextSelectTestPos(vars.Pos[2], "Line three", 4);
        TextSelectTestDrag(ctx, p0, p1);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "one\nLine two\nLine");
        TextSelectTestDrag(ctx, p1, p0);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "one\nLine two\nLine");
    };

    // A click places a caret and selects nothing (copy must then leave the clipboard alone). Shift+Click selects
    // from the caret, and the caret stays the fixed end for further Shift+Clicks on either side of it.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_shift_click");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "Line one");
        TextSelectTestText(&vars.Pos[1], "Line two");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[0], "Line one", 5));
        ctx->MouseClick(0);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
        ctx->KeyDown(ImGuiMod_Shift);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[1], "Line two", 4));
        ctx->MouseClick(0);
        ctx->KeyUp(ImGuiMod_Shift);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "one\nLine");
        ctx->KeyDown(ImGuiMod_Shift);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[0], "Line one", 8));
        ctx->MouseClick(0);
        ctx->KeyUp(ImGuiMod_Shift);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "one");
        ctx->KeyDown(ImGuiMod_Shift);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[0], "Line one", 0));
        ctx->MouseClick(0);
        ctx->KeyUp(ImGuiMod_Shift);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Line");
    };

    // Double-Click selects a word: a run of word characters or a run of delimiters. Double-Click+drag extends by
    // whole words, Shift+Double-Click extends an existing selection to the end of the word.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_word");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "alpha beta(gamma) delta");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        const char* text = "alpha beta(gamma) delta";
        TextSelectTestReset(ctx);
        TextSelectTestClicks(ctx, TextSelectTestPos(vars.Pos[0], text, 13, 2.0f), 2);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "gamma");
        TextSelectTestClicks(ctx, TextSelectTestPos(vars.Pos[0], text, 10, 2.0f), 2);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "(");
        TextSelectTestClicksAndDrag(ctx, TextSelectTestPos(vars.Pos[0], text, 7, 2.0f), 2, TextSelectTestPos(vars.Pos[0], text, 20));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "beta(gamma) delta");
        TextSelectTestClicks(ctx, TextSelectTestPos(vars.Pos[0], text, 0), 1);
        ctx->KeyDown(ImGuiMod_Shift);
        TextSelectTestClicks(ctx, TextSelectTestPos(vars.Pos[0], text, 13, 2.0f), 2);
        ctx->KeyUp(ImGuiMod_Shift);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "alpha beta(gamma");
    };

    // Triple-Click selects the line: everything on that row of the window, not only the item under the mouse.
    // Triple-Click+drag extends by whole lines.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_line");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(NULL, "Name:");
        ImGui::SameLine();
        TextSelectTestText(&vars.Pos[0], "Dear ImGui");
        TextSelectTestText(&vars.Pos[1], "second line");
        TextSelectTestText(NULL, "third line");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestClicks(ctx, TextSelectTestPos(vars.Pos[0], "Dear ImGui", 2, 2.0f), 3);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Name: Dear ImGui");
        TextSelectTestClicksAndDrag(ctx, TextSelectTestPos(vars.Pos[0], "Dear ImGui", 2, 2.0f), 3, TextSelectTestPos(vars.Pos[1], "second line", 3));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Name: Dear ImGui\nsecond line");
    };

    // Shift+Alt+drag selects a block: the glyphs inside the rectangle between the two points, one line per row.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_block");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "0123456789");
        TextSelectTestText(NULL, "abcdefghij");
        TextSelectTestText(&vars.Pos[1], "ABCDEFGHIJ");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        ctx->KeyDown(ImGuiMod_Shift | ImGuiMod_Alt);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], "0123456789", 2), TextSelectTestPos(vars.Pos[1], "ABCDEFGHIJ", 6));
        ctx->KeyUp(ImGuiMod_Shift | ImGuiMod_Alt);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "2345\ncdef\nCDEF");
    };

    // Three ways to drop a selection: Escape, a click on a widget, a click on empty background away from text.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_clear");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "Some words here");
        if (ImGui::Button("Button"))
            vars.Clicks++;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        const ImVec2 word = TextSelectTestPos(vars.Pos[0], "Some words here", 7);
        TextSelectTestReset(ctx);
        ctx->SetRef("Text Select Test");

        TextSelectTestClicks(ctx, word, 2);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "words");
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);

        TextSelectTestClicks(ctx, word, 2);
        ctx->ItemClick("Button");
        IM_CHECK_EQ(vars.Clicks, 1);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);

        TextSelectTestClicks(ctx, word, 2);
        ctx->MouseMoveToPos(ImVec2(vars.WindowPos.x + vars.WindowSize.x - 20.0f, word.y));
        ctx->MouseClick(0);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
    };

    //-----------------------------------------------------------------------------------------------------------
    // Copied text
    //-----------------------------------------------------------------------------------------------------------

    // Items on one row are joined left to right: by a space when they are a word gap apart, by a tab when the gap
    // is wide (column-like layouts paste into a spreadsheet).
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_same_line");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "key");
        ImGui::SameLine();
        TextSelectTestText(NULL, "value");
        TextSelectTestText(NULL, "left");
        ImGui::SameLine(200.0f);
        TextSelectTestText(NULL, "right");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "key", 1));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "key value\nleft\tright");
    };

    // Word wrap is presentation: a wrapped paragraph copies as one line, only its own '\n' breaks it.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_wrapped");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 160.0f);
        TextSelectTestText(&vars.Pos[0], "The quick brown fox jumps over the lazy dog.\nNew paragraph.");
        ImGui::PopTextWrapPos();
        vars.RectMin = ImGui::GetItemRectMin();
        vars.RectMax = ImGui::GetItemRectMax();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        IM_CHECK_GT(vars.RectMax.y - vars.RectMin.y, ImGui::GetFontSize() * 2.5f);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "The", 1));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "The quick brown fox jumps over the lazy dog.\nNew paragraph.");
    };

    // Vertical space between two rows survives the copy as one empty line.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_empty_line");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "above");
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight() * 2.0f));
        TextSelectTestText(NULL, "below");
        TextSelectTestText(NULL, "last");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "above", 1));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "above\n\nbelow\nlast");
    };

    // Table cells copy as tab-separated values, one line per table row.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_table");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestTable(vars);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], "Apple", 0), TextSelectTestPos(vars.Pos[5], "0.50", 4));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Apple\t3\t1.25\nBanana\t12\t0.50");
    };

    // While an InputText is being edited the copy and select-all shortcuts belong to it, even if a text selection
    // exists elsewhere.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_input_text");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "Some words here");
        if (vars.Step == 1)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("##input", vars.Buf, IM_ARRAYSIZE(vars.Buf));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestClicks(ctx, TextSelectTestPos(vars.Pos[0], "Some words here", 7), 2);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "words");
        vars.Step = 1;
        ctx->Yield(2);
        vars.Step = 0;
        ctx->Yield(2);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "editable");
        ctx->KeyPress(ImGuiKey_Escape);
    };

    //-----------------------------------------------------------------------------------------------------------
    // Containers: a selection stays inside the child window or table it started in
    //-----------------------------------------------------------------------------------------------------------

    // A drag that leaves the child window it started in is clamped to it. A drag started in the parent takes in
    // the children overlapping the rectangle between its two points, merging their rows: with two children side
    // by side, a drag down the left side leaves the right one out.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_container_child");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        const ImVec2 child_size(150.0f, ImGui::GetTextLineHeightWithSpacing() * 3.0f);
        TextSelectTestText(&vars.Pos[0], "Header");
        ImGui::BeginChild("left", child_size);
        TextSelectTestText(&vars.Pos[1], "L1");
        TextSelectTestText(NULL, "L2");
        TextSelectTestText(&vars.Pos[2], "L3");
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("right", child_size);
        TextSelectTestText(&vars.Pos[3], "R1");
        TextSelectTestText(NULL, "R2");
        TextSelectTestText(&vars.Pos[4], "R3");
        ImGui::EndChild();
        TextSelectTestText(&vars.Pos[5], "Footer");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[1], "L1", 0), TextSelectTestPos(vars.Pos[4], "R3", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "L1\nL2\nL3");
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[3], "R1", 0), TextSelectTestPos(vars.Pos[2], "L3", 0));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "R1\nR2");
        const ImVec2 header = TextSelectTestPos(vars.Pos[0], "Header", 0);
        const ImVec2 footer = TextSelectTestPos(vars.Pos[5], "Footer", 6);
        TextSelectTestDrag(ctx, header, footer);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Header\nL1\nL2\nL3\nFooter");
        TextSelectTestDrag(ctx, header, ImVec2(vars.Pos[4].x + 20.0f, footer.y));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Header\nL1\tR1\nL2\tR2\nL3\tR3\nFooter");
    };

    // A drag started in a table stays in the table.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_container_table");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(NULL, "Intro");
        TextSelectTestTable(vars);
        TextSelectTestText(&vars.Pos[3], "Outro");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[1], "Banana", 0), TextSelectTestPos(vars.Pos[3], "Outro", 5));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Banana\t12\t0.50\nCherry\t7\t3.00");
    };

    // Ctrl+A selects the container holding the caret; pressing it again widens: table, then window.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_select_all_table");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(NULL, "Intro");
        TextSelectTestTable(vars);
        TextSelectTestText(NULL, "Outro");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "Apple", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_TABLE_TEXT);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        char expected[256];
        snprintf(expected, sizeof(expected), "Intro\n%s\nOutro", TEXT_SELECT_TEST_TABLE_TEXT);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), expected);
    };

    // Same widening from a child window to its parent.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_select_all_child");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(NULL, "parent text");
        ImGui::BeginChild("child", ImVec2(200.0f, ImGui::GetTextLineHeightWithSpacing() * 2.0f));
        TextSelectTestText(&vars.Pos[0], "child one");
        TextSelectTestText(NULL, "child two");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "child one", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "child one\nchild two");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "parent text\nchild one\nchild two");
    };

    // Holding a drag below a scrolling container scrolls it and keeps extending the selection, so text that was
    // out of view when the drag started can be reached.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_auto_scroll");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestRows(ctx, vars);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        IM_CHECK_EQ(vars.ScrollY, 0.0f);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[1], "Row 01", 0));
        ctx->MouseDown(0);
        ctx->MouseMoveToPos(ImVec2(vars.RectMin.x + 100.0f, vars.RectMax.y + 40.0f));
        ctx->SleepNoSkip(1.0f, 1.0f / 60.0f);
        ctx->MouseUp(0);
        IM_CHECK_GT(vars.ScrollY, 0.0f);
        const char* copied = TextSelectTestCopy(ctx);
        IM_CHECK(strncmp(copied, "Row 01\nRow 02\n", 14) == 0);
        IM_CHECK(strstr(copied, "\nRow 15\n") != NULL);
    };

    // A selection is attached to the text, not to the screen: scrolling it out of view changes nothing.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_scrolled_away");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestRows(ctx, vars);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[2], "Row 02", 0), TextSelectTestPos(vars.Pos[4], "Row 04", 6));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Row 02\nRow 03\nRow 04");
        ctx->MouseWheelY(-10.0f);
        ctx->Yield(2);
        IM_CHECK_GT(vars.ScrollY, ImGui::GetTextLineHeightWithSpacing() * 6.0f);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Row 02\nRow 03\nRow 04");
    };

    // Copy includes rows the clipper did not submit: Ctrl+A, Ctrl+C on a 1000 row table yields 1000 rows, with
    // the frozen header row first.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_clipped_rows");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        if (ImGui::BeginTable("big", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 150.0f)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin(1000);
            while (clipper.Step())
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if (row == 1)
                        vars.Pos[0] = ImGui::GetCursorScreenPos();
                    ImGui::Text("Row %04d", row);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", row * 3);
                }
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "Row 0001", 2));
        const char* copied = TextSelectTestCopy(ctx);
        ImGuiTextBuffer expected;
        expected.append("ID\tValue");
        for (int row = 0; row < 1000; row++)
            expected.appendf("\nRow %04d\t%d", row, row * 3);
        int lines = 1;
        for (const char* p = copied; *p; p++)
            lines += (*p == '\n');
        IM_CHECK_EQ(lines, 1001);
        IM_CHECK(strncmp(copied, "ID\tValue\nRow 0000\t0\n", 20) == 0);
        IM_CHECK(strcmp(copied, expected.c_str()) == 0);
    };

    // Same horizontally: text wider than its container copies whole, not cut at the container's edges.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_clipped_x");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        if (ctx->IsFirstGuiFrame())
            ImGui::SetNextWindowScroll(ImVec2(0.0f, 0.0f));
        ImGui::BeginChild("wide", ImVec2(150.0f, ImGui::GetTextLineHeightWithSpacing() * 3.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
        TextSelectTestText(&vars.Pos[0], "0123456789 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "0123456789", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "0123456789 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    };

    // A column the user has hidden is not copied, even though the application still submits its cells.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_copy_hidden_column");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestTable(vars, ImGuiTableFlags_Hideable, true);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "Apple", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Name\tPrice\nApple\t1.25\nBanana\t0.50\nCherry\t3.00");
    };

    // The window title is not part of the window's text, and the title bar keeps moving the window.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_title_bar");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "body");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "body", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "body");
        const ImVec2 window_pos = vars.WindowPos;
        const ImVec2 title(window_pos.x + 60.0f, window_pos.y + ImGui::GetFrameHeight() * 0.5f);
        TextSelectTestDrag(ctx, title, ImVec2(title.x + 30.0f, title.y + 20.0f));
        IM_CHECK_EQ(vars.WindowPos.x, window_pos.x + 30.0f);
        IM_CHECK_EQ(vars.WindowPos.y, window_pos.y + 20.0f);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
    };

    //-----------------------------------------------------------------------------------------------------------
    // Widgets: selection must not take anything away from them
    //-----------------------------------------------------------------------------------------------------------

    // A click or a drag on a button or a slider is theirs: the button clicks, the slider edits, nothing is selected.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_widgets_keep_mouse");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        vars.Pos[0] = ImVec2(ImGui::GetCursorScreenPos().x + padding.x, ImGui::GetCursorScreenPos().y + padding.y);
        if (ImGui::Button("Press me"))
            vars.Clicks++;
        ImGui::SliderFloat("##slider", &vars.Float, 0.0f, 1.0f);
        vars.RectMin = ImGui::GetItemRectMin();
        vars.RectMax = ImGui::GetItemRectMax();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[0], "Press me", 3));
        ctx->MouseClick(0);
        IM_CHECK_EQ(vars.Clicks, 1);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], "Press me", 0, 1.0f), TextSelectTestPos(vars.Pos[0], "Press me", 8));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
        const ImVec2 slider((vars.RectMin.x + vars.RectMax.x) * 0.5f, (vars.RectMin.y + vars.RectMax.y) * 0.5f);
        TextSelectTestDrag(ctx, slider, ImVec2(slider.x + 60.0f, slider.y));
        IM_CHECK_GT(vars.Float, 0.5f);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
    };

    // Alt+drag selects the text of any widget without activating it.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_widgets_alt_drag");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        vars.Pos[0] = ImVec2(ImGui::GetCursorScreenPos().x + padding.x, ImGui::GetCursorScreenPos().y + padding.y);
        if (ImGui::Button("Press me"))
            vars.Clicks++;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        ctx->KeyDown(ImGuiMod_Alt);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], "Press me", 0), TextSelectTestPos(vars.Pos[0], "Press me", 5));
        ctx->KeyUp(ImGuiMod_Alt);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Press");
        IM_CHECK_EQ(vars.Clicks, 0);
    };

    // A tree node or a row-spanning Selectable is mostly label: a click still toggles it, a drag along its label
    // selects text instead and does not toggle it.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_widgets_drag_label");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        vars.Pos[0] = ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetTreeNodeToLabelSpacing(), ImGui::GetCursorScreenPos().y);
        vars.Open = ImGui::TreeNode("Tree node label");
        if (vars.Open)
        {
            ImGui::TextUnformatted("child item");
            ImGui::TreePop();
        }
        if (ImGui::BeginTable("selectables", 2))
        {
            ImGui::TableNextColumn();
            vars.Pos[1] = ImGui::GetCursorScreenPos();
            ImGui::Selectable("Row label", &vars.Selected, ImGuiSelectableFlags_SpanAllColumns);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("detail");
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[0], "Tree node label", 2));
        ctx->MouseClick(0);
        IM_CHECK_EQ(vars.Open, true);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], "Tree node label", 5), TextSelectTestPos(vars.Pos[0], "Tree node label", 9));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "node");
        IM_CHECK_EQ(vars.Open, true);

        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[1], "Row label", 2));
        ctx->MouseClick(0);
        IM_CHECK_EQ(vars.Selected, true);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[1], "Row label", 0, 1.0f), TextSelectTestPos(vars.Pos[1], "Row label", 3));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Row");
        IM_CHECK_EQ(vars.Selected, true);
    };

    //-----------------------------------------------------------------------------------------------------------
    // Extension API (imgui_text_select.h)
    //-----------------------------------------------------------------------------------------------------------

    // ImGuiItemFlags_NoTextSelect: the text is never copied and a drag on it selects nothing.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_no_text_select");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "visible A");
        TextSelectTestText(NULL, "visible B");
        ImGui::PushItemFlag(ImGuiItemFlags_NoTextSelect, true);
        TextSelectTestText(&vars.Pos[1], "secret");
        ImGui::PopItemFlag();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "visible A", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "visible A\nvisible B");
        ctx->KeyPress(ImGuiKey_Escape);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[1], "secret", 0, 1.0f), TextSelectTestPos(vars.Pos[1], "secret", 6));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
    };

    // TextSelectPushDisabledRegion(): as NoTextSelect for an area, and the copy shortcut does nothing while the
    // mouse is over it (the area can then handle the shortcut itself).
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_disabled_region");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "visible A");
        ImGui::TextSelectPushDisabledRegion();
        TextSelectTestText(&vars.Pos[1], "status: ready");
        ImGui::TextSelectPopDisabledRegion();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestSelectAll(ctx, TextSelectTestPos(vars.Pos[0], "visible A", 2));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "visible A");
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[1], "status: ready", 3));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
    };

    // TextSelectSetNextCopyText(): a text drawn shortened copies as its full value, whatever part is selected.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_next_copy_text");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        if (ImGui::TextSelectIsCopying())
            ImGui::TextSelectSetNextCopyText("a_very_long_file_name.txt");
        TextSelectTestText(&vars.Pos[0], "a_very_lo...");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestDrag(ctx, TextSelectTestPos(vars.Pos[0], "a_very_lo...", 2), TextSelectTestPos(vars.Pos[0], "a_very_lo...", 6));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "a_very_long_file_name.txt");
    };

    // TextSelectAddText(): a rectangle drawn without AddText() behaves as one unit of text. A drag ending inside
    // it takes all of it or none, whichever edge is nearer.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_add_text");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "Time:");
        ImGui::SameLine();
        const ImVec2 size = ImGui::CalcTextSize("00:00:00");
        vars.RectMin = ImGui::GetCursorScreenPos();
        vars.RectMax = ImVec2(vars.RectMin.x + size.x, vars.RectMin.y + size.y);
        ImGui::Dummy(size);
        ImGui::TextSelectAddText(vars.RectMin, vars.RectMax, "12:34:56");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        const ImVec2 start = TextSelectTestPos(vars.Pos[0], "Time:", 0);
        const float width = vars.RectMax.x - vars.RectMin.x;
        TextSelectTestDrag(ctx, start, ImVec2(vars.RectMin.x + width * 0.3f, start.y));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Time:");
        TextSelectTestDrag(ctx, start, ImVec2(vars.RectMin.x + width * 0.7f, start.y));
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "Time: 12:34:56");
    };

    // TextSelectSelectAndCopy(): one call (here from a button) selects a rectangle and copies it, optionally
    // putting another text on the clipboard.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_select_and_copy");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(NULL, "order-42");
        const ImVec2 p_min = ImGui::GetItemRectMin(), p_max = ImGui::GetItemRectMax();
        if (ImGui::Button("Copy id"))
            ImGui::TextSelectSelectAndCopy(p_min, p_max);
        if (ImGui::Button("Copy link"))
            ImGui::TextSelectSelectAndCopy(p_min, p_max, "https://example.com/order-42");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TextSelectTestReset(ctx);
        ctx->SetRef("Text Select Test");
        ImGui::SetClipboardText(TEXT_SELECT_TEST_UNCHANGED);
        ctx->ItemClick("Copy id");
        ctx->Yield(3);
        IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "order-42");
        ctx->ItemClick("Copy link");
        ctx->Yield(3);
        IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "https://example.com/order-42");
    };

    // TextSelectSetWordDelimiters(): which characters end a word for Double-Click.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_word_delimiters");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "path/to/file.txt");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        const ImVec2 word = TextSelectTestPos(vars.Pos[0], "path/to/file.txt", 6);
        TextSelectTestReset(ctx);
        TextSelectTestClicks(ctx, word, 2);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "path/to/file.txt");
        ImGui::TextSelectSetWordDelimiters("/.");
        TextSelectTestClicks(ctx, word, 2);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "to");
    };
    t->TeardownFunc = [](ImGuiTestContext*) { ImGui::TextSelectSetWordDelimiters(NULL); };

    // TextSelectSetCopyShortcut(): the copy keys are the application's choice.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_copy_shortcut");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "Some words here");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        TextSelectTestClicks(ctx, TextSelectTestPos(vars.Pos[0], "Some words here", 7), 2);
        ImGui::TextSelectSetCopyShortcut(ImGuiMod_Ctrl | ImGuiKey_K);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), TEXT_SELECT_TEST_UNCHANGED);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx, ImGuiMod_Ctrl | ImGuiKey_K), "words");
    };
    t->TeardownFunc = [](ImGuiTestContext*) { ImGui::TextSelectSetCopyShortcut(ImGuiMod_Ctrl | ImGuiKey_C, ImGuiMod_Ctrl | ImGuiKey_Insert); };

    // TextSelectSelectAll(), TextSelectHasSelection(), TextSelectClear(): the selection driven from code, e.g. from
    // a menu.
    t = IM_REGISTER_TEST(e, "text_select", "text_select_api_select_all");
    t->SetVarsDataType<TextSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestBegin(vars);
        TextSelectTestText(&vars.Pos[0], "one");
        TextSelectTestText(NULL, "two");
        if (vars.Step == 1)
            ImGui::TextSelectSelectAll();
        if (vars.Step == 2)
            ImGui::TextSelectClear();
        vars.HasSelection = ImGui::TextSelectHasSelection();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TEXT_SELECT_TEST_VARS();
        TextSelectTestReset(ctx);
        ctx->MouseMoveToPos(TextSelectTestPos(vars.Pos[0], "one", 1));
        IM_CHECK_EQ(vars.HasSelection, false);
        vars.Step = 1;
        ctx->Yield(2);
        vars.Step = 0;
        ctx->Yield(2);
        IM_CHECK_EQ(vars.HasSelection, true);
        IM_CHECK_STR_EQ(TextSelectTestCopy(ctx), "one\ntwo");
        vars.Step = 2;
        ctx->Yield(2);
        vars.Step = 0;
        ctx->Yield(2);
        IM_CHECK_EQ(vars.HasSelection, false);
    };
}
