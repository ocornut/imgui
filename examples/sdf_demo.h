// Rotate from https://gist.github.com/carasuca/e72aacadcf6cf8139de46f97158f790f
#include <math.h>
static inline ImVec2 ImMin(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y); }
static inline ImVec2 ImMax(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(lhs.x >= rhs.x ? lhs.x : rhs.x, lhs.y >= rhs.y ? lhs.y : rhs.y); }
static inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a)        { return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static inline ImVec2 operator*(const float& lhs, const ImVec2& rhs)            { return ImVec2(lhs * rhs.x, lhs * rhs.y); }
int rotation_start_index; 
void ImRotateStart() 
{ 
	rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size; 
}

ImVec2 ImRotationCenter()
{
	ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

	const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
	for (int i = rotation_start_index; i < buf.Size; i++)
		l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

	return ImVec2((l.x+u.x)/2, (l.y+u.y)/2); // or use _ClipRectStack?
}

void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
{
	float s=sinf(rad), c=cosf(rad);
	center = ImRotate(center, s, c) - center;

	auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
	for (int i = rotation_start_index; i < buf.Size; i++)
		buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
}


void SDFDemo() {
  ImGui::Separator();
  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("Fonts");
  ImGui::SetWindowFontScale(1);
  ImGui::TextWrapped("Signed distance tester. You can choose multiple fonts in the style manager, supporting both signed distance fonts and regular fonts at the same time. Because of additional shader support that is needed, this is only enabled if the backend enables the new SignedDistance flags.");
  static float scale = 3.0f;
  static ImVec4 text_inner_color = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
  static ImVec4 text_outline_start = ImVec4(1.0f, .0f, .0f, 1.0f);
  static ImVec4 text_outline_end = ImVec4(1.0f, .0f, .0f, 0.0f);
  static float outline_width = .6f;
  static float rotate = 0.1f;

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);
  ImGui::SetWindowFontScale(scale);
  ImRotateStart();

  ImGui::TextWrapped("Some scaled text.");

  ImGui::PushStyleVar(ImGuiStyleVar_FontShadowSize, outline_width);
  ImGui::PushStyleColor(ImGuiCol_Text, text_inner_color);
  ImGui::PushStyleColor(ImGuiCol_FontShadowStart, text_outline_start);
  ImGui::PushStyleColor(ImGuiCol_FontShadowEnd, text_outline_end);
  ImGui::TextWrapped("Additional styles are supported.");
  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar(1);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);

  ImRotateEnd(rotate + float(M_PI/2));
  ImGui::SetWindowFontScale(1);

  ImGui::SliderFloat("scale", &scale, 0.2f, 10.0f);
  ImGui::ColorEdit4("text color", (float*)&text_inner_color);
  ImGui::ColorEdit4("outline start", (float*)&text_outline_start);
  ImGui::ColorEdit4("outline end", (float*)&text_outline_end);
  ImGui::SliderFloat("outline width", &outline_width, 0.0f, .99f);
  static bool animate_rotation = false;
  ImGui::SliderFloat("rotate", &rotate, 0.0f, float(2*M_PI));
  ImGui::Checkbox("Animate rotation", &animate_rotation);
  if (animate_rotation)
    rotate = 0.05f*float(ImGui::GetTime());

  ImGui::Separator();
  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("Shapes");
  ImGui::SetWindowFontScale(1);
  ImGui::TextWrapped("To support text effects, additional information needs be encoded in ImDrawVert. This same information can easily be used for drawing outlines and shadow for regular shapes. A rounded rectangle with shadow can be drawn with 18 triangles (size of shadow or rounding does not have an impact). See the DrawCmd in Metrics/Debugger window how these rectangles are drawn (especially ones with some round corners, and some straight corners).");

  ImGui::TextWrapped("Support for these new rounded rectangles is enabled if the flag SignedDistanceShapes is set by the backend. If enabled, AddRect and AddRectFilled are using these new implementation of shapes. This causes the number of vertices and triangles to drop significantly when using rounded frames.");

  ImGui::TextWrapped("Because AddRect and AddRectFilled are extended with additional functionality, window and frame shadows can easily be added. To support styling additional variables are introduced: WindowShadowSize and FrameShadowSize (see 'Style' tab).");

  static ImVec4 color = ImColor(IM_COL32(0xff, 0x98, 0x07, 0xff)); //ImGui::GetColorU32(ImGuiCol_Text));
  ImGui::ColorEdit4("inner color", (float*)&color); // Edit 3 floats representing a color
  static ImVec4 shadowStart = ImVec4(.0f, .0f, .0f, .25f);
  ImGui::ColorEdit4("shadow start", (float*)&shadowStart); // Edit 3 floats representing a color
  static ImVec4 shadowEnd = ImVec4(.0f, .0f, .0f, 0.0f);
  ImGui::ColorEdit4("shadow end", (float*)&shadowEnd); // Edit 3 floats representing a color

  static float shadow = 50;
  ImGui::SliderFloat("shadow", &shadow, 0.0f, 100.0f);

  static bool rounded_top_left = true;
  static bool rounded_top_right = true;
  static bool rounded_bottom_left = true;
  static bool rounded_bottom_right = false;
  static float radius = 100.0f;
  ImGui::SliderFloat("radius", &radius, 0.0f, 200.0f);
  ImGui::Text("Rounded:");
  ImGui::SameLine();
  ImGui::BeginGroup();
  ImGui::Checkbox("top-left", &rounded_top_left);
  ImGui::SameLine();
  ImGui::Checkbox("top-right", &rounded_top_right);
  ImGui::Checkbox("bottom-left", &rounded_bottom_left);
  ImGui::SameLine();
  ImGui::Checkbox("bottom-right", &rounded_bottom_right);
  ImGui::EndGroup();

  int w = 300;
  int h = 200;

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);

  auto pos = ImGui::GetCursorScreenPos();

  //pos.y += 100;
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  ImDrawFlags flags = 0;
  if (rounded_top_left)
    flags |= ImDrawFlags_RoundCornersTopLeft;
  if (rounded_top_right)
    flags |= ImDrawFlags_RoundCornersTopRight;
  if (rounded_bottom_left)
    flags |= ImDrawFlags_RoundCornersBottomLeft;
  if (rounded_bottom_right)
    flags |= ImDrawFlags_RoundCornersBottomRight;
  float radiusCopy = radius;
  if (flags == 0)
    radiusCopy = 0.0f;
  drawList->AddRectFilled(ImVec2(pos.x + 100.0f, pos.y), ImVec2(pos.x + w + 100.0f, pos.y + h), ImColor(color), radiusCopy, flags, shadow, ImColor(shadowStart), ImColor(shadowEnd));

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + h + 50);
}
