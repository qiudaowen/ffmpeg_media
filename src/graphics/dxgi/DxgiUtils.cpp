#include "DxgiUtils.h"
#include <d3d11.h>
#include "utils/QsMonitorInfo.h"

namespace DxgiUtils {

	void enumAdapter(AdapterCb&& cb, bool bFilterWindowAdapter)
	{
		CComPtr<IDXGIFactory1> factory;
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		if (FAILED(hr))
			return;

		for (int i = 0;;++i)
		{
			CComPtr<IDXGIAdapter> adapter;
			hr = factory->EnumAdapters(i, &adapter);
			if (hr != S_OK)
				break;

			DXGI_ADAPTER_DESC adapterDesc;
			hr = adapter->GetDesc(&adapterDesc);
			if (FAILED(hr))
				continue;

			/* ignore Microsoft's 'basic' renderer' */
			if ((bFilterWindowAdapter && adapterDesc.VendorId == 0x1414 && adapterDesc.DeviceId == 0x8c) )
				continue;

			if (!cb(i, adapter, adapterDesc))
				return;
		}
	}
	void enumAdapterOutput(AdapterOutputCb&& cb, bool bFilterWindowAdapter)
	{
		CComPtr<IDXGIFactory1> factory;
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		if (FAILED(hr))
			return;

		for (int i = 0;; ++i)
		{
			CComPtr<IDXGIAdapter> adapter;
			hr = factory->EnumAdapters(i, &adapter);
			if (hr != S_OK)
				break;

			DXGI_ADAPTER_DESC adapterDesc;
			hr = adapter->GetDesc(&adapterDesc);
			if (FAILED(hr))
				continue;

			/* ignore Microsoft's 'basic' renderer' */
			if ((bFilterWindowAdapter && adapterDesc.VendorId == 0x1414 && adapterDesc.DeviceId == 0x8c))
				continue;

			CComPtr<IDXGIOutput> output;
			for (int iOutput = 0;; ++iOutput)
			{
				hr = adapter->EnumOutputs(iOutput, &output);
				if (hr != S_OK)
					break;

				DXGI_OUTPUT_DESC outputDesc;
				hr = output->GetDesc(&outputDesc);
				if (FAILED(hr))
					continue;

				if (!cb(i, adapter, adapterDesc, iOutput, output, outputDesc))
					return;
			}
		}
	}

	HRESULT monitorToOutputIndex(int monitor_idx, int* pOutputIndex, IDXGIOutput** ppOutput, IDXGIAdapter** ppAdpter)
	{
		QsMonitorInfo monitorInfo;
		if (!getMonitorInfo(monitor_idx, monitorInfo))
			return E_FAIL;

		bool bFounded = false;
		enumAdapterOutput([&](int, IDXGIAdapter* pAdapter, const DXGI_ADAPTER_DESC& desc, int iOutputIndex, IDXGIOutput* pOutput, const DXGI_OUTPUT_DESC& outputDesc)->bool {
			if (outputDesc.AttachedToDesktop && monitorInfo.hMonitor == outputDesc.Monitor)
			{
				bFounded = true;
				if (ppOutput)
				{
					*ppOutput = pOutput;
					pOutput->AddRef();
				}
				if (ppAdpter)
				{
					*ppAdpter = pAdapter;
					pAdapter->AddRef();
				}
				if (pOutputIndex)
					*pOutputIndex = iOutputIndex;
				return false;
			}
			return true;
		});
		return bFounded ? S_OK : E_FAIL;
	}

	HRESULT getGetAdapter(ID3D11Device* pDx11Device, IDXGIAdapter** ppAdapter)
	{
		if (pDx11Device == nullptr)
			return E_FAIL;

		CComPtr<IDXGIDevice> dxgiDevice;
		HRESULT hr = pDx11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
		if (FAILED(hr))
		{
			return hr;
		}
		hr = dxgiDevice->GetAdapter(ppAdapter);
		if (FAILED(hr))
		{
			return hr;
		}
		return hr;
	}

	bool isSameAdapter(IDXGIAdapter* pAdapter1, IDXGIAdapter* pAdapter2)
	{
		DXGI_ADAPTER_DESC adapterDesc1;
		HRESULT hr = pAdapter1->GetDesc(&adapterDesc1);
		DXGI_ADAPTER_DESC adapterDesc2;
		pAdapter2->GetDesc(&adapterDesc2);
		return memcmp(&adapterDesc1.AdapterLuid, &adapterDesc2.AdapterLuid, sizeof(adapterDesc2.AdapterLuid)) == 0;
	}

	CComPtr<IDXGIFactory1> dxgiFactory()
	{
		CComPtr<IDXGIFactory1> factory;
		CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		return factory;
	}

	CComPtr<IDXGIAdapter1> getDefaultAdapter()
	{
		CComPtr<IDXGIAdapter1> adapter;
		dxgiFactory()->EnumAdapters1(0, &adapter);
		return adapter;
	}

	CComPtr<IDXGIAdapter> getAdapter(uint64_t adapterLUID)
	{
		CComPtr<IDXGIAdapter> ret;
		enumAdapter([&](int index, IDXGIAdapter* pAdapter, const DXGI_ADAPTER_DESC& desc)->bool {
			if (memcmp(&adapterLUID, &desc.AdapterLuid, sizeof(desc.AdapterLuid)) == 0)
			{
				ret = pAdapter;
				return false;
			}
			return true;
		});
		return ret;
	}

	void fillSwapChainDesc(HWND hWnd, UINT w, UINT h, DXGI_SWAP_CHAIN_DESC *out)
	{
		ZeroMemory(out, sizeof(*out));
		out->BufferCount = 3;
		out->BufferDesc.Width = w;
		out->BufferDesc.Height = h;
		out->BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		out->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		out->SampleDesc.Count = 1;
		out->OutputWindow = hWnd;
		out->Windowed = TRUE;
		out->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		//out->Flags = 512; // DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO;

		bool isWin10OrGreater = false;
		HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
		if (hKernel32 != NULL)
			isWin10OrGreater = GetProcAddress(hKernel32, "GetSystemCpuSetInformation") != NULL;
		if (isWin10OrGreater)
			out->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		else
		{
			bool isWin80OrGreater = false;
			if (hKernel32 != NULL)
				isWin80OrGreater = GetProcAddress(hKernel32, "CheckTokenCapability") != NULL;
			if (isWin80OrGreater)
				out->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			else
			{
				out->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				out->BufferCount = 1;
			}
		}
	}
};