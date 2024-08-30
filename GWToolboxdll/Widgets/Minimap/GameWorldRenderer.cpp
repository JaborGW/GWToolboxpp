#include "stdafx.h"

#include <GWCA/Context/MapContext.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Camera.h>
#include <GWCA/GameEntities/Pathing.h>
#include <GWCA/Managers/CameraMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/RenderMgr.h>

#include <Defines.h>
#include <Widgets/Minimap/GameWorldRenderer.h>
#include <Widgets/Minimap/Minimap.h>
#include <ImGuiAddons.h>

// Note: these two files are autogenerated by CMake!
#include "GWCA/GameEntities/Agent.h"
#include "GWCA/Managers/AgentMgr.h"
#include "Shaders/game_world_renderer_vs.h"
#include "Shaders/game_world_renderer_dotted_ps.h"

namespace {
    unsigned lerp_steps_per_line = 10;
    float render_max_distance = 5000.f;
    float fog_factor = 0.5f;
    bool need_sync_markers = true;
    bool need_configure_pipeline = true;


    GameWorldRenderer::RenderableVectors renderables;
    std::mutex renderables_mutex{};
    IDirect3DVertexShader9* vshader = nullptr;
    IDirect3DPixelShader9* pshader = nullptr;
    IDirect3DVertexDeclaration9* vertex_declaration = nullptr;

    constexpr GW::Vec2f lerp(const GW::Vec2f& a, const GW::Vec2f& b, const float t)
    {
        return a * t + b * (1.f - t);
    }

    constexpr auto ALTITUDE_UNKNOWN = std::numeric_limits<float>::max();

    std::vector<GW::GamePos> circular_points_from_marker(const GW::GamePos& marker, const float size)
    {
        std::vector<GW::GamePos> points{};
        constexpr float pi = DirectX::XM_PI;
        constexpr size_t num_points_per_circle = 48;
        constexpr auto slice = 2.0f * pi / static_cast<float>(num_points_per_circle);
        for (auto i = 0u; i < num_points_per_circle; i++) {
            const auto angle = slice * static_cast<float>(i);
            points.emplace_back(marker.x + size * std::cos(angle), marker.y + size * std::sin(angle), marker.zplane);
        }
        points.push_back(points.at(0)); // to complete the line list
        return points;
    }

    GameWorldRenderer::GenericPolyRenderable* find_matching_poly(const GameWorldRenderer::GenericPolyRenderable& poly_to_find)
    {
        // Check to see if we've already got this poly plotted; this will save us having to calculate altitude later.
        const auto found = std::ranges::find_if(renderables, [&poly_to_find](const GameWorldRenderer::GenericPolyRenderable& check) {
            if (!(check.map_id == poly_to_find.map_id
                  && check.col == poly_to_find.col
                  && check.filled == poly_to_find.filled
                  && check.points.size() == poly_to_find.points.size()))
                return false;
            for (size_t i = 0; i < check.points.size(); i++) {
                if (check.points[i] != poly_to_find.points[i])
                    return false;
            }
            return true;
        });
        if (found != renderables.end()) {
            return &(*found);
        }
        return nullptr;
    }

    // update altitudes if not done already, then add to the device buffer
    bool AddPolyToDevice(GameWorldRenderer::GenericPolyRenderable& poly, IDirect3DDevice9* device)
    {
        if (poly.vb)
            return true; // Already created the vertex buffer for this poly, which means altitudes have been done!
        auto& vertices = poly.vertices;
        if (poly.vertices_processed == vertices.size())
            return true;
        // altitudes (Z value) for each vertex can't be known until we are in the correct map,
        // so these are dynamically computed, one-time.
        float altitude = ALTITUDE_UNKNOWN;

        // in order to properly query altitudes, we have to use the pathing map
        // to determine the number of Z planes in the current map.
        const GW::PathingMapArray* pathing_map = GW::Map::GetPathingMap();
        if (!pathing_map)
            return false;
        const size_t pmap_size = pathing_map->size();
        if (!pmap_size)
            return false;


        const auto z_plane0 = poly.vertices_zplanes[0];
        GW::Map::QueryAltitude({vertices[0].x, vertices[0].y, z_plane0}, 5.f, altitude);
        [[maybe_unused]] const auto altitude0 = altitude;
        ++poly.vertices_processed;
        vertices[0].z = altitude;

        const auto z_planeZ = poly.vertices_zplanes[vertices.size() - 1];
        GW::Map::QueryAltitude({vertices[vertices.size() - 1].x, vertices[vertices.size() - 1].y, z_planeZ}, 5.f, altitude);
        [[maybe_unused]] const auto altitudeZ = altitude;
        vertices[vertices.size() - 1].z = altitude;

        const auto altitude_diff = altitudeZ - altitude0;

        for (size_t i = poly.vertices_processed; i < vertices.size() - 1; i++, poly.vertices_processed++) {
            // until we have a better solution, all Z planes will be queried per vertex
            // to avoid a significant delay in the render thread, query one plane per frame
            // until all have been queried. this might result in some renderables shifting
            // not appearing for a while on first map load, but IMO is better than stalling.
            // It seems to take, even in most extreme cases, less time than it takes for agents
            // to appear.

            // @Cleanup: zplane needs setting properly here!
            const auto z_plane = poly.vertices_zplanes[i];
            GW::Map::QueryAltitude({vertices[i].x, vertices[i].y, z_plane}, 5.f, altitude);

            if (altitude < vertices[i].z) {
                // recall that the Up camera component is inverted
                vertices[i].z = altitude;
            }

            const auto guessed_altitude = altitude0 + (altitude_diff * static_cast<float>(i) / static_cast<float>(vertices.size() - 1));
            if (std::abs(altitude - guessed_altitude) > 20.f) {
                auto min_diff = std::abs(altitude - guessed_altitude);
                for (unsigned zplane = pmap_size - 1; zplane >= 1; zplane -= 1) {
                    GW::Map::QueryAltitude({vertices[i].x, vertices[i].y, zplane}, 5.f, altitude);
                    const auto cur_diff = std::abs(altitude - guessed_altitude);
                    if (cur_diff < min_diff && altitude < vertices[i].z) {
                        min_diff = cur_diff;
                        vertices[i].z = altitude;
                    }
                }
            }
        }
        ++poly.vertices_processed;

        // commit the completed vertices to vram
        auto res = device->CreateVertexBuffer(vertices.size() * sizeof(D3DVertex), D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &poly.vb, nullptr);
        if (res != S_OK) {
            poly.vb = nullptr;
            return false;
        }
        void* mem_loc = nullptr;
        // map the vertex buffer memory and write vertices to it.

        res = poly.vb->Lock(0, vertices.size() * sizeof(D3DVertex), &mem_loc, D3DLOCK_DISCARD);
        if (!(res == S_OK && mem_loc)) {
            poly.vb->Release();
            poly.vb = nullptr;
            return false;
        }
        // this should avoid an invalid memcpy, if locking fails for some reason
        memcpy(mem_loc, vertices.data(), vertices.size() * sizeof(D3DVertex));
        poly.vb->Unlock();
        return true;
    }
} // namespace

GameWorldRenderer::GenericPolyRenderable::GenericPolyRenderable(
    const GW::Constants::MapID map_id,
    const std::vector<GW::GamePos>& points,
    const unsigned int col,
    const bool filled) noexcept
    : map_id(map_id),
      col(col),
      points(points),
      filled(filled) {}

GameWorldRenderer::GenericPolyRenderable::~GenericPolyRenderable() noexcept
{
    if (vb != nullptr) {
        vb->Release();
    }
}

void GameWorldRenderer::GenericPolyRenderable::Draw(IDirect3DDevice9* device)
{
    if (vertices.empty()) {
        if (filled && points.size() >= 3) {
            // (filling doesn't make sense if there is not at least enough points for one triangle)
            std::vector<GW::GamePos> lerp_points{};
            for (size_t i = 0; i < points.size(); i++) {
                if (!lerp_points.empty() && lerp_steps_per_line > 0) {
                    for (auto j = 1u; j < lerp_steps_per_line; j++) {
                        const float div = static_cast<float>(j) / static_cast<float>(lerp_steps_per_line);
                        auto split = lerp(points[i], points[i - 1], div);
                        lerp_points.emplace_back(split.x, split.y, points[i].zplane);
                    }
                }
                lerp_points.push_back(points[i]);
            }
            const std::vector<unsigned> indices = mapbox::earcut<unsigned>(std::vector{{lerp_points}});
            for (size_t i = 0; i < indices.size(); i++) {
                const auto& pt = lerp_points[indices[i]];
                vertices.emplace_back(pt.x, pt.y, ALTITUDE_UNKNOWN, col);
                vertices_zplanes.push_back(pt.zplane);
            }
        }
        else {
            for (size_t i = 0; i < points.size(); i++) {
                const auto& pt = points[i];
                if (!vertices.empty() && lerp_steps_per_line > 0) {
                    for (auto j = 1u; j < lerp_steps_per_line; j++) {
                        const auto div = static_cast<float>(j) / static_cast<float>(lerp_steps_per_line);
                        const auto split = lerp(points[i], points[i - 1], div);
                        vertices.emplace_back(split.x, split.y, ALTITUDE_UNKNOWN, col);
                        const auto zplanes = std::vector{points[i].zplane, points[i - 1].zplane, points[0].zplane, points[points.size() - 1].zplane};
                        const auto zplane = std::ranges::max_element(zplanes);
                        vertices_zplanes.push_back(*zplane);
                    }
                }
                vertices.emplace_back(pt.x, pt.y, ALTITUDE_UNKNOWN, col);
                vertices_zplanes.push_back(pt.zplane);
            }
        }
    }


    if (!AddPolyToDevice(*this, device))
        return;

    if (from_player_pos && !vertices.empty()) {
        const auto player = GW::Agents::GetControlledCharacter();
        if (player) {
            auto& vertex = vertices[0];
            vertex.x = player->pos.x;
            vertex.y = player->pos.y;
            vertex.z = player->name_tag_z;
            void* mem_loc = nullptr;
            auto res = vb->Lock(0, sizeof(D3DVertex), &mem_loc, D3DLOCK_DISCARD);
            if (res == S_OK) {
                memcpy(mem_loc, &vertex, sizeof(D3DVertex));
                vb->Unlock();
            }
        }
    }

    // draw this specific renderable
    if (device->SetStreamSource(0, vb, 0, sizeof(D3DVertex)) != D3D_OK) {
        // a safe failure mode
        return;
    }

    // copy the vertex buffer to the back buffer
    filled ? device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, vertices.size() / 3) : device->DrawPrimitive(D3DPT_LINESTRIP, 0, vertices.size() - 1);
}

bool GameWorldRenderer::SetD3DTransform(IDirect3DDevice9* device)
{
    // set up directX standard view/proj matrices according to those used to render the game world
    if (device == nullptr) {
        return false;
    }

    constexpr auto vertex_shader_view_matrix_offset = 0u;
    constexpr auto vertex_shader_proj_matrix_offset = 4u;

    // compute view matrix:
    DirectX::XMFLOAT4X4A mat_view{};
    const auto cam = GW::CameraMgr::GetCamera();
    const DirectX::XMFLOAT3 eye_pos = {cam->position.x, cam->position.y, cam->position.z};
    const DirectX::XMFLOAT3 player_pos = {cam->look_at_target.x, cam->look_at_target.y, cam->look_at_target.z};
    constexpr DirectX::XMFLOAT3 up = {0.0f, 0.0f, -1.0f};
    XMStoreFloat4x4A(
        &mat_view,
        XMMatrixTranspose(
            DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eye_pos), XMLoadFloat3(&player_pos), XMLoadFloat3(&up))
        )
    );
    if (device->SetVertexShaderConstantF(vertex_shader_view_matrix_offset, reinterpret_cast<const float*>(&mat_view), 4) != D3D_OK) {
        Log::Error("GameWorldRenderer: unable to SetVertexShaderConstantF(view), aborting render.");
        return false;
    }

    // compute projection matrix:
    DirectX::XMFLOAT4X4A mat_proj{};
    const auto fov = GW::Render::GetFieldOfView();
    const auto aspect_ratio = static_cast<float>(GW::Render::GetViewportWidth()) / static_cast<float>(GW::Render::GetViewportHeight());

    XMStoreFloat4x4A(
        &mat_proj,
        XMMatrixTranspose(
            DirectX::XMMatrixPerspectiveFovLH(fov, aspect_ratio, 0.1f, 100000.0f)
        )
    );
    if (device->SetVertexShaderConstantF(vertex_shader_proj_matrix_offset, reinterpret_cast<const float*>(&mat_proj), 4) != D3D_OK) {
        Log::Error("GameWorldRenderer: unable to SetVertexShaderConstantF(projection), aborting render.");
        return false;
    }

    return true;
}

void GameWorldRenderer::Render(IDirect3DDevice9* device)
{
    if (need_sync_markers) {
        // marker synchronisation is done when needed on the render thread, as it requires access
        // to the directX device for creating vertex buffers.
        SyncAllMarkers();
    }
    if (renderables.empty()) {
        // if nothing ticked "Draw On Terrain", don't waste performance
        return;
    }
    if (need_configure_pipeline) {
        if (!ConfigureProgrammablePipeline(device)) {
            return;
        }
    }

    if (vshader == nullptr || device->SetVertexShader(vshader) != D3D_OK) {
        Log::Error("GameWorldRenderer: unable to SetVertexShader, aborting render.");
        return;
    }
    if (pshader == nullptr || device->SetPixelShader(pshader) != D3D_OK) {
        Log::Error("GameWorldRenderer: unable to SetPixelShader, aborting render.");
        return;
    }
    if (device->SetVertexDeclaration(vertex_declaration) != D3D_OK) {
        Log::Error("GameWorldRenderer: unable to SetVertexShader declaration, aborting render.");
        return;
    }

    // backup original immediate state and transforms:
    DWORD old_D3DRS_SCISSORTESTENABLE = 0;
    DWORD old_D3DRS_STENCILENABLE = 0;
    device->GetRenderState(D3DRS_SCISSORTESTENABLE, &old_D3DRS_SCISSORTESTENABLE);
    device->GetRenderState(D3DRS_STENCILENABLE, &old_D3DRS_STENCILENABLE);

    // no scissor test / stencil (used by minimap)
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, 0u);
    device->SetRenderState(D3DRS_STENCILENABLE, 0u);

    if (SetD3DTransform(device)) {
        const GW::Camera* cam = GW::CameraMgr::GetCamera();
        // set Pixel Shader constants. they are always expressed as Float4 here:
        // first is the player's position

        constexpr auto pixel_shader_cur_pos_offset = 0u;
        constexpr auto pixel_shader_max_dist_offset = 1u;
        constexpr auto pixel_shader_fog_starts_at_offset = 2u;

        const float cur_pos_constant[4] = {cam->look_at_target.x, cam->look_at_target.y, cam->look_at_target.z, 0.0f};
        if (device->SetPixelShaderConstantF(pixel_shader_cur_pos_offset, cur_pos_constant, 1) != D3D_OK) {
            Log::Error("GameWorldRenderer: unable to SetPixelShaderConstantF#0, aborting render.");
            return;
        }

        // second is the render max distance
        const float max_dist_constant[4] = {render_max_distance, 0.0f, 0.0f, 0.0f};
        if (device->SetPixelShaderConstantF(pixel_shader_max_dist_offset, max_dist_constant, 1) != D3D_OK) {
            Log::Error("GameWorldRenderer: unable to SetPixelShaderConstantF#1, aborting render.");
            return;
        }

        // third is the fog constant
        const float fog_starts_at_constant[4] = {render_max_distance - render_max_distance * fog_factor, 0.0f, 0.0f, 0.0f};
        if (device->SetPixelShaderConstantF(pixel_shader_fog_starts_at_offset, fog_starts_at_constant, 1) != D3D_OK) {
            Log::Error("GameWorldRenderer: unable to SetPixelShaderConstantF#2, aborting render.");
            return;
        }

        const auto map_id = GW::Map::GetMapID();
        renderables_mutex.lock();
        for (auto& renderable : renderables) {
            if (renderable.map_id == map_id) {
                renderable.Draw(device);
            }
        }
        renderables_mutex.unlock();
    }

    // restore immediate state:
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, old_D3DRS_SCISSORTESTENABLE);
    device->SetRenderState(D3DRS_STENCILENABLE, old_D3DRS_STENCILENABLE);
}

bool GameWorldRenderer::ConfigureProgrammablePipeline(IDirect3DDevice9* device)
{
    constexpr D3DVERTEXELEMENT9 decl[] = {{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0}, D3DDECL_END()};
    if (device->CreateVertexDeclaration(decl, &vertex_declaration) != D3D_OK) {
        //Log::Error("GameWorldRenderer: unable to CreateVertexDeclaration");
        return false;
    }

    if (device->CreateVertexShader(reinterpret_cast<const DWORD*>(&g_vs20_main), &vshader) != D3D_OK) {
        //Log::Error("GameWorldRenderer: unable to CreateVertexShader");
        return false;
    }
    if (device->CreatePixelShader(reinterpret_cast<const DWORD*>(&g_ps20_main), &pshader) != D3D_OK) {
        //Log::Error("GameWorldRenderer: unable to CreateVertexShader");
        return false;
    }
    need_configure_pipeline = false;
    return true;
}

void GameWorldRenderer::LoadSettings(const ToolboxIni* ini, const char* section)
{
    // load the rendering settings from disk
    render_max_distance = std::max(static_cast<float>(ini->GetDoubleValue(section, VAR_NAME(render_max_distance), render_max_distance)), 10.0f);
    lerp_steps_per_line = ini->GetLongValue(section, VAR_NAME(lerp_steps_per_line), lerp_steps_per_line);
    fog_factor = std::clamp(static_cast<float>(ini->GetDoubleValue(section, VAR_NAME(fog_factor), fog_factor)), 0.0f, 1.0f);
    need_sync_markers = true;
}

void GameWorldRenderer::SaveSettings(ToolboxIni* ini, const char* section)
{
    // save the rendering settings to disk
    ini->SetDoubleValue(section, VAR_NAME(render_max_distance), render_max_distance);
    ini->SetLongValue(section, VAR_NAME(lerp_steps_per_line), lerp_steps_per_line);
    ini->SetDoubleValue(section, VAR_NAME(fog_factor), fog_factor);
}

void GameWorldRenderer::DrawSettings()
{
    // draw the settings using ImGui
    const auto red = ImGui::ColorConvertU32ToFloat4(Colors::Red());
    ImGui::TextColored(red, "Warning: This is a beta feature and will render over your character, game props, and UI elements.");
    ImGui::Text("Note: custom markers are only rendered in-game if the option is enabled for a particular marker (check settings).");
    ImGui::DragFloat("Maximum render distance", &render_max_distance, 5.f, 0.f, 10000.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::ShowHelp("Maximum distance to render custom markers on the in-game terrain.");
    need_sync_markers |= ImGui::DragInt("Interpolation granularity", reinterpret_cast<int*>(&lerp_steps_per_line), 1.0f, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::ShowHelp("Number of points to interpolate. Affects smoothness of rendering.");
    ImGui::DragFloat("Fog factor", &fog_factor, 0.1f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::ShowHelp("Scales from 0.0 (disabled) to 1.0");
}

void GameWorldRenderer::TriggerSyncAllMarkers()
{
    // a publicly accessible function to trigger a re-sync of all custom markers
    need_sync_markers = true;
}

void GameWorldRenderer::Terminate()
{
    // free up any vertex buffers
    renderables.clear();
    if (vshader)
        vshader->Release();
    vshader = nullptr;
    if (pshader)
        pshader->Release();
    pshader = nullptr;
    if (vertex_declaration)
        vertex_declaration->Release();
    vertex_declaration = nullptr;
}

void GameWorldRenderer::SyncAllMarkers()
{
    renderables_mutex.lock();
    auto lines = SyncLines();
    auto polys = SyncPolys();
    auto markers = SyncMarkers();

    renderables.clear();
    renderables.reserve(lines.size() + polys.size() + markers.size());

    for (auto& line : lines) {
        renderables.push_back(std::move(line));
    }
    for (auto& poly : polys) {
        renderables.push_back(std::move(poly));
    }
    for (auto& marker : markers) {
        renderables.push_back(std::move(marker));
    }
    renderables_mutex.unlock();
    need_sync_markers = false;
}

GameWorldRenderer::RenderableVectors GameWorldRenderer::SyncLines()
{
    // sync lines with CustomRenderer
    const auto& lines = Minimap::Instance().custom_renderer.GetLines();

    const auto map_id = GW::Map::GetMapID();

    RenderableVectors out;
    out.reserve(lines.size());
    // for each line, add as a renderable if appropriate
    for (const auto line : lines) {
        if (!(line->draw_on_terrain && line->visible)) {
            continue;
        }
        if (!(line->map == map_id || line->map == GW::Constants::MapID::None))
            continue;
        if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost && map_id == GW::Constants::MapID::Domain_of_Anguish && !line->draw_everywhere) {
            // don't draw normal lines in doa outpost
            continue;
        }
        std::vector points = {line->p1, line->p2};

        auto poly_to_add = GenericPolyRenderable(line->map, points, line->color, false);

        poly_to_add.from_player_pos = line->from_player_pos;

        // Check to see if we've already got this poly plotted; this will save us having to calculate altitude later.

        if (const auto found = find_matching_poly(poly_to_add)) {
            out.emplace_back(std::move(*found));
        }
        else {
            out.emplace_back(std::move(poly_to_add));
        }
    }
    return out;
}

GameWorldRenderer::RenderableVectors GameWorldRenderer::SyncPolys()
{
    // sync polygons with CustomRenderer
    const auto& polys = Minimap::Instance().custom_renderer.GetPolys();
    RenderableVectors out;

    const auto map_id = GW::Map::GetMapID();

    out.reserve(polys.size());
    // for each poly, add as a renderable if appropriate
    for (const auto& poly : polys) {
        if (!(poly.draw_on_terrain && poly.visible && poly.points.size())) {
            continue;
        }
        if (!(poly.map == map_id || poly.map == GW::Constants::MapID::None))
            continue;
        if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost && map_id == GW::Constants::MapID::Domain_of_Anguish) {
            // don't draw normal polys in doa outpost
            continue;
        }
        std::vector<GW::GamePos> pts{};
        std::ranges::transform(poly.points, std::back_inserter(pts), [](const GW::Vec2f& pt) { return GW::GamePos(pt); });

        auto poly_to_add = GenericPolyRenderable(poly.map, pts, poly.color, poly.filled);

        // Check to see if we've already got this poly plotted; this will save us having to calculate altitude later.
        auto found = find_matching_poly(poly_to_add);

        if (found) {
            out.emplace_back(std::move(*found));
        }
        else {
            out.emplace_back(std::move(poly_to_add));
        }
    }
    return out;
}

GameWorldRenderer::RenderableVectors GameWorldRenderer::SyncMarkers()
{
    // sync markers with CustomRenderer
    const auto& markers = Minimap::Instance().custom_renderer.GetMarkers();

    const auto map_id = GW::Map::GetMapID();

    RenderableVectors out;
    out.reserve(markers.size());
    // for each marker, add as a renderable if appropriate
    for (const auto& marker : markers) {
        if (!(marker.draw_on_terrain && marker.visible)) {
            continue;
        }
        if (!(marker.map == map_id || marker.map == GW::Constants::MapID::None)) {
            continue;
        }
        if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost && map_id == GW::Constants::MapID::Domain_of_Anguish) {
            // don't draw normal markers in doa outpost
            continue;
        }

        auto points = circular_points_from_marker(marker.pos, marker.size);

        auto poly_to_add = GenericPolyRenderable(marker.map, points, marker.color, marker.IsFilled());

        // Check to see if we've already got this poly plotted; this will save us having to calculate altitude later.
        auto found = find_matching_poly(poly_to_add);

        if (found) {
            out.emplace_back(std::move(*found));
        }
        else {
            out.emplace_back(std::move(poly_to_add));
        }
    }
    return out;
}
