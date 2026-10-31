#include "get_open_filename.h"

#include <array>

#include <Windows.h>
#include <shobjidl.h>
#pragma comment(lib, "Ole32.lib")


struct co_stuff
	{
	HRESULT hr;
	co_stuff() : hr{CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)} {}

	co_stuff           (const co_stuff&  copy) = delete;
	co_stuff& operator=(const co_stuff&  copy) = delete;
	co_stuff           (      co_stuff&& move) = delete;
	co_stuff& operator=(      co_stuff&& move) = delete;

	~co_stuff() 
		{
		if (SUCCEEDED(hr)) { CoUninitialize(); }
		}

	operator bool() const noexcept { return SUCCEEDED(hr); }
	};

template <typename T>
struct co_release_wrapper
	{
	T* ptr{nullptr};

	co_release_wrapper(T* ptr) : ptr{ptr} {}
	co_release_wrapper() {}
	
	co_release_wrapper           (const co_release_wrapper&  copy) = delete;
	co_release_wrapper& operator=(const co_release_wrapper&  copy) = delete;
	co_release_wrapper           (      co_release_wrapper&& move) = delete;
	co_release_wrapper& operator=(      co_release_wrapper&& move) = delete;

	~co_release_wrapper()
		{
		if (ptr) { ptr->Release(); }
		}

	operator bool() const noexcept { return ptr; }

	const T* operator->() const noexcept { return ptr; }
	      T* operator->()       noexcept { return ptr; }
	};

std::optional<std::vector<std::filesystem::path>> user_selected_files(const sf::Window& owner) noexcept
	{
	std::vector<std::filesystem::path> ret;

	const co_stuff co_stuff;
	if(!co_stuff) { return std::nullopt; }

	co_release_wrapper<IFileOpenDialog> file_open_dialog{[&]() -> IFileOpenDialog*
		{
		IFileOpenDialog* ptr{nullptr};
		const HRESULT hr{CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&ptr))};
		if (FAILED(hr)) { return nullptr; }

		FILEOPENDIALOGOPTIONS options;
		ptr->GetOptions(&options);
		ptr->SetOptions(options | FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT);

		const std::array file_types
			{
			COMDLG_FILTERSPEC{ L"Supported Images (.bmp, *.dds, *.png, *.jpg, *.jpeg, *.tga, *.psd, *.webp)", L".bmp;*.dds;*.png;*.jpg;*.jpeg;*.tga;*.psd;*.webp" },
			COMDLG_FILTERSPEC{ L"All Files", L"*.*" }
			};

		ptr->SetFileTypes(static_cast<UINT>(file_types.size()), file_types.data());
		return ptr;
		}()};
	if (!file_open_dialog) { return std::nullopt; }

	const HRESULT hr{file_open_dialog->Show(owner.getNativeHandle())};
	if (FAILED(hr)) { return std::nullopt; }

	co_release_wrapper<IShellItemArray> item_array{[&]() -> IShellItemArray*
		{
		IShellItemArray* ptr{nullptr};
		const HRESULT hr{file_open_dialog->GetResults(std::addressof(ptr))};
		if (FAILED(hr)) { return nullptr; }
		return ptr;
		}()};
	if (!item_array) { return std::nullopt; }

	DWORD count{0};
	if (FAILED(item_array->GetCount(std::addressof(count)))) { return std::nullopt; }
	ret.reserve(count);

	for (DWORD i{0}; i < count; i++)
		{
		co_release_wrapper<IShellItem> item{[&]() -> IShellItem*
			{
			IShellItem* ptr{nullptr};
			const HRESULT hr{item_array->GetItemAt(i, std::addressof(ptr))};
			if (FAILED(hr)) { return nullptr; }
			return ptr;
			}()};
		if (!item) { continue; }

		wchar_t* path{nullptr};
		HRESULT hr{item->GetDisplayName(SIGDN_FILESYSPATH, std::addressof(path))};
		if (FAILED(hr)) { continue; }
		ret.emplace_back(path);
		CoTaskMemFree(path);
		}

	return ret;
	}

std::optional<std::vector<std::filesystem::path>> user_selected_directories(const sf::Window& owner) noexcept
	{
	std::vector<std::filesystem::path> ret;

	const co_stuff co_stuff;
	if(!co_stuff) { return std::nullopt; }

	co_release_wrapper<IFileOpenDialog> file_open_dialog{[&]() -> IFileOpenDialog*
		{
		IFileOpenDialog* ptr{nullptr};
		const HRESULT hr{CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&ptr))};
		if (FAILED(hr)) { return nullptr; }

		FILEOPENDIALOGOPTIONS options;
		ptr->GetOptions(&options);
		ptr->SetOptions(options | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);

		return ptr;
		}()};
	if (!file_open_dialog) { return std::nullopt; }

	const HRESULT hr{file_open_dialog->Show(owner.getNativeHandle())};
	if (FAILED(hr)) { return std::nullopt; }

	co_release_wrapper<IShellItemArray> item_array{[&]() -> IShellItemArray*
		{
		IShellItemArray* ptr{nullptr};
		const HRESULT hr{file_open_dialog->GetResults(std::addressof(ptr))};
		if (FAILED(hr)) { return nullptr; }
		return ptr;
		}()};
	if (!item_array) { return std::nullopt; }

	DWORD count{0};
	if (FAILED(item_array->GetCount(std::addressof(count)))) { return std::nullopt; }
	ret.reserve(count);

	for (DWORD i{0}; i < count; i++)
		{
		co_release_wrapper<IShellItem> item{[&]() -> IShellItem*
			{
			IShellItem* ptr{nullptr};
			const HRESULT hr{item_array->GetItemAt(i, std::addressof(ptr))};
			if (FAILED(hr)) { return nullptr; }
			return ptr;
			}()};
		if (!item) { continue; }

		wchar_t* path{nullptr};
		HRESULT hr{item->GetDisplayName(SIGDN_FILESYSPATH, std::addressof(path))};
		if (FAILED(hr)) { continue; }
		ret.emplace_back(path);
		CoTaskMemFree(path);
		}

	return ret;
	}