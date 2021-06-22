// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.210505.3

#ifndef WINRT_Windows_UI_Core_Preview_1_H
#define WINRT_Windows_UI_Core_Preview_1_H
#include "winrt/impl/Windows.UI.Core.Preview.0.h"
WINRT_EXPORT namespace winrt::Windows::UI::Core::Preview
{
    struct __declspec(empty_bases) ICoreAppWindowPreview :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ICoreAppWindowPreview>
    {
        ICoreAppWindowPreview(std::nullptr_t = nullptr) noexcept {}
        ICoreAppWindowPreview(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ICoreAppWindowPreview(ICoreAppWindowPreview const&) noexcept = default;
        ICoreAppWindowPreview(ICoreAppWindowPreview&&) noexcept = default;
        ICoreAppWindowPreview& operator=(ICoreAppWindowPreview const&) & noexcept = default;
        ICoreAppWindowPreview& operator=(ICoreAppWindowPreview&&) & noexcept = default;
    };
    struct __declspec(empty_bases) ICoreAppWindowPreviewStatics :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ICoreAppWindowPreviewStatics>
    {
        ICoreAppWindowPreviewStatics(std::nullptr_t = nullptr) noexcept {}
        ICoreAppWindowPreviewStatics(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ICoreAppWindowPreviewStatics(ICoreAppWindowPreviewStatics const&) noexcept = default;
        ICoreAppWindowPreviewStatics(ICoreAppWindowPreviewStatics&&) noexcept = default;
        ICoreAppWindowPreviewStatics& operator=(ICoreAppWindowPreviewStatics const&) & noexcept = default;
        ICoreAppWindowPreviewStatics& operator=(ICoreAppWindowPreviewStatics&&) & noexcept = default;
    };
    struct __declspec(empty_bases) ISystemNavigationCloseRequestedPreviewEventArgs :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ISystemNavigationCloseRequestedPreviewEventArgs>
    {
        ISystemNavigationCloseRequestedPreviewEventArgs(std::nullptr_t = nullptr) noexcept {}
        ISystemNavigationCloseRequestedPreviewEventArgs(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ISystemNavigationCloseRequestedPreviewEventArgs(ISystemNavigationCloseRequestedPreviewEventArgs const&) noexcept = default;
        ISystemNavigationCloseRequestedPreviewEventArgs(ISystemNavigationCloseRequestedPreviewEventArgs&&) noexcept = default;
        ISystemNavigationCloseRequestedPreviewEventArgs& operator=(ISystemNavigationCloseRequestedPreviewEventArgs const&) & noexcept = default;
        ISystemNavigationCloseRequestedPreviewEventArgs& operator=(ISystemNavigationCloseRequestedPreviewEventArgs&&) & noexcept = default;
    };
    struct __declspec(empty_bases) ISystemNavigationManagerPreview :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ISystemNavigationManagerPreview>
    {
        ISystemNavigationManagerPreview(std::nullptr_t = nullptr) noexcept {}
        ISystemNavigationManagerPreview(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ISystemNavigationManagerPreview(ISystemNavigationManagerPreview const&) noexcept = default;
        ISystemNavigationManagerPreview(ISystemNavigationManagerPreview&&) noexcept = default;
        ISystemNavigationManagerPreview& operator=(ISystemNavigationManagerPreview const&) & noexcept = default;
        ISystemNavigationManagerPreview& operator=(ISystemNavigationManagerPreview&&) & noexcept = default;
    };
    struct __declspec(empty_bases) ISystemNavigationManagerPreviewStatics :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ISystemNavigationManagerPreviewStatics>
    {
        ISystemNavigationManagerPreviewStatics(std::nullptr_t = nullptr) noexcept {}
        ISystemNavigationManagerPreviewStatics(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ISystemNavigationManagerPreviewStatics(ISystemNavigationManagerPreviewStatics const&) noexcept = default;
        ISystemNavigationManagerPreviewStatics(ISystemNavigationManagerPreviewStatics&&) noexcept = default;
        ISystemNavigationManagerPreviewStatics& operator=(ISystemNavigationManagerPreviewStatics const&) & noexcept = default;
        ISystemNavigationManagerPreviewStatics& operator=(ISystemNavigationManagerPreviewStatics&&) & noexcept = default;
    };
}
#endif