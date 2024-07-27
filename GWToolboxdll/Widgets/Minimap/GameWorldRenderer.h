#pragma once

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/GamePos.h>
#include <ToolboxIni.h>
#include <Widgets/Minimap/D3DVertex.h>
#include <stacktrace>

class GameWorldRenderer {
public:
    class GenericPolyRenderable {
    public:
        GenericPolyRenderable(IDirect3DDevice9* device, GW::Constants::MapID map_id, const std::vector<GW::Vec3f>& points, unsigned int col, bool filled) noexcept;
        ~GenericPolyRenderable() noexcept;

        // copy not allowed
        GenericPolyRenderable(const GenericPolyRenderable& other) = delete;

        GenericPolyRenderable(GenericPolyRenderable&& other) noexcept
            : map_id(other.map_id), col(other.col)
            , filled(other.filled)
            , vb(other.vb)
            , cur_altitude(other.cur_altitude)
            , all_altitudes_queried(other.all_altitudes_queried)
        {
            other.vb = nullptr;
            points = std::move(other.points);
            vertices = std::move(other.vertices);
        }

        // copy not allowed
        GenericPolyRenderable& operator=(const GenericPolyRenderable& other) = delete;

        GenericPolyRenderable& operator=(GenericPolyRenderable&& other) noexcept
        {
            if (vb != nullptr) {
                vb->Release();
            }
            vb = other.vb; // Move the buffer!
            other.vb = nullptr;
            points = std::move(other.points);
            vertices = std::move(other.vertices);
            map_id = other.map_id;
            col = other.col;
            filled = other.filled;
            all_altitudes_queried = other.all_altitudes_queried;
            cur_altitude = other.cur_altitude;
            return *this;
        }

        void Draw(IDirect3DDevice9* device);
        GW::Constants::MapID map_id{};
        unsigned int col = 0u;
        std::vector<GW::Vec3f> points{};
        std::vector<D3DVertex> vertices{};
        bool filled = false;

    private:
        IDirect3DVertexBuffer9* vb = nullptr;

        unsigned int cur_altitude = 0u;
        bool all_altitudes_queried = false;
    };

    static void Render(IDirect3DDevice9* device);
    static void LoadSettings(const ToolboxIni* ini, const char* section);
    static void SaveSettings(ToolboxIni* ini, const char* section);
    static void DrawSettings();
    static void Terminate();
    static void TriggerSyncAllMarkers();

    using RenderableVectors = std::vector<GenericPolyRenderable>;

private:
    static RenderableVectors SyncLines(IDirect3DDevice9* device);
    static RenderableVectors SyncPolys(IDirect3DDevice9* device);
    static RenderableVectors SyncMarkers(IDirect3DDevice9* device);
    static void SyncAllMarkers(IDirect3DDevice9* device);
    static bool ConfigureProgrammablePipeline(IDirect3DDevice9* device);
    static bool SetD3DTransform(IDirect3DDevice9* device);
};
