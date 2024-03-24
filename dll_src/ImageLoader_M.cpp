#include <unordered_map>
#include <string>
#include <cmath>
#include <memory>
#include <optional>
#include <algorithm>

#include <Windows.h>
#include <gdiplus.h>
#include "lua.hpp"


struct PixelBGRA {
    uint8_t b, g, r, a;
};


struct ImageData {
    int width;
    int height;
    HANDLE map;
    bool baton;

    ImageData(int w, int h) : width(w), height(h), baton(true) {
        auto bytes = w * h * sizeof(PixelBGRA);
        map = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, bytes, NULL);
    }

    ~ImageData() noexcept {
        if (baton && map)
            CloseHandle(map);
    }

    ImageData(const ImageData&) = delete;

    ImageData(ImageData&& r) noexcept {
        width = r.width;
        height= r.height;
        map = r.map;
        baton = true;
        r.baton = false;
    }

    constexpr bool is_valid() const { return map; }

    constexpr int get_bytes() const { return width * height * sizeof(PixelBGRA); }

    constexpr bool operator==(const ImageData& r) const {
        return this->map == r.map;
    }
};

struct MappedPixelData {
    PixelBGRA* pixels;
    MappedPixelData(const ImageData& data) {
        pixels = static_cast<PixelBGRA*>(MapViewOfFile(data.map, FILE_MAP_ALL_ACCESS, 0, 0, data.get_bytes()));
    }
    ~MappedPixelData() {
        UnmapViewOfFile(pixels);
    }
    PixelBGRA& operator[](int i) const{
        return pixels[i];
    }
};

enum resizing_methods {
    NEAREST_NEIGHBOR = 0,
    BILINEAR
};


ImageData* get_image_data(std::string_view filename);
int load_image(lua_State* L);
int get_image_size(lua_State* L);


static std::unordered_map<std::string, ImageData> imagelist;

inline std::optional<std::wstring> string_convert_A2W(std::string_view str) {
    auto size = MultiByteToWideChar(CP_ACP, 0, str.data(), str.size(), nullptr, 0);
    if (size == 0)return std::nullopt;
    std::wstring ret(size, '\0');
    if (MultiByteToWideChar(CP_ACP, 0, str.data(), str.size(), ret.data(), size) == 0)return std::nullopt;
    return ret;
}

ImageData* get_image_data(std::string_view filename) {
    if (auto itr = std::find_if(imagelist.begin(), imagelist.end(), [filename](const auto& elm) { return elm.first == filename; }); itr != imagelist.end())
        return &itr->second;

    auto filenamew = string_convert_A2W(filename);
    if (!filenamew.has_value())return nullptr;

    std::unique_ptr<Gdiplus::Bitmap> image(Gdiplus::Bitmap::FromFile(filenamew->c_str()));

    if (!image)
        return nullptr;

    ImageData data(image->GetWidth(), image->GetHeight());

    if (!data.is_valid())
        return nullptr;
    
    const int bytes = data.get_bytes();

    MappedPixelData mapped_pixels(data);

    if (!mapped_pixels.pixels)
        return nullptr;
    

    Gdiplus::Rect rect(0, 0, data.width, data.height);
    Gdiplus::BitmapData bmpData;
    image->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    memcpy(mapped_pixels.pixels, reinterpret_cast<byte*>(bmpData.Scan0), bytes);
    image->UnlockBits(&bmpData);

    auto [emplaced_itr, emplaced] = imagelist.try_emplace(std::string(filename), std::move(data));

    return &emplaced_itr->second;
}

int load_image(lua_State* L) {
    PixelBGRA* buffer_pixels = reinterpret_cast<PixelBGRA*>(lua_touserdata(L, 1));
    const char* filename = lua_tostring(L, 2);
    auto x = static_cast<int>(lua_tointeger(L, 3));
    auto y = static_cast<int>(lua_tointeger(L, 4));
    const auto scale = static_cast<float>(lua_tonumber(L, 5));
    const auto buffer_w = static_cast<int>(lua_tointeger(L, 6));
    const auto buffer_h = static_cast<int>(lua_tointeger(L, 7));
    const auto method = static_cast<int>(lua_tointeger(L, 8));

    if (!buffer_pixels || scale == 0 || buffer_w == 0 || buffer_h == 0)
        return 0;

    const auto data = get_image_data(filename);

    if (!data)
        return 0;

    x = (std::max)((std::min)(x, static_cast<int>(data->width - ceil(buffer_w / scale))), 0);
    y = (std::max)((std::min)(y, static_cast<int>(data->height - ceil(buffer_h / scale))), 0);

    auto bytes = data->get_bytes();
    
    MappedPixelData mapped_pixels(*data);
    if (!mapped_pixels.pixels)
        return 0;

    switch (method) {
    case NEAREST_NEIGHBOR:
    {
        auto position_list_x = std::make_unique<int[]>(buffer_w);
        auto position_list_y = std::make_unique<int[]>(buffer_h);

        for (int i = 0; i < buffer_w; i++) {
            position_list_x[i] = x + static_cast<int>((std::min)(i / scale, static_cast<float>(data->width - 1)));
        }
        for (int i = 0; i < buffer_h; i++) {
            position_list_y[i] = y + static_cast<int>((std::min)(i / scale, static_cast<float>(data->height - 1)));
        }

        for (int i = 0; i < buffer_h; i++) {
            for (int j = 0; j < buffer_w; j++) {
                buffer_pixels[i * buffer_w + j] = mapped_pixels[position_list_y[i] * data->width + position_list_x[j]];
            }
        }
        break;
    }
    case BILINEAR:
    {
        auto bilinear_inter = [](unsigned char a1, unsigned char a2, unsigned char a3, unsigned char a4, float af_x, float af_y) {
            return static_cast<unsigned char>(a1 * ((1 - af_x) * (1 - af_y)) + a2 * (af_x * (1 - af_y)) + a3 * ((1 - af_x) * af_y) + a4 * (af_x * af_y));
        };
        auto position_list_x = std::make_unique<int[]>(buffer_w);
        auto position_list_y = std::make_unique<int[]>(buffer_h);
        auto position_list_mod_x = std::make_unique<float[]>(buffer_w);
        auto position_list_mod_y = std::make_unique<float[]>(buffer_h);

        for (int i = 0; i < buffer_w; i++) {
            position_list_x[i] = x + static_cast<int>(i / scale);
            position_list_mod_x[i] = fmodf(i / scale, 1);
        }
        for (int i = 0; i < buffer_h; i++) {
            position_list_y[i] = y + static_cast<int>(i / scale);
            position_list_mod_y[i] = fmodf(i / scale, 1);
        }
        PixelBGRA* d1, * d2, * d3, * d4;
        for (int i = 0; i < buffer_h; i++) {
            for (int j = 0; j < buffer_w; j++) {
                int image_x = position_list_x[j];
                int image_y = position_list_y[i];
                d1 = &mapped_pixels[image_y * data->width + image_x];
                d2 = &mapped_pixels[image_y * data->width + (std::min)(image_x + 1, data->width - 1)];
                d3 = &mapped_pixels[(std::min)(image_y + 1, data->height - 1) * data->width + image_x];
                d4 = &mapped_pixels[(std::min)(image_y + 1, data->height - 1) * data->width + (std::min)(image_x + 1, data->width - 1)];
                buffer_pixels[j + i * buffer_w].b = bilinear_inter(d1->b, d2->b, d3->b, d4->b, position_list_mod_x[j], position_list_mod_y[i]);
                buffer_pixels[j + i * buffer_w].g = bilinear_inter(d1->g, d2->g, d3->g, d4->g, position_list_mod_x[j], position_list_mod_y[i]);
                buffer_pixels[j + i * buffer_w].r = bilinear_inter(d1->r, d2->r, d3->r, d4->r, position_list_mod_x[j], position_list_mod_y[i]);
                buffer_pixels[j + i * buffer_w].a = bilinear_inter(d1->a, d2->a, d3->a, d4->a, position_list_mod_x[j], position_list_mod_y[i]);
            }
        }
        break;
    }

    }


    return 0;
}

int get_image_size(lua_State* L) {
    const char* filename = lua_tostring(L, 1);
    const float scale = static_cast<float>(lua_tonumber(L, 2));
    const int max_w = static_cast<int>(lua_tointeger(L, 3));
    const int max_h = static_cast<int>(lua_tointeger(L, 4));

    auto data = get_image_data(filename);

    if (!data) {
        lua_pushinteger(L, -1);
        lua_pushinteger(L, -1);
    }
    else {
        lua_pushinteger(L, (std::min)(static_cast<int>(floor(data->width * scale)), max_w));
        lua_pushinteger(L, (std::min)(static_cast<int>(floor(data->height * scale)), max_h));
    }

    return 2;
}

static luaL_Reg functions[] = {
    { "load_image", load_image },
    { "get_image_size", get_image_size },
    { nullptr, nullptr }
};

EXTERN_C __declspec(dllexport) int luaopen_ImageLoader_M(lua_State* L) {
    luaL_register(L, "ImageLoader_M", functions);
    return 1;
}


static Gdiplus::GdiplusStartupInput input;
static ULONG_PTR token;

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        Gdiplus::GdiplusStartup(&token, &input, NULL);
        break;

    case DLL_PROCESS_DETACH:
        imagelist.clear();
        Gdiplus::GdiplusShutdown(token);
        break;
    }

    return TRUE;
}
