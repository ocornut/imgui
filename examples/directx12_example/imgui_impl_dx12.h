struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12RootSignature;

#define FRAME_COUNT 3

bool ImGui_ImplDX12_Init(void* _hwnd);
void ImGui_ImplDX12_RenderDrawLists(ImDrawData* _draw_data);

void ImGui_ImplDX12_CreateDeviceObjects(ID3D12Device* _device
        , ID3D12GraphicsCommandList* _commandList
        , ID3D12CommandQueue* _commandQueue
        , ID3D12Fence* _fence
        , D3D12_CPU_DESCRIPTOR_HANDLE _srvHandle
        , ID3D12RootSignature* _rootSignature);

void ImGui_ImplDX12_SetRenderData(ID3D12GraphicsCommandList* _commandList
        , D3D12_CPU_DESCRIPTOR_HANDLE _renderTarget);

void ImGui_ImplDX12_NewFrame();