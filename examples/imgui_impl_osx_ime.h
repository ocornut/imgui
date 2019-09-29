// dear imgui: Platform Binding for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)

static const NSRange kEmptyRange = { NSNotFound, 0 };

@interface ImGuiView : IMGUI_IMPLOSX_BASEVIEW <NSTextInputClient> {
    int m_x;
    int m_y;
}
- (void)setPosX:(int)x setPosY:(int)y;
@end

@implementation ImGuiView

- (void)setPosX:(int)x setPosY:(int)y {
    m_x = x;
    m_y = y;
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange {
    NSString* characters = [aString isKindOfClass: [NSAttributedString class]]
    ? [aString string]
    : aString;
    ImGuiIO& io = ImGui::GetIO();
    NSUInteger len = [characters length];
    for (NSUInteger i = 0; i < len; ++i) {
        const unichar codepoint = [characters characterAtIndex:i];
        if ((codepoint & 0xff00) == 0xf700)
            continue;
        io.AddInputCharacter(codepoint);
    }
}

- (BOOL)hasMarkedText {
    return false;
}

- (NSRange)markedRange {
    return kEmptyRange;
}

- (NSRange)selectedRange {
    return kEmptyRange;
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
}

- (void)unmarkText {
}

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange {
    if (actualRange) {
        *actualRange = aRange;
    }
    NSWindow* window = [self window];
    NSRect contentRect = [window contentRectForFrameRect:[window frame]];
    NSRect rect = NSMakeRect(m_x, contentRect.size.height - m_y, 0, 0);
    return [window convertRectToScreen:rect];
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSInteger)conversationIdentifier {
    return (NSInteger) self;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)thePoint {
    return 0;
}

- (NSArray *)validAttributesForMarkedText {
    return [NSArray array];
}

@end

static void ImGui_ImplOSX_SetImePos(int x, int y) {
    if (ImGuiView* view = (__bridge ImGuiView*)ImGui::GetIO().ImeWindowHandle) {
        [view setPosX: x setPosY:y];
    }
}

static void ImGui_ImplOSX_InitIme(ImGuiView* view) {
    ImGuiIO& io = ImGui::GetIO();
    io.ImeWindowHandle = (__bridge void*)view;
    io.ImeSetInputScreenPosFn = ImGui_ImplOSX_SetImePos;
}
