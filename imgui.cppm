// dear imgui, v1.92.6 WIP
// (modules)

module;

#include "imgui.h"

export module ImGui;

export {
    // Scalar data types
    using ::ImGuiID;
    using ::ImS8;
    using ::ImU8;
    using ::ImS16;
    using ::ImU16;
    using ::ImS32;
    using ::ImU32;
    using ::ImS64;
    using ::ImU64;

    // Forward declarations: ImDrawList, ImFontAtlas layer
    using ::ImDrawChannel;
    using ::ImDrawCmd;
    using ::ImDrawData;
    using ::ImDrawList;
    using ::ImDrawListSharedData;
    using ::ImDrawListSplitter;
    using ::ImDrawVert;
    using ::ImFont;
    using ::ImFontAtlas;
    using ::ImFontAtlasBuilder;
    using ::ImFontAtlasRect;
    using ::ImFontBaked;
    using ::ImFontConfig;
    using ::ImFontGlyph;
    using ::ImFontGlyphRangesBuilder;
    using ::ImFontLoader;
    using ::ImTextureData;
    using ::ImTextureRect;
    using ::ImColor;

    // Forward declarations: ImGui layer
    using ::ImGuiContext;
    using ::ImGuiIO;
    using ::ImGuiInputTextCallbackData;
    using ::ImGuiKeyData;
    using ::ImGuiListClipper;
    using ::ImGuiMultiSelectIO;
    using ::ImGuiOnceUponAFrame;
    using ::ImGuiPayload;
    using ::ImGuiPlatformIO;
    using ::ImGuiPlatformImeData;
    using ::ImGuiSelectionBasicStorage;
    using ::ImGuiSelectionExternalStorage;
    using ::ImGuiSelectionRequest;
    using ::ImGuiSizeCallbackData;
    using ::ImGuiStorage;
    using ::ImGuiStoragePair;
    using ::ImGuiStyle;
    using ::ImGuiTableSortSpecs;
    using ::ImGuiTableColumnSortSpecs;
    using ::ImGuiTextBuffer;
    using ::ImGuiTextFilter;
    using ::ImGuiViewport;

    // Enumerations
    using ::ImGuiDir;
    using ::ImGuiKey;
    using ::ImGuiMouseSource;
    using ::ImGuiSortDirection;
    using ::ImGuiCol;
    using ::ImGuiCond;
    using ::ImGuiDataType;
    using ::ImGuiMouseButton;
    using ::ImGuiMouseCursor;
    using ::ImGuiStyleVar;
    using ::ImGuiTableBgTarget;

    // Flags
    using ::ImDrawFlags;
    using ::ImDrawListFlags;
    using ::ImDrawTextFlags;
    using ::ImFontFlags;
    using ::ImFontAtlasFlags;
    using ::ImGuiBackendFlags;
    using ::ImGuiButtonFlags;
    using ::ImGuiChildFlags;
    using ::ImGuiColorEditFlags;
    using ::ImGuiConfigFlags;
    using ::ImGuiComboFlags;
    using ::ImGuiDragDropFlags;
    using ::ImGuiFocusedFlags;
    using ::ImGuiHoveredFlags;
    using ::ImGuiInputFlags;
    using ::ImGuiInputTextFlags;
    using ::ImGuiInputTextFlags;
    using ::ImGuiItemFlags;
    using ::ImGuiKeyChord;
    using ::ImGuiListClipperFlags;
    using ::ImGuiPopupFlags;
    using ::ImGuiMultiSelectFlags;
    using ::ImGuiSelectableFlags;
    using ::ImGuiSliderFlags;
    using ::ImGuiTabBarFlags;
    using ::ImGuiTabItemFlags;
    using ::ImGuiTableFlags;
    using ::ImGuiTableColumnFlags;
    using ::ImGuiTableRowFlags;
    using ::ImGuiViewportFlags;
    using ::ImGuiWindowFlags;

    // Character types
    using ::ImWchar32;
    using ::ImWchar16;
    using ::ImWchar;

    // Multi-Selection item index or identifier when using BeginMultiSelect()
    using ::ImGuiSelectionUserData;

    // Callback and functions types
    using ::ImGuiInputTextCallback;
    using ::ImGuiSizeCallback;
    using ::ImGuiMemAllocFunc;
    using ::ImGuiMemFreeFunc;

    // ImVec2
    using ::ImVec2;

    // ImVec4
    using ::ImVec4;

    // ImTextureID
    #ifndef ImTextureID
    using ::ImTextureID;
    #endif

    // ImTextureRef
    using ::ImTextureRef;

    // IM_MALLOC(), IM_FREE(), IM_NEW(), IM_PLACEMENT_NEW(), IM_DELETE()
    using ::ImNewWrapper;
    using ::operator new;
    using ::operator delete;
    constexpr void* IM_ALLOC(size_t _SIZE) {
        return ImGui::MemAlloc(_SIZE);
    }
    constexpr void IM_FREE(void* _PTR) {
        ImGui::MemFree(_PTR);
    }
    using ::IM_DELETE;
    using ::ImVector;
}

export namespace ImGui {
    // Context creation and access
    using ImGui::CreateContext;
    using ImGui::DestroyContext;
    using ImGui::GetCurrentContext;
    using ImGui::SetCurrentContext;

    // Main
    using ImGui::GetIO;
    using ImGui::GetPlatformIO;
    using ImGui::GetStyle;
    using ImGui::NewFrame;
    using ImGui::EndFrame;
    using ImGui::Render;
    using ImGui::GetDrawData;

    // Demo, Debug, Information
    using ImGui::ShowDemoWindow;
    using ImGui::ShowMetricsWindow;
    using ImGui::ShowDebugLogWindow;
    using ImGui::ShowIDStackToolWindow;
    using ImGui::ShowAboutWindow;
    using ImGui::ShowStyleEditor;
    using ImGui::ShowStyleSelector;
    using ImGui::ShowFontSelector;
    using ImGui::ShowUserGuide;
    using ImGui::GetVersion;

    // Styles
    using ImGui::StyleColorsDark;
    using ImGui::StyleColorsLight;
    using ImGui::StyleColorsClassic;

    // Windows
    using ImGui::Begin;
    using ImGui::End;

    // Child Windows
    using ImGui::BeginChild;
    using ImGui::EndChild;

    // Windows Utilities
    using ImGui::IsWindowAppearing;
    using ImGui::IsWindowCollapsed;
    using ImGui::IsWindowFocused;
    using ImGui::IsWindowHovered;
    using ImGui::GetWindowDrawList;
    using ImGui::GetWindowPos;
    using ImGui::GetWindowSize;
    using ImGui::GetWindowWidth;
    using ImGui::GetWindowHeight;

    // Window manipulation
    using ImGui::SetNextWindowPos;
    using ImGui::SetNextWindowSize;
    using ImGui::SetNextWindowSizeConstraints;
    using ImGui::SetNextWindowContentSize;
    using ImGui::SetNextWindowCollapsed;
    using ImGui::SetNextWindowFocus;
    using ImGui::SetNextWindowScroll;
    using ImGui::SetNextWindowBgAlpha;
    using ImGui::SetWindowPos;
    using ImGui::SetWindowSize;
    using ImGui::SetWindowCollapsed;
    using ImGui::SetWindowFocus;

    // Windows Scrolling
    using ImGui::GetScrollX;
    using ImGui::GetScrollY;
    using ImGui::SetScrollX;
    using ImGui::SetScrollY;
    using ImGui::GetScrollMaxX;
    using ImGui::GetScrollMaxY;
    using ImGui::SetScrollHereX;
    using ImGui::SetScrollHereY;
    using ImGui::SetScrollFromPosX;
    using ImGui::SetScrollFromPosY;

    // Parameters stacks (font)
    using ImGui::PushFont;
    using ImGui::PopFont;
    using ImGui::GetFont;
    using ImGui::GetFontSize;
    using ImGui::GetFontBaked;

    // Parameters stacks (shared)
    using ImGui::PushStyleColor;
    using ImGui::PopStyleColor;
    using ImGui::PushStyleVar;
    using ImGui::PushStyleVarX;
    using ImGui::PushStyleVarY;
    using ImGui::PopStyleVar;
    using ImGui::PushItemFlag;
    using ImGui::PopItemFlag;

    // Parameters stacks (current window)
    using ImGui::PushItemWidth;
    using ImGui::PopItemWidth;
    using ImGui::SetNextItemWidth;
    using ImGui::CalcItemWidth;
    using ImGui::PushTextWrapPos;
    using ImGui::PopTextWrapPos;

    // Style read access
    using ImGui::GetFontTexUvWhitePixel;
    using ImGui::GetColorU32;
    using ImGui::GetStyleColorVec4;

    // Layout cursor positioning
    using ImGui::GetCursorScreenPos;
    using ImGui::SetCursorScreenPos;
    using ImGui::GetContentRegionAvail;
    using ImGui::GetCursorPos;
    using ImGui::GetCursorPosX;
    using ImGui::GetCursorPosY;
    using ImGui::SetCursorPos;
    using ImGui::SetCursorPosX;
    using ImGui::SetCursorPosY;
    using ImGui::GetCursorStartPos;

    // Other layout functions
    using ImGui::Separator;
    using ImGui::SameLine;
    using ImGui::NewLine;
    using ImGui::Spacing;
    using ImGui::Dummy;
    using ImGui::Indent;
    using ImGui::Unindent;
    using ImGui::BeginGroup;
    using ImGui::EndGroup;
    using ImGui::AlignTextToFramePadding;
    using ImGui::GetTextLineHeight;
    using ImGui::GetTextLineHeightWithSpacing;
    using ImGui::GetFrameHeight;
    using ImGui::GetFrameHeightWithSpacing;

    // ID stack/scopes
    using ImGui::PushID;
    using ImGui::PopID;
    using ImGui::GetID;

    // Widgets: Text
    using ImGui::TextUnformatted;
    using ImGui::Text;
    using ImGui::TextV;
    using ImGui::TextColored;
    using ImGui::TextColoredV;
    using ImGui::TextDisabled;
    using ImGui::TextDisabledV;
    using ImGui::TextWrapped;
    using ImGui::TextWrappedV;
    using ImGui::LabelText;
    using ImGui::LabelTextV;
    using ImGui::BulletText;
    using ImGui::BulletTextV;
    using ImGui::SeparatorText;

    // Widgets: Main
    using ImGui::Button;
    using ImGui::SmallButton;
    using ImGui::InvisibleButton;
    using ImGui::ArrowButton;
    using ImGui::Checkbox;
    using ImGui::CheckboxFlags;
    using ImGui::RadioButton;
    using ImGui::ProgressBar;
    using ImGui::Bullet;
    using ImGui::TextLink;
    using ImGui::TextLinkOpenURL;

    // Widgets: Images
    using ImGui::Image;
    using ImGui::ImageWithBg;
    using ImGui::ImageButton;

    // Widgets: Combo Box (Dropdown)
    using ImGui::BeginCombo;
    using ImGui::EndCombo;
    using ImGui::Combo;

    // Widgets: Drag Sliders
    using ImGui::DragFloat;
    using ImGui::DragFloat2;
    using ImGui::DragFloat3;
    using ImGui::DragFloat4;
    using ImGui::DragFloatRange2;
    using ImGui::DragInt;
    using ImGui::DragInt2;
    using ImGui::DragInt3;
    using ImGui::DragInt4;
    using ImGui::DragIntRange2;
    using ImGui::DragScalar;
    using ImGui::DragScalarN;

    // Widgets: Regular Sliders
    using ImGui::SliderFloat;
    using ImGui::SliderFloat2;
    using ImGui::SliderFloat3;
    using ImGui::SliderFloat4;
    using ImGui::SliderAngle;
    using ImGui::SliderInt;
    using ImGui::SliderInt2;
    using ImGui::SliderInt3;
    using ImGui::SliderInt4;
    using ImGui::SliderScalar;
    using ImGui::SliderScalarN;
    using ImGui::VSliderFloat;
    using ImGui::VSliderInt;
    using ImGui::VSliderScalar;

    // Widgets: Input with Keyboard
    using ImGui::InputText;
    using ImGui::InputTextMultiline;
    using ImGui::InputTextWithHint;
    using ImGui::InputFloat;
    using ImGui::InputFloat2;
    using ImGui::InputFloat3;
    using ImGui::InputFloat4;
    using ImGui::InputInt;
    using ImGui::InputInt2;
    using ImGui::InputInt3;
    using ImGui::InputInt4;
    using ImGui::InputDouble;
    using ImGui::InputScalar;
    using ImGui::InputScalarN;

    // Widgets: Color Editor/Picker
    using ImGui::ColorEdit3;
    using ImGui::ColorEdit4;
    using ImGui::ColorPicker3;
    using ImGui::ColorPicker4;
    using ImGui::ColorButton;
    using ImGui::SetColorEditOptions;

    // Widgets: Trees
    using ImGui::TreeNode;
    using ImGui::TreeNodeV;
    using ImGui::TreeNodeEx;
    using ImGui::TreeNodeExV;
    using ImGui::TreePush;
    using ImGui::TreePop;
    using ImGui::GetTreeNodeToLabelSpacing;
    using ImGui::CollapsingHeader;
    using ImGui::SetNextItemOpen;
    using ImGui::SetNextItemStorageID;

    // Widgets: Selectables
    using ImGui::Selectable;

    // Multi-selection system for Selectable(), Checkbox(), TreeNode() functions [BETA]
    using ImGui::BeginMultiSelect;
    using ImGui::EndMultiSelect;
    using ImGui::SetNextItemSelectionUserData;
    using ImGui::IsItemToggledSelection;

    // Widgets: List Boxes
    using ImGui::BeginListBox;
    using ImGui::EndListBox;
    using ImGui::ListBox;

    // Widgets: Data Plotting
    using ImGui::PlotLines;
    using ImGui::PlotHistogram;

    // Widgets: Value() Helpers.
    using ImGui::Value;

    // Widgets: Menus
    using ImGui::BeginMenuBar;
    using ImGui::EndMenuBar;
    using ImGui::BeginMainMenuBar;
    using ImGui::EndMainMenuBar;
    using ImGui::BeginMenu;
    using ImGui::EndMenu;
    using ImGui::MenuItem;

    // Tooltips
    using ImGui::BeginTooltip;
    using ImGui::EndTooltip;
    using ImGui::SetTooltip;
    using ImGui::SetTooltipV;

    // Tooltips: helpers for showing a tooltip when hovering an item
    using ImGui::BeginItemTooltip;
    using ImGui::SetItemTooltip;
    using ImGui::SetItemTooltipV;

    // Popups, Modals
    using ImGui::BeginPopup;
    using ImGui::BeginPopupModal;
    using ImGui::EndPopup;

    // Popups: open/close functions
    using ImGui::OpenPopup;
    using ImGui::OpenPopupOnItemClick;
    using ImGui::CloseCurrentPopup;

    // Popups: open+begin combined functions helpers
    using ImGui::BeginPopupContextItem;
    using ImGui::BeginPopupContextWindow;
    using ImGui::BeginPopupContextVoid;

    // Popups: query functions
    using ImGui::IsPopupOpen;

    // Tables
    using ImGui::BeginTable;
    using ImGui::EndTable;
    using ImGui::TableNextRow;
    using ImGui::TableNextColumn;
    using ImGui::TableSetColumnIndex;

    // Tables: Headers & Columns declaration
    using ImGui::TableSetupColumn;
    using ImGui::TableSetupScrollFreeze;
    using ImGui::TableHeader;
    using ImGui::TableHeadersRow;
    using ImGui::TableAngledHeadersRow;

    // Tables: Sorting & Miscellaneous functions
    using ImGui::TableGetSortSpecs;
    using ImGui::TableGetColumnCount;
    using ImGui::TableGetColumnIndex;
    using ImGui::TableGetRowIndex;
    using ImGui::TableGetColumnName;
    using ImGui::TableGetColumnFlags;
    using ImGui::TableSetColumnEnabled;
    using ImGui::TableGetHoveredColumn;
    using ImGui::TableSetBgColor;

    // Legacy Columns API
    using ImGui::Columns;
    using ImGui::NextColumn;
    using ImGui::GetColumnIndex;
    using ImGui::GetColumnWidth;
    using ImGui::SetColumnWidth;
    using ImGui::GetColumnOffset;
    using ImGui::SetColumnOffset;
    using ImGui::GetColumnsCount;

    // Tab Bars, Tabs
    using ImGui::BeginTabBar;
    using ImGui::EndTabBar;
    using ImGui::BeginTabItem;
    using ImGui::EndTabItem;
    using ImGui::TabItemButton;
    using ImGui::SetTabItemClosed;

    // Logging/Capture
    using ImGui::LogToTTY;
    using ImGui::LogToFile;
    using ImGui::LogToClipboard;
    using ImGui::LogFinish;
    using ImGui::LogButtons;
    using ImGui::LogText;
    using ImGui::LogTextV;

    // Drag and Drop
    using ImGui::BeginDragDropSource;
    using ImGui::SetDragDropPayload;
    using ImGui::EndDragDropSource;
    using ImGui::BeginDragDropTarget;
    using ImGui::AcceptDragDropPayload;
    using ImGui::EndDragDropTarget;
    using ImGui::GetDragDropPayload;

    // Disabling [BETA API]
    using ImGui::BeginDisabled;
    using ImGui::EndDisabled;

    // Clipping
    using ImGui::PushClipRect;
    using ImGui::PopClipRect;

    // Focus, Activation
    using ImGui::SetItemDefaultFocus;
    using ImGui::SetKeyboardFocusHere;

    // Keyboard/Gamepad Navigation
    using ImGui::SetNavCursorVisible;

    // Overlapping mode
    using ImGui::SetNextItemAllowOverlap;

    // Item/Widgets Utilities and Query Functions
    using ImGui::IsItemHovered;
    using ImGui::IsItemActive;
    using ImGui::IsItemFocused;
    using ImGui::IsItemClicked;
    using ImGui::IsItemVisible;
    using ImGui::IsItemEdited;
    using ImGui::IsItemActivated;
    using ImGui::IsItemDeactivated;
    using ImGui::IsItemDeactivatedAfterEdit;
    using ImGui::IsItemToggledOpen;
    using ImGui::IsAnyItemHovered;
    using ImGui::IsAnyItemActive;
    using ImGui::IsAnyItemFocused;
    using ImGui::GetItemID;
    using ImGui::GetItemRectMin;
    using ImGui::GetItemRectMax;
    using ImGui::GetItemRectSize;

    // Viewports
    using ImGui::GetMainViewport;

    // Background/Foreground Draw Lists
    using ImGui::GetBackgroundDrawList;
    using ImGui::GetForegroundDrawList;

    // Miscellaneous Utilities
    using ImGui::IsRectVisible;
    using ImGui::GetTime;
    using ImGui::GetFrameCount;
    using ImGui::GetDrawListSharedData;
    using ImGui::GetStyleColorName;
    using ImGui::SetStateStorage;
    using ImGui::GetStateStorage;

    // Text Utilities
    using ImGui::CalcTextSize;

    // Color Utilities
    using ImGui::ColorConvertU32ToFloat4;
    using ImGui::ColorConvertFloat4ToU32;
    using ImGui::ColorConvertRGBtoHSV;
    using ImGui::ColorConvertHSVtoRGB;

    // Inputs Utilities: Keyboard/Mouse/Gamepad
    using ImGui::IsKeyDown;
    using ImGui::IsKeyPressed;
    using ImGui::IsKeyReleased;
    using ImGui::IsKeyChordPressed;
    using ImGui::GetKeyPressedAmount;
    using ImGui::GetKeyName;
    using ImGui::SetNextFrameWantCaptureKeyboard;

    // Inputs Utilities: Shortcut Testing & Routing [BETA]
    using ImGui::Shortcut;
    using ImGui::SetNextItemShortcut;

    // Inputs Utilities: Key/Input Ownership [BETA]
    using ImGui::SetItemKeyOwner;

    // Inputs Utilities: Mouse
    using ImGui::IsMouseDown;
    using ImGui::IsMouseClicked;
    using ImGui::IsMouseReleased;
    using ImGui::IsMouseDoubleClicked;
    using ImGui::IsMouseReleasedWithDelay;
    using ImGui::GetMouseClickedCount;
    using ImGui::IsMouseHoveringRect;
    using ImGui::IsMousePosValid;
    using ImGui::IsAnyMouseDown;
    using ImGui::GetMousePos;
    using ImGui::GetMousePosOnOpeningCurrentPopup;
    using ImGui::IsMouseDragging;
    using ImGui::GetMouseDragDelta;
    using ImGui::ResetMouseDragDelta;
    using ImGui::GetMouseCursor;
    using ImGui::SetMouseCursor;
    using ImGui::SetNextFrameWantCaptureMouse;

    // Clipboard Utilities
    using ImGui::GetClipboardText;
    using ImGui::SetClipboardText;

    // Settings/.Ini Utilities
    using ImGui::LoadIniSettingsFromDisk;
    using ImGui::LoadIniSettingsFromMemory;
    using ImGui::SaveIniSettingsToDisk;
    using ImGui::SaveIniSettingsToMemory;

    // Debug Utilities
    using ImGui::DebugTextEncoding;
    using ImGui::DebugFlashStyleColor;
    using ImGui::DebugStartItemPicker;
    using ImGui::DebugCheckVersionAndDataLayout;
    #ifndef IMGUI_DISABLE_DEBUG_TOOLS
    using ImGui::DebugLog;
    using ImGui::DebugLogV;
    #endif

    // Memory Allocators
    using ImGui::SetAllocatorFunctions;
    using ImGui::GetAllocatorFunctions;
    using ImGui::MemAlloc;
    using ImGui::MemFree;
}

export {
    using ::ImGuiWindowFlags_;
    using ::ImGuiChildFlags_;
    using ::ImGuiItemFlags_;
    using ::ImGuiInputTextFlags_;
    using ::ImGuiTreeNodeFlags_;
    using ::ImGuiPopupFlags_;
    using ::ImGuiSelectableFlags_;
    using ::ImGuiComboFlags_;
    using ::ImGuiTabBarFlags_;
    using ::ImGuiTabItemFlags_;
    using ::ImGuiFocusedFlags_;
    using ::ImGuiHoveredFlags_;
    using ::ImGuiDragDropFlags_;
    using ::ImGuiDataType_;
    using ::ImGuiInputFlags_;
    using ::ImGuiConfigFlags_;
    using ::ImGuiBackendFlags_;
    using ::ImGuiCol_;
    using ::ImGuiStyleVar_;
    using ::ImGuiButtonFlags_;
    using ::ImGuiColorEditFlags_;
    using ::ImGuiSliderFlags_;
    using ::ImGuiMouseButton_;
    using ::ImGuiMouseCursor_;
    using ::ImGuiCond_;
    using ::ImGuiTableFlags_;
    using ::ImGuiTableColumnFlags_;
    using ::ImGuiTableRowFlags_;
    using ::ImGuiTableBgTarget_;
    using ::ImGuiListClipperFlags_;
    using ::ImGuiMultiSelectFlags_;
    using ::ImGuiSelectionRequestType;
    using ::ImDrawFlags_;
    using ::ImDrawListFlags_;
    using ::ImFontAtlasFlags_;
    using ::ImFontFlags_;
    using ::ImGuiViewportFlags_;
}
