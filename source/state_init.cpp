#include "state.h"


void state::create_render_shaders()
{
	const std::vector<kl::dx::layout_descriptor> layout_descriptors = {
        {     "KL_Home", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "KL_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "KL_Velocity", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        {    "KL_Color", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const std::string shader_source = kl::files::read_string("shaders/render.hlsl");
	const kl::compiled_shader compiled_vertex_shader = gpu.compile_vertex_shader(shader_source);
	const kl::compiled_shader compiled_pixel_shader = gpu.compile_pixel_shader(shader_source);

	render_shaders.input_layout = gpu.create_input_layout(compiled_vertex_shader, layout_descriptors);
	render_shaders.vertex_shader = gpu.device_holder::create_vertex_shader(compiled_vertex_shader);
	render_shaders.pixel_shader = gpu.device_holder::create_pixel_shader(compiled_pixel_shader);
}

void state::setup_gui()
{
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(gpu.device().Get(), gpu.context().Get());

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(15.0f, 15.0f);
    style.WindowRounding = 2.0f;
    style.FramePadding = ImVec2(5.0f, 5.0f);
    style.FrameRounding = 2.0f;
    style.ItemSpacing = ImVec2(12.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.SelectableTextAlign = ImVec2(0.5f, 0.5f);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 5.0f;
    style.GrabRounding = 3.0f;
    style.PopupBorderSize = 1.0f;
    style.PopupRounding = 5.0f;
    style.ChildBorderSize = 1.0f;
    style.ChildRounding = 5.0f;

    const ImVec4 colorNone = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    const ImVec4 colorDark = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    const ImVec4 colorMid = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    const ImVec4 colorLight = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    const ImVec4 colorSpec = ImVec4(0.7f, 0.4f, 0.0f, 1.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = colorLight;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.077f, 0.077f, 0.077f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = colorDark;
    style.Colors[ImGuiCol_PopupBg] = colorDark;
    style.Colors[ImGuiCol_Border] = colorLight;
    style.Colors[ImGuiCol_BorderShadow] = colorMid;
    style.Colors[ImGuiCol_FrameBg] = colorDark;
    style.Colors[ImGuiCol_FrameBgHovered] = colorMid;
    style.Colors[ImGuiCol_FrameBgActive] = colorLight;
    style.Colors[ImGuiCol_TitleBg] = colorDark;
    style.Colors[ImGuiCol_TitleBgActive] = colorDark;
    style.Colors[ImGuiCol_TitleBgCollapsed] = colorDark;
    style.Colors[ImGuiCol_MenuBarBg] = colorDark;
    style.Colors[ImGuiCol_ScrollbarBg] = colorDark;
    style.Colors[ImGuiCol_ScrollbarGrab] = colorLight;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = colorMid;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = colorLight;
    style.Colors[ImGuiCol_CheckMark] = colorSpec;
    style.Colors[ImGuiCol_SliderGrab] = colorSpec;
    style.Colors[ImGuiCol_SliderGrabActive] = colorSpec;
    style.Colors[ImGuiCol_Button] = colorMid;
    style.Colors[ImGuiCol_ButtonHovered] = colorLight;
    style.Colors[ImGuiCol_ButtonActive] = colorLight;
    style.Colors[ImGuiCol_Header] = colorMid;
    style.Colors[ImGuiCol_HeaderHovered] = colorLight;
    style.Colors[ImGuiCol_HeaderActive] = colorSpec;
    style.Colors[ImGuiCol_Separator] = colorMid;
    style.Colors[ImGuiCol_SeparatorHovered] = colorLight;
    style.Colors[ImGuiCol_SeparatorActive] = colorSpec;
    style.Colors[ImGuiCol_ResizeGrip] = colorMid;
    style.Colors[ImGuiCol_ResizeGripHovered] = colorLight;
    style.Colors[ImGuiCol_ResizeGripActive] = colorSpec;
    style.Colors[ImGuiCol_Tab] = colorMid;
    style.Colors[ImGuiCol_TabHovered] = colorSpec;
    style.Colors[ImGuiCol_TabActive] = colorSpec;
    style.Colors[ImGuiCol_TabUnfocused] = colorMid;
    style.Colors[ImGuiCol_TabUnfocusedActive] = colorLight;
    style.Colors[ImGuiCol_DockingPreview] = colorSpec;
    style.Colors[ImGuiCol_DockingEmptyBg] = colorMid;
    style.Colors[ImGuiCol_PlotLines] = colorSpec;
    style.Colors[ImGuiCol_PlotLinesHovered] = colorLight;
    style.Colors[ImGuiCol_PlotHistogram] = colorSpec;
    style.Colors[ImGuiCol_PlotHistogramHovered] = colorLight;
    style.Colors[ImGuiCol_TableHeaderBg] = colorMid;
    style.Colors[ImGuiCol_TableBorderStrong] = colorDark;
    style.Colors[ImGuiCol_TableBorderLight] = colorLight;
    style.Colors[ImGuiCol_TableRowBg] = colorDark;
    style.Colors[ImGuiCol_TableRowBgAlt] = colorMid;
    style.Colors[ImGuiCol_TextSelectedBg] = colorLight;
    style.Colors[ImGuiCol_DragDropTarget] = colorSpec;
    style.Colors[ImGuiCol_NavHighlight] = colorSpec;
    style.Colors[ImGuiCol_NavWindowingHighlight] = colorSpec;
    style.Colors[ImGuiCol_NavWindowingDimBg] = colorMid;
    style.Colors[ImGuiCol_ModalWindowDimBg] = colorMid;
}
