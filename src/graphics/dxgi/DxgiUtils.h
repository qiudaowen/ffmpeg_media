#pragma once

#include <functional>
#include <dxgi.h>
#include <atlbase.h>
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIOutputDuplication;

using AdapterCb = std::function<bool(int index, IDXGIAdapter* pAdapter, const DXGI_ADAPTER_DESC& desc)>;
using AdapterOutputCb = std::function<bool(int adapter, IDXGIAdapter* pAdapter, const DXGI_ADAPTER_DESC& desc, int iOutputIndex, IDXGIOutput* pOutput, const DXGI_OUTPUT_DESC& outputDesc)>;
namespace DxgiUtils {
	void enumAdapter(AdapterCb&& fun, bool bFilterWindowAdapter = true);
	void enumAdapterOutput(AdapterOutputCb&& fun, bool bFilterWindowAdapter = true);
	HRESULT monitorToOutputIndex(int monitor_idx, int* outputIndex, IDXGIOutput** output1, IDXGIAdapter** ppAdpter1);

	HRESULT createDevice(IDXGIAdapter* adapter, ID3D11Device** ppDevice, ID3D11DeviceContext** ppDeviceContext);
	HRESULT createDuplicator(int monitorIndex, ID3D11Device* pDevice, IDXGIOutputDuplication** ppDuplicator, ID3D11Device** ppDevice, int* pDxgiOutputIndex);
	
	CComPtr<IDXGIAdapter> getAdapter(ID3D11Device* pDx11Device);
	bool isSameAdapter(IDXGIAdapter* pAdapter1, IDXGIAdapter* pAdapter2);
	bool isSameAdapter(ID3D11Device* device1, ID3D11Device* device2);

	void fillSwapChainDesc(HWND hWnd, UINT w, UINT h, DXGI_SWAP_CHAIN_DESC *out);

	CComPtr<IDXGIFactory1> dxgiFactory();
	CComPtr<IDXGIAdapter1> getDefaultAdapter();
	CComPtr<IDXGIAdapter> getAdapter(uint64_t adapterLUID);
};
