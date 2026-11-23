// ---- MH3U: the picture (included by hooks.cpp in its MH3U section, inside its anonymous namespace) ----------------
// Widescreen (the 3D view widened, and the top screen rendered wider), the field of view, the screen filters, the
// particle effects and the render settings word (dither, stereoscopic 3D) for an MH3U library: the same options as
// MH4U's (Options above, mh4u.ini and the app's rows), on MH3U's own objects. Found by reading MH3U USA v1.0's code
// beside MH4U's (docs/MH3U.md "The picture"): the engine is the same MT Framework, so the functions match MH4U's one by
// one, at other addresses and with other field offsets. Nothing here runs with an MH4U library.

// sLtRender (SLTRENDER_INSTANCE, port_map.txt: MH3U's [0x00ca8944]). Its render settings: the word the property
// setters write (mVSyncEdit 0x1 ... mULCDEnableEdit 0x40, mDitherDisplay0/1 0x100000/0x200000; MH4U's +0x6ba8) and
// the word the renderer reads (MH4U's +0x6ba4; sLtRender::initGraphics 0x0098e080 clears its 0x40 at start). MH3U's
// main scene target is made 256 x 512 by sLtRender's constructor (MH4U's 240 x 400) with no anti-aliasing mode (its
// FSAA parameter is 0, and the word has no mFSAA_2D): the app's Anti-aliasing row has nothing to turn off on MH3U.
constexpr u32 MH3U_RENDER_EDIT = 0x6bb4;
constexpr u32 MH3U_RENDER_FLAGS = 0x6bb0;
constexpr u32 MH3U_RENDER_ROTATED = 0x8;          // (the applied word) the screens drawn rotated into the scene target
constexpr u32 MH3U_SLTRENDER_SCENE = 0x6c30;      // nDraw::Scene*: the main render target (MH4U +0x6c28)
constexpr u32 MH3U_SLTRENDER_TOP_TRANSFER_Y = 0x6d64;     // u32: first row of the top screen in the target (112)
constexpr u32 MH3U_SLTRENDER_BOTTOM_TRANSFER_Y = 0x6d6c;  // u32: the bottom screen's (112)
constexpr u32 MH3U_SCENE_ROWS = 512;              // the main scene target's rows (MH4U: 400, as the top screen)
constexpr u32 MH3U_SCENE_CTOR_RETURN = 0x00990394;  // sLtRender's constructor, after its call of nDraw::Scene::Scene
// sCamera (SCAMERA_INSTANCE, port_map.txt: [0x00ca8914]): 8 viewports of 0x198 bytes at +0x28 (sCamera::update
// 0x008804a4), each with its camera at +0x04, mVisible +0x10, mMode +0x14, mDisplay +0x15 (0 top, 1 bottom) and its
// region {x0, y0, x1, y1} at +0x18 (sCamera::Viewport::updateRegion 0x00881478; MH4U's +0x24, a byte further on each)
constexpr u32 MH3U_SCAMERA_VIEWPORTS = 0x28, MH3U_VIEWPORT_SIZE = 0x198;
constexpr u32 MH3U_VIEWPORT_CAMERA = 0x04, MH3U_VIEWPORT_SCENE_TEXTURE = 0x0c, MH3U_VIEWPORT_MODE = 0x14,
              MH3U_VIEWPORT_DISPLAY = 0x15, MH3U_VIEWPORT_Y0 = 0x1c, MH3U_VIEWPORT_Y1 = 0x24;

/// Whether `camera` is shown on the bottom screen (only top-screen views get widened), MH3U's viewports.
bool Mh3uOnBottomScreen(RCtx* ctx, u32 camera) {
    const u32 scamera = SCAMERA_INSTANCE.Known() ? rd32(ctx, SCAMERA_INSTANCE) : 0;
    if (scamera == 0) {
        return false;
    }
    for (u32 i = 0; i < 8; ++i) {
        const u32 vp = scamera + MH3U_SCAMERA_VIEWPORTS + i * MH3U_VIEWPORT_SIZE;
        if (rd32(ctx, vp + MH3U_VIEWPORT_CAMERA) == camera && rd8(ctx, vp + MH3U_VIEWPORT_DISPLAY) == 1) {
            return true;
        }
    }
    return false;
}

u32 Mh3uRender(RCtx* ctx) {
    return SLTRENDER_INSTANCE.Known() ? rd32(ctx, SLTRENDER_INSTANCE) : 0;
}

/// The HUD's sprites (the clock, the gauges, the name plates: Mh3uSpriteDraw) are drawn through the main view's
/// projection too, so the scale given to it is kept (mh3u_view_sx / sy) and taken off them: MH4U's HUD is flat and never
/// got either (mh3u-paridad: at 4:3 the bars were a quarter wider than the rest of the HUD and the clock off the screen's
/// left; a field of view of 1.3 drew them smaller, toward the middle).
float mh3u_view_sx = 1.0f, mh3u_view_sy = 1.0f;  // (across: the screen's shape times the field of view; up: the latter)
bool Mh3uMainView(RCtx* ctx, u32 camera) {  // (sCamera's first viewport, on the top screen: the hunt's camera)
    const u32 scamera = SCAMERA_INSTANCE.Known() ? rd32(ctx, SCAMERA_INSTANCE) : 0;
    return scamera != 0 && rd32(ctx, scamera + MH3U_SCAMERA_VIEWPORTS + MH3U_VIEWPORT_CAMERA) == camera &&
           rd8(ctx, scamera + MH3U_SCAMERA_VIEWPORTS + MH3U_VIEWPORT_DISPLAY) == 0;
}
/// The cameras' getProjMatrix(out, camera): uAppCameraBase's (uGameCamera, the hunt's camera: 0x003320fc, a copy of the
/// matrix it keeps), uFreeCamera's (0x00265c54) and uMotionCamera's (the cutscenes': 0x00309d6c, which may multiply the
/// perspective by a lens matrix of its own). As GetProjMatrix for MH4U, on the matrix returned (row vectors: clip x
/// comes from column 0, MtMatrix::perspective 0x00938d60 puts the -1 at m23): widescreen narrows or widens clip x by
/// what wide_render's wider picture doesn't already give, fov scales clip x and y alike (perspective matrices only).
/// Columns, not m00 and m11 alone, so that uMotionCamera's lens matrix is scaled with them.
void Mh3uProjMatrix(RCtx* ctx, RFunc original) {
    const u32 out = ctx->r[0];
    const u32 camera = ctx->r[1];
    original(ctx);
    const bool wanted = options.widescreen > 0.0f || options.fov != 1.0f;
    float sx = 1.0f, sy = 1.0f;  // (what this call scales clip x and y by)
    struct Keep {  // (the main view's scale, kept for the HUD's sprites, whatever way this returns)
        RCtx* ctx;
        u32 camera, out;
        const float &sx, &sy;
        ~Keep() {
            if (ReadF32(ctx, out + 60) == 0.0f && Mh3uMainView(ctx, camera)) mh3u_view_sx = sx, mh3u_view_sy = sy;
        }
    } keep{ctx, camera, out, sx, sy};
    if ((!wanted && !HuntAids::DamageNumbersOn()) || Mh3uOnBottomScreen(ctx, camera)) {
        return;
    }
    const auto scale_column = [ctx, out](u32 column, float k) {
        for (u32 row = 0; row < 4; ++row) {
            const u32 at = out + (row * 4 + column) * 4;
            WriteF32(ctx, at, ReadF32(ctx, at) * k);
        }
    };
    const float rendered = WideRows() ? WideRows() / static_cast<float>(LCD_COLUMNS) : NATIVE_ASPECT;
    if (options.trace_layout) {  // (research: the cameras whose matrices this sees, once each)
        static std::vector<u32> seen;
        const u32 vt = rd32(ctx, camera);
        if (seen.size() < 32 && std::find(seen.begin(), seen.end(), vt) == seen.end()) {
            seen.push_back(vt);
            std::string m;
            for (int i = 0; i < 16; ++i) m += fmt::format(" {:.3f}", ReadF32(ctx, out + 4 * i));
            LogInfo("picture: projection of camera {:08x} (vtable {:08x}):{}", camera, vt, m);
        }
    }
    if (options.widescreen > 0.0f && std::fabs(options.widescreen - rendered) > 0.001f) {
        scale_column(0, rendered / options.widescreen);
        sx *= rendered / options.widescreen;
    }
    if (options.fov != 1.0f && ReadF32(ctx, out + 60) == 0.0f) {
        const float m11 = ReadF32(ctx, out + 20);
        if (m11 > 0.0f) {
            const float fovy = 2.0f * std::atan(1.0f / m11);
            const float want = std::clamp(fovy * options.fov, 0.05f, 3.0f);
            const float k = std::tan(fovy * 0.5f) / std::tan(want * 0.5f);
            scale_column(0, k);
            scale_column(1, k);
            sx *= k, sy *= k;
        }
    }
    if (HuntAids::DamageNumbersOn()) {
        HuntAids::Projection(ctx, camera, out);  // (where the damage numbers go: hunt_aids.cpp, the matrix as drawn)
    }
}

/// nDraw::Scene::Scene(this, params {columns, rows, ...}) (0x00573a60): MH3U's main scene target is 512 rows, but the
/// game uses its last 400 only (both screens are transferred from row 112): its first 112 rows share their memory with
/// the touch screen's third framebuffer (0x1804b000-0x18070800, the target starting at 0x18054800). A top screen wider than
/// 400 there (16:9 to 21:10: from row 512 - W) would be drawn over that framebuffer, and each transfer of the touch screen
/// into it made those rows stale for the renderer, which loaded them back from memory, a render pass broken each time
/// (mh3u-perf: 2 loads, 112 KB, every frame at 16:9 in Moga Village). So any wider top screen gets 1024 rows, the most a
/// PICA framebuffer has (21:9 is 560 wide), as 21:9 always did: the game places that target clear of the framebuffers.
/// The same picture but for that framebuffer: at 16:9 the touch screen showed noise on its right side during the logos
/// with 512 rows (the top screen's rows over it), none with 1024; the option mh3u_scene_1024 = off gives the 512 back.
/// The console's 400 too (mh3u-rend): the touch screen's transfers into that framebuffer write the target's first 112
/// rows whatever the top screen's width, so at 4:3 (the RG406V) and 5:3 the renderer still saw them go stale: one
/// transfer in three of the touch screen (its third framebuffer; the game draws the touch screen one frame in five) was
/// made at once, waiting for the GPU, and those rows loaded back (Puerto Tanzia in Low-end mode: 6.7 % of the frames
/// 12.4 ms of the renderer's draws instead of 7.5). With 1024 rows the target is clear of the framebuffers at any width.
/// Only sLtRender's constructor's call (its return address), whose parameters are 256 x 512.
void Mh3uSceneCtor(RCtx* ctx, RFunc original) {
    const u32 rows = WideRows() ? WideRows() : TOP_ROWS;  // (the top screen's rows: wide_render's, or the console's 400)
    const u32 params = ctx->r[1];
    const bool big = options.mh3u_scene_1024 || rows > MH3U_SCENE_ROWS;
    if (big && ctx->r[14] == MH3U_SCENE_CTOR_RETURN && rd32(ctx, params + 4) == MH3U_SCENE_ROWS) {
        wr32(ctx, params + 4, MAX_WIDE_ROWS);
        LogInfo("wide render: scene target {}x{}", rd32(ctx, params), MAX_WIDE_ROWS);
    }
    original(ctx);
}

/// nDraw::DisplayBuffer::init(this, columns, rows, format, ...) (0x0056e3a8; sLtRender::initGraphics 0x0098e080 makes
/// the top screen's 240 x 400 for each eye, the bottom screen's 240 x 320): as DisplayBufferInit for MH4U.
void Mh3uDisplayBufferInit(RCtx* ctx, RFunc original) {
    const u32 rows = WideRows();
    if (rows && ctx->r[1] == LCD_COLUMNS && ctx->r[2] == TOP_ROWS) {
        ctx->r[2] = rows;
        P().SetTopScreenRows(rows);
    }
    original(ctx);
}

std::vector<OwnerRegion> mh3u_owner_regions;  // (as owner_regions, MH3U's viewports)

/// sCamera::Viewport::updateRegion(this) (0x00881478): as UpdateRegion for MH4U, on MH3U's viewport fields. Its regions
/// of the 400 rows (modes 0 and 2..9) are scaled to the wider picture; a region its owner set is moved by half the
/// extra rows, once; mode 0xa's rectangle is setupDraw's (a tunable).
void Mh3uUpdateRegion(RCtx* ctx, RFunc original) {
    const u32 viewport = ctx->r[0];
    original(ctx);
    const u32 rows = WideRows();
    if (!rows || rd8(ctx, viewport + MH3U_VIEWPORT_DISPLAY) != 0 || rd32(ctx, viewport + MH3U_VIEWPORT_SCENE_TEXTURE) != 0) {
        return;
    }
    const u32 render = Mh3uRender(ctx);
    if (render == 0 || rd32(ctx, render + MH3U_SLTRENDER_SCENE) == 0) {
        return;
    }
    const u8 mode = rd8(ctx, viewport + MH3U_VIEWPORT_MODE);
    if (mode == 0 || (mode >= 2 && mode <= 9)) {
        for (const u32 field : {MH3U_VIEWPORT_Y0, MH3U_VIEWPORT_Y1}) {
            const u32 y = rd32(ctx, viewport + field);
            if (y <= TOP_ROWS) {
                wr32(ctx, viewport + field, y * rows / TOP_ROWS);
            }
        }
        return;
    }
    if (mode == 0xa) {
        return;
    }
    const u32 y0 = rd32(ctx, viewport + MH3U_VIEWPORT_Y0);
    const u32 y1 = rd32(ctx, viewport + MH3U_VIEWPORT_Y1);
    auto it = std::find_if(mh3u_owner_regions.begin(), mh3u_owner_regions.end(),
                           [viewport](const OwnerRegion& r) { return r.viewport == viewport; });
    if (it != mh3u_owner_regions.end() && it->y0 == y0 && it->y1 == y1) {
        return;
    }
    if (y0 > y1 || y1 > TOP_ROWS) {
        return;
    }
    const bool whole = y0 == 0 && y1 == TOP_ROWS;
    const u32 shift = (rows - TOP_ROWS) / 2;
    const u32 new_y0 = whole ? 0 : y0 + shift;
    const u32 new_y1 = whole ? rows : y1 + shift;
    wr32(ctx, viewport + MH3U_VIEWPORT_Y0, new_y0);
    wr32(ctx, viewport + MH3U_VIEWPORT_Y1, new_y1);
    if (it == mh3u_owner_regions.end()) {
        mh3u_owner_regions.push_back({viewport, new_y0, new_y1});
    } else {
        it->y0 = new_y0;
        it->y1 = new_y1;
    }
}

/// As WideTransfer for MH4U: the display transfer copies the top screen from row 112 of the 512-row target (the
/// constructor's), so W rows start at height - W (the console's 400 with no wide picture: 624 of a 1024-row target,
/// Mh3uSceneCtor; 112 of 512, the game's own); the bottom screen, drawn in rows 80..400, from height - 400.
void Mh3uWideTransfer(RCtx* ctx, u32 render) {
    const u32 rows = WideRows() ? WideRows() : TOP_ROWS;
    const u32 scene = render ? rd32(ctx, render + MH3U_SLTRENDER_SCENE) : 0;
    const u32 target = scene ? rd32(ctx, scene + SCENE_TARGET) : 0;
    const u32 textures = target ? rd32(ctx, target + TARGET_TEXTURES) : 0;
    const u32 texture = textures ? rd32(ctx, textures) : 0;
    if (texture == 0) {
        return;
    }
    const u32 height = (rd32(ctx, texture + TEXTURE_SIZE) >> 16) & 0x7FFF;
    if (height >= rows && rd32(ctx, render + MH3U_SLTRENDER_TOP_TRANSFER_Y) != height - rows) {
        wr32(ctx, render + MH3U_SLTRENDER_TOP_TRANSFER_Y, height - rows);
        LogInfo("wide render: top screen transferred from row {} of {}", height - rows, height);
    }
    if (height >= TOP_ROWS && rd32(ctx, render + MH3U_SLTRENDER_BOTTOM_TRANSFER_Y) != height - TOP_ROWS) {
        wr32(ctx, render + MH3U_SLTRENDER_BOTTOM_TRANSFER_Y, height - TOP_ROWS);
        LogInfo("wide render: bottom screen transferred from row {} of {}", height - TOP_ROWS, height);
    }
}

// The screen filters (the options bloom, motion_blur, depth_of_field, radial_blur, haze, as MH4U's): MH3U has the same
// uFilter classes, each drawn by the scene through render(this, cDraw*), vtable slot 12 (+0x30), plus subclasses of its
// own (uAppBloomFilter, uAppHazeFilter). A filter turned off gets uFilter's empty render (EMPTY_RENDER, port_map.txt:
// 0x00564ee8, `bx lr`) in that slot, as the effects below and MH4U's effects do: the code reads the slot at every call, so
// it applies at once, and on puts the class's own render back. No hook point is needed for it. The vtables were found
// from each class's getDTI (slot 5), config/mt_classes.tsv. MH3U's filters with no option of MH4U's stay as they are
// (uBlurFilter, uSunFilter, uFlashFilter, uFadeFilter, colour correction, cross-fade, image plane, outline, TV noise).
struct Mh3uVtableSlot {
    u32 vtable;        // MH3U USA v1.0
    u32 render;        // its slot 12 as the game has it
    const char* name;  // in the log
    u32 option;        // the FILTER_* bit that leaves it out (0: Effects)
};
constexpr Mh3uVtableSlot MH3U_FILTERS[] = {
    {0x00b51938, 0x002a8928, "uBloomFilter", FILTER_BLOOM},
    {0x00b57788, 0x002a8928, "uAppBloomFilter", FILTER_BLOOM},
    {0x00b5baa8, 0x003f8858, "uMotionBlurFilter", FILTER_MOTION_BLUR},
    {0x00b5a2c4, 0x002c106c, "uMultiBlurFilter", FILTER_MOTION_BLUR},
    {0x00b4d704, 0x0019dd34, "uDOFFilter", FILTER_DEPTH_OF_FIELD},
    {0x00b51988, 0x002a9ca8, "uBokehFilter", FILTER_DEPTH_OF_FIELD},
    {0x00b5baf8, 0x003fa8a8, "uRadialBlurFilter", FILTER_RADIAL_BLUR},
    {0x00b50458, 0x00272a90, "uHazeFilter", FILTER_HAZE},
    {0x00b555c0, 0x00272a90, "uAppHazeFilter", FILTER_HAZE},
};
// The particle effects (the option effects, MH4U's S27): MH3U's effect classes that draw (uBaseEffect's subclasses; its 61
// uEffectNNN controllers draw nothing of their own, their slot 12 is already the empty render)
constexpr Mh3uVtableSlot MH3U_EFFECTS[] = {
    {0x00b68324, 0x008a1c68, "uEffect", 0},        {0x00b502b4, 0x008a1c68, "uGameEffect", 0},
    {0x00b58f60, 0x008a1c68, "uGameEffectChild", 0}, {0x00b55610, 0x008a1c68, "uEffectControl", 0},
    {0x00b5286c, 0x002c3c7c, "uStageEffect", 0},   {0x00b53634, 0x008a1c64, "uCameraEffect", 0},
    {0x00b54b28, 0x0031552c, "uSimpleEffect", 0},
};
u32 mh3u_filters_applied = 0xFFFFFFFF;  // filters_off as last written (all ones: not yet)
int mh3u_effects_applied = -1;          // 1 drawn, 0 left out, -1 not yet
bool mh3u_slots_unknown = false;        // a vtable held neither value: another version of the game, nothing written

/// Writes slot 12 of each class in `classes`: its own render where `drawn(class)`, else the empty render. False (and
/// nothing written) when a slot holds neither.
template <size_t N, typename Drawn>
bool Mh3uWriteSlots(RCtx* ctx, const Mh3uVtableSlot (&classes)[N], Drawn drawn) {
    if (!EMPTY_RENDER.Known()) {
        return false;
    }
    for (const Mh3uVtableSlot& c : classes) {
        const u32 slot = rd32(ctx, c.vtable + 0x30);
        if (slot != c.render && slot != u32{EMPTY_RENDER}) {
            LogInfo("picture: {}'s vtable {:08x} holds {:08x} where {:08x} was expected (another version of the game): "
                    "the screen filters and Effects options do nothing", c.name, c.vtable, slot, c.render);
            return false;
        }
    }
    for (const Mh3uVtableSlot& c : classes) {
        wr32(ctx, c.vtable + 0x30, drawn(c) ? c.render : u32{EMPTY_RENDER});
    }
    return true;
}

u32 mh3u_flags_logged[2] = {0xFFFFFFFF, 0xFFFFFFFF};
double mh3u_flags_logged_at = -10.0;

/// Once a frame (Mh3uFrameTiming, before the game's frame): the render settings, the wide picture's transfer rows, the
/// screen filters and the effects.
void Mh3uGfxFrame(RCtx* ctx) {
    const u32 render = Mh3uRender(ctx);
    if (render != 0) {
        // dither and stereoscopic 3D off, as MH4U's Frame (the game reads the applied word; the edit word is what its
        // own setters write, so both): without the dither bits no dither pattern, which an upscaled picture shows as a
        // screen-door grid; without the stereo bit the top screen is drawn once
        const u32 clear = options.render_clear | (options.dither ? 0 : RENDER_DITHER) | (options.stereo_3d ? 0 : RENDER_STEREO);
        if (options.render_set || clear) {
            const u32 edit = rd32(ctx, render + MH3U_RENDER_EDIT);
            wr32(ctx, render + MH3U_RENDER_EDIT, (edit & ~clear) | options.render_set);
            const u32 applied = rd32(ctx, render + MH3U_RENDER_FLAGS);
            const u32 applied_clear = clear & (RENDER_DITHER | RENDER_STEREO);
            if (applied & applied_clear) {
                wr32(ctx, render + MH3U_RENDER_FLAGS, applied & ~applied_clear);
            }
        }
        Mh3uWideTransfer(ctx, render);
        if (options.trace_filters) {
            const double now = EmulatedSeconds();
            const u32 words[2] = {rd32(ctx, render + MH3U_RENDER_EDIT), rd32(ctx, render + MH3U_RENDER_FLAGS)};
            if ((words[0] != mh3u_flags_logged[0] || words[1] != mh3u_flags_logged[1]) && now - mh3u_flags_logged_at >= 1.0) {
                LogInfo("picture at {:.1f} s: render settings {:08X}, applied {:08X}", now, words[0], words[1]);
                mh3u_flags_logged[0] = words[0], mh3u_flags_logged[1] = words[1];
                mh3u_flags_logged_at = now;
            }
        }
    }
    if (mh3u_slots_unknown) {
        return;
    }
    if (options.filters_off != mh3u_filters_applied) {
        if (!Mh3uWriteSlots(ctx, MH3U_FILTERS, [](const Mh3uVtableSlot& c) { return !(options.filters_off & c.option); })) {
            mh3u_slots_unknown = true;
            return;
        }
        if (mh3u_filters_applied != 0xFFFFFFFF || options.filters_off) {
            std::string off;
            for (const auto& [name, bit] : FILTER_OPTIONS) {
                if (options.filters_off & bit) off += (off.empty() ? "" : ", ") + std::string(name);
            }
            LogInfo("screen filters off: {}", off.empty() ? "none" : off);
        }
        mh3u_filters_applied = options.filters_off;
    }
    const int effects = options.effects ? 1 : 0;
    if (effects != mh3u_effects_applied) {
        if (!Mh3uWriteSlots(ctx, MH3U_EFFECTS, [effects](const Mh3uVtableSlot&) { return effects != 0; })) {
            mh3u_slots_unknown = true;
            return;
        }
        if (mh3u_effects_applied != -1 || !effects) {
            LogInfo("effects: the game's particle effects {} ({} classes)", effects ? "drawn" : "left out",
                    std::size(MH3U_EFFECTS));
        }
        mh3u_effects_applied = effects;
    }
}

// The 2D picture (rGUI: the menus, the talk windows and the HUD's cockpit units, all uGUI's subclasses): uGUI's draw
// (slot 24, 0x00549748, every class's) puts the projection from the cDraw's viewport (+0x12c: x0, y0, x1, y1, rows
// across the top screen) in four constants at [cDraw+0x790] (2/columns, -2/rows, and the offsets of the two axes), then
// draws the unit's instance (0x002957b4), whose commands the render thread reads later. The GUI keeps its pixel size,
// so in a wider top screen it stays at the end where row 0 is: the hook moves it by half the extra rows (the 400-pixel
// layout centred, as MH4U's rLayout is), and with hud_edges the HUD's side units out to their edge (EdgeRows). A unit
// is told by its vtable (each cockpit part is a class of its own, config/mt_classes.tsv).
constexpr u32 MH3U_CDRAW_VIEWPORT = 0x12c;        // s32 x0, y0, x1, y1 (sCamera's region of the viewport drawn)
constexpr u32 MH3U_CDRAW_GUI_CONSTANTS = 0x790;   // u32: where 0x00549748 wrote its four floats
enum class Mh3uHudSide : u8 { Middle, Left, Right };
struct Mh3uHudUnit {
    u32 vtable;
    const char* name;
    Mh3uHudSide side;
};
// (seen in the Moga Woods with a sword and shield: Main, Item and Map; the weapons' gauges are placed by where the game
// draws them, under the sharpness or beside the items, not seen yet)
constexpr Mh3uHudUnit MH3U_HUD_UNITS[] = {
    {0x00b55930, "uIDCockpitMain", Mh3uHudSide::Left},        // health, stamina, sharpness, the clock
    {0x00b53c48, "uIDCockpitAir", Mh3uHudSide::Left},         // the air gauge under water
    {0x00b53d58, "uIDCockpitAxe", Mh3uHudSide::Left},         // the Switch Axe's gauge (under the sharpness)
    {0x00b594a0, "uIDCockpitWsword", Mh3uHudSide::Left},      // the Long Sword's spirit gauge
    {0x00b5c2b0, "uIDCockpitGunLance", Mh3uHudSide::Left},    // the Gunlance's shells
    {0x00b55a40, "uIDCockpitPipe", Mh3uHudSide::Left},        // the Hunting Horn's notes
    {0x00b55820, "uIDCockpitItem", Mh3uHudSide::Right},       // the item bar
    {0x00b53e68, "uIDCockpitBow", Mh3uHudSide::Right},        // the bow's coatings (beside the items, as MH4U's ui205)
    {0x00b59390, "uIDCockpitGunner", Mh3uHudSide::Right},     // a bowgun's ammo
    {0x00b59170, "uIDCockpitBallet", Mh3uHudSide::Right},
    {0x00b54088, "uIDCockpitMap", Mh3uHudSide::Right},        // the map
};
// The maps' signs (uIDCockpitSign: the hunters' arrows and the monsters' icons on the area map, placed from the map's
// place in its 400 pixels) go to the map's side with it (mh3u_hud.cpp's DRAW_PARTS: the map's part too)
constexpr u32 MH3U_SIGN_VTABLE = 0x00b55b50;
Mh3uHudSide Mh3uSideOf(u32 vtable) {
    if (vtable == MH3U_SIGN_VTABLE) {  // (its names with the bars, SignNamePass; its map signs at the map's side)
        if (Mh3u::SignNamePass()) return Mh3uHudSide::Left;
        return Mh3u::SignAtMapSide() ? Mh3uHudSide::Right : Mh3uHudSide::Middle;
    }
    for (const Mh3uHudUnit& u : MH3U_HUD_UNITS) {
        if (u.vtable == vtable) return u.side;
    }
    return Mh3uHudSide::Middle;
}
std::vector<u32> mh3u_gui_traced;  // (trace_layout: the classes logged)
constexpr float MH3U_GUI_LEFT = 1.0f;  // which way GUI rows grow on the screen: 1, row 0 at the screen's left
// The unit being drawn on the wide top screen: its shift along the rows (0: none), and whether uGUI's draw's own block
// still waits for it (it is written before the unit's instance is drawn, 0x002957b4; the instance's animations can
// write blocks of their own, nGUI::Draw's begin 0x00539d48, which take the same shift)
float mh3u_gui_shift = 0.0f;
bool mh3u_gui_block_pending = false;

/// Moves the constants block [cDraw+0x790] by `shift` rows, if the block's viewport (+0x12c) is the wide top screen's.
void Mh3uGuiShiftBlock(RCtx* ctx, u32 cdraw, float shift) {
    const u32 rows = WideRows();
    const s32 y0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 4));
    const s32 y1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 12));
    const u32 constants = rd32(ctx, cdraw + MH3U_CDRAW_GUI_CONSTANTS);
    if (rows == 0 || y1 - y0 != static_cast<s32>(rows) || constants == 0) {
        return;
    }
    // (the rows' scale as written, -2 / rows: the offset moves by that much a row)
    WriteF32(ctx, constants + 12, ReadF32(ctx, constants + 12) + shift * ReadF32(ctx, constants + 4));
}

// (mh3u-ui) The HUD's size, moves and hiding (hud_size, hud_<part>: the layout editor's), for the unit being drawn on the
// top screen (Mh3u::HudDraw, mh3u_hud.cpp): its block's scale times `size` round the part's corner, then moved, in the
// GUI's own axes (rows across the top screen from its left, MH3U_GUI_LEFT; columns up it from its bottom: uGUI's
// draw writes +2 / columns, so a higher column is higher on the screen, as MH4U's layout rows are)
struct Mh3uHudPlace {
    bool on = false;
    float size = 1.0f, dx = 0.0f, dy = 0.0f, ax = 0.0f, ay = 0.0f;
};
Mh3uHudPlace mh3u_hud_place;
constexpr bool MH3U_GUI_COLUMNS_UP = true;
void Mh3uHudBlock(RCtx* ctx, u32 cdraw) {
    const Mh3uHudPlace& h = mh3u_hud_place;
    const u32 constants = rd32(ctx, cdraw + MH3U_CDRAW_GUI_CONSTANTS);
    const s32 y0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 4));
    const s32 y1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 12));
    const u32 rows = WideRows() ? WideRows() : TOP_ROWS;
    if (!h.on || constants == 0 || y1 - y0 != static_cast<s32>(rows)) return;  // (the top screen's pass only)
    const float c0 = ReadF32(ctx, constants), c1 = ReadF32(ctx, constants + 4);
    // the part's corner in the GUI's axes: its row (the screen's x) and its column (from the screen's bottom)
    const float gy = MH3U_GUI_LEFT > 0.0f ? h.ax : static_cast<float>(TOP_ROWS) - h.ax;
    // (the Moga Woods, woods2: the GUI's column 0 is the screen's top, its columns counted up: -y)
    const float gx = MH3U_GUI_COLUMNS_UP ? -h.ay : h.ay;
    const float mx = MH3U_GUI_COLUMNS_UP ? -h.dy : h.dy, my = MH3U_GUI_LEFT > 0.0f ? h.dx : -h.dx;
    WriteF32(ctx, constants, c0 * h.size);
    WriteF32(ctx, constants + 4, c1 * h.size);
    WriteF32(ctx, constants + 8, ReadF32(ctx, constants + 8) + gx * c0 * (1.0f - h.size) + mx * c0);
    WriteF32(ctx, constants + 12, ReadF32(ctx, constants + 12) + gy * c1 * (1.0f - h.size) + my * c1);
}
/// The block's shift (the wide top screen) and the HUD's place (mh3u-ui), as the unit being drawn has them
void Mh3uGuiPlaceBlock(RCtx* ctx, u32 cdraw, float shift) {
    Mh3uGuiShiftBlock(ctx, cdraw, shift);
    Mh3uHudBlock(ctx, cdraw);
}

// The single screen's touch parts (Mh3u::OneScreenParts, mh3u_onescreen.cpp). A uGUI whose flags (+0x158) have bit 3
// draws each node of its instance's tree only on the screens its mask says (node + 0x78, bit n: screen n; the instance
// draw 0x002957b4 tests 1 << the screen being drawn, cDraw + 0x1d4 bits 1 to 5, against it, and leaves a node out with
// its children). Such a unit drawn on both screens (uIDCommonFrame: the touch screen's help line and the item's panel
// with the quantity, as "Take how many?" and the Market's "How many do you want to buy?" ask; a facility's list title)
// keeps those nodes off the top screen: given the top screen's viewport alone, nothing of them showed (the uIDPanel tried
// in mh3u-menus2 holds no instance at all). For a part, the unit is drawn once more in the top screen's pass with that
// pass's screen number set to the touch screen's (the block [0x00ca8924]: + 0x74 u8 the top screen's number, + 0x75 the
// touch screen's) and only the part's nodes (and their parents) left on, placed as the single screen's menus are
// (OneScreenDraw's size and move).
constexpr u32 MH3U_GUI_FLAGS2 = 0x158;          // bit 3: the nodes drawn by their screens' masks
constexpr u32 MH3U_GUI_INSTANCE = 0xf4;         // the instance's root node
constexpr u32 MH3U_NODE_CHILD = 0x80, MH3U_NODE_NEXT = 0x84, MH3U_NODE_SCREENS = 0x78;
constexpr u32 MH3U_CDRAW_SCREEN = 0x1d4;        // bits 1 to 5: the screen the pass draws (0 the top one)
constexpr u32 MH3U_SCREENS = 0x00ca8924;        // [ ] + 0x75: u8 the touch screen's number
constexpr int MH3U_PART_NODES_MAX = 512;
bool Mh3uGuiNode(u32 p) {
    return (p & 3) == 0 && p >= 0x08000000 && p < 0x20000000;
}
struct Mh3uPartPass {
    bool on = false;
    float size = 1.0f, dx = 0.0f, dy = 0.0f;
};
Mh3uPartPass mh3u_part_pass;  // (the touch part being drawn: Mh3uGuiDrawOne places it)
/// A unit's nodes for the touch screen alone (mask 2 of its screens), in the tree's order, each with its parents
void Mh3uTouchNodes(RCtx* ctx, u32 root, std::vector<u32>& touch, std::vector<std::vector<u32>>& chains) {
    struct Item {
        u32 node;
        int parent;
    };
    std::vector<Item> items;  // (every node met, its parent's index)
    std::vector<std::pair<u32, int>> stack;
    if (Mh3uGuiNode(rd32(ctx, root + MH3U_NODE_CHILD))) stack.push_back({rd32(ctx, root + MH3U_NODE_CHILD), -1});
    while (!stack.empty() && static_cast<int>(items.size()) < MH3U_PART_NODES_MAX) {
        auto [n, parent] = stack.back();
        stack.pop_back();
        std::vector<std::pair<u32, int>> children;  // (a level's children after the level, in order)
        for (int k = 0; Mh3uGuiNode(n) && k < MH3U_PART_NODES_MAX; ++k, n = rd32(ctx, n + MH3U_NODE_NEXT)) {
            const int me = static_cast<int>(items.size());
            items.push_back({n, parent});
            if ((rd32(ctx, n + MH3U_NODE_SCREENS) & 0xffff) == 2) {
                touch.push_back(n);
                std::vector<u32> chain;
                for (int p = parent; p >= 0; p = items[p].parent) chain.push_back(items[p].node);
                chains.push_back(chain);
                continue;  // (its children go with it)
            }
            const u32 child = rd32(ctx, n + MH3U_NODE_CHILD);
            if (Mh3uGuiNode(child)) children.push_back({child, me});
        }
        for (auto it = children.rbegin(); it != children.rend(); ++it) stack.push_back(*it);
    }
}
/// Every node of the tree
void Mh3uAllNodes(RCtx* ctx, u32 root, std::vector<u32>& all) {
    std::vector<u32> stack;
    if (Mh3uGuiNode(rd32(ctx, root + MH3U_NODE_CHILD))) stack.push_back(rd32(ctx, root + MH3U_NODE_CHILD));
    while (!stack.empty() && static_cast<int>(all.size()) < MH3U_PART_NODES_MAX) {
        u32 n = stack.back();
        stack.pop_back();
        for (int k = 0; Mh3uGuiNode(n) && k < MH3U_PART_NODES_MAX; ++k, n = rd32(ctx, n + MH3U_NODE_NEXT)) {
            all.push_back(n);
            const u32 child = rd32(ctx, n + MH3U_NODE_CHILD);
            if (Mh3uGuiNode(child)) stack.push_back(child);
        }
    }
}
void Mh3uGuiDrawOne(RCtx* ctx, RFunc original);
/// One touch part of `unit` drawn on the top screen (the draw's arguments as the hook got them)
void Mh3uGuiDrawTouchPart(RCtx* ctx, RFunc original, u32 unit, u32 cdraw, const Mh3u::OneScreenPart& part) {
    if ((rd32(ctx, unit + MH3U_GUI_FLAGS2) & 8) == 0) return;
    const u32 root = rd32(ctx, unit + MH3U_GUI_INSTANCE);
    const u32 screens = rd32(ctx, MH3U_SCREENS);
    if (!Mh3uGuiNode(root) || !Mh3uGuiNode(screens)) return;
    std::vector<u32> touch;
    std::vector<std::vector<u32>> chains;
    Mh3uTouchNodes(ctx, root, touch, chains);
    if (touch.empty() || part.node >= static_cast<int>(touch.size())) return;
    // the nodes left on: the part's and their parents; every other node that would draw on the touch screen off
    std::vector<u32> keep;
    for (std::size_t i = 0; i < touch.size(); ++i) {
        if (part.node >= 0 && static_cast<int>(i) != part.node) continue;
        keep.push_back(touch[i]);
        keep.insert(keep.end(), chains[i].begin(), chains[i].end());
        // (and everything under it: its text, as the help line's)
        if (Mh3uGuiNode(rd32(ctx, touch[i] + MH3U_NODE_CHILD))) {
            std::vector<u32> under;
            Mh3uAllNodes(ctx, touch[i], under);
            keep.insert(keep.end(), under.begin(), under.end());
        }
    }
    std::vector<u32> all;
    Mh3uAllNodes(ctx, root, all);
    std::vector<std::pair<u32, u32>> saved;
    const u32 touch_screen = rd8(ctx, screens + 0x75) & 0x1fu;
    for (const u32 n : all) {
        if (std::find(keep.begin(), keep.end(), n) != keep.end()) continue;
        const u32 mask = rd32(ctx, n + MH3U_NODE_SCREENS);
        if ((mask >> touch_screen) & 1) {
            saved.push_back({n, mask});
            wr32(ctx, n + MH3U_NODE_SCREENS, mask & ~(1u << touch_screen));
        }
    }
    const u32 was = rd32(ctx, cdraw + MH3U_CDRAW_SCREEN);
    wr32(ctx, cdraw + MH3U_CDRAW_SCREEN, (was & ~0x3eu) | (touch_screen << 1));
    const Mh3uPartPass outer = mh3u_part_pass;
    mh3u_part_pass = {true, part.size, part.dx, part.dy};
    Mh3uGuiDrawOne(ctx, original);
    mh3u_part_pass = outer;
    wr32(ctx, cdraw + MH3U_CDRAW_SCREEN, was);
    for (const auto& [n, mask] : saved) wr32(ctx, n + MH3U_NODE_SCREENS, mask);
}

/// uGUI's draw (slot 24) of one unit: works out the unit's shift, which the two hooks below apply.
void Mh3uGuiDrawOne(RCtx* ctx, RFunc original) {
    const u32 unit = ctx->r[0];
    const u32 cdraw = rd32(ctx, ctx->r[1] + 4);
    const u32 rows = WideRows();
    float shift = 0.0f;
    Mh3uHudSide side = Mh3uHudSide::Middle;
    if (rows && cdraw) {
        const s32 y0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 4));
        const s32 y1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 12));
        if (y1 - y0 == static_cast<s32>(rows)) {  // (not the touch screen, a sub-view, or the console's own width)
            const u32 vtable = rd32(ctx, unit);
            side = options.hud_edges ? Mh3uSideOf(vtable) : Mh3uHudSide::Middle;
            // (the bow's coatings' panel is the base uIDCockpit's: beside the items, Mh3u::HudBow)
            if (options.hud_edges && vtable == 0x00b4f038 && Mh3u::HudBow()) side = Mh3uHudSide::Right;
            shift = static_cast<float>(rows - TOP_ROWS) * 0.5f;  // (the 400 centred)
            if (side != Mh3uHudSide::Middle) {
                const float edge = static_cast<float>(EdgeRows() ? EdgeRows() : rows);  // (the HUD's width)
                const float margin = (static_cast<float>(rows) - edge) * 0.5f;
                const bool low = (side == Mh3uHudSide::Left) == (MH3U_GUI_LEFT > 0.0f);  // (the side at row 0)
                shift = low ? margin : margin + (edge - static_cast<float>(TOP_ROWS));
            }
            if (options.trace_layout &&
                std::find(mh3u_gui_traced.begin(), mh3u_gui_traced.end(), vtable) == mh3u_gui_traced.end()) {
                mh3u_gui_traced.push_back(vtable);
                LogInfo("picture: GUI unit {:08x} (vtable {:08x}) on the wide top screen, moved {:.0f} rows{}", unit, vtable,
                        shift, side == Mh3uHudSide::Left ? " (HUD, left)" : side == Mh3uHudSide::Right ? " (HUD, right)" : "");
            }
        }
    }
    // (mh3u-ui) the HUD's parts: hidden ones not drawn, the others at their size and place; on the top screen alone
    // (the maps' signs, a part of the map's, are drawn on the touch panels' map too)
    Mh3uHudPlace hud;
    bool hud_hidden = false;
    bool top_pass = false;
    bool touch_nodes = false;  // (a placed unit whose nodes say their screens: drawn with the touch screen's number)
    if (cdraw) {
        const s32 y0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 4));
        const s32 y1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 12));
        top_pass = y1 - y0 == static_cast<s32>(rows ? rows : TOP_ROWS);
    }
    if (top_pass) {
        // (the unit's GUI's place, for trace_state's log: its instance's root matrix, the last row)
        const u32 instance = rd32(ctx, unit + 0xf4);
        const bool placed = instance >= 0x08000000 && instance < 0x20000000 && (instance & 3) == 0;
        const float tx = placed ? ReadF32(ctx, instance + 0x10 + 0x30) : 0.0f;
        const float ty = placed ? ReadF32(ctx, instance + 0x10 + 0x34) : 0.0f;
        // (the single screen: a unit of the touch screen's menu drawn here, at its place, mh3u_onescreen.cpp; a touch
        // part of a unit of both screens, Mh3uGuiDrawTouchPart)
        if (mh3u_part_pass.on) {
            hud.on = true, hud.size = mh3u_part_pass.size, hud.dx = mh3u_part_pass.dx, hud.dy = mh3u_part_pass.dy;
            hud.ax = 0.0f, hud.ay = 0.0f;
        } else if (Mh3u::OneScreenDraw(unit, &hud.size, &hud.dx, &hud.dy)) {
            hud.on = true, hud.ax = 0.0f, hud.ay = 0.0f;
            // (its nodes by their screens: those of the touch screen's, as there)
            if ((rd32(ctx, unit + MH3U_GUI_FLAGS2) & 8) != 0) touch_nodes = true;
        } else {
            hud.on = Mh3u::HudGuiDraw(unit, rd32(ctx, unit), placed, tx, ty, &hud.size, &hud.dx, &hud.dy, &hud.ax,
                                      &hud.ay, &hud_hidden);
        }
    }
    if (hud.on && hud_hidden) return;
    const Mh3uHudPlace outer_hud = mh3u_hud_place;
    mh3u_hud_place = hud;
    const float outer = mh3u_gui_shift;
    mh3u_gui_shift = shift;
    mh3u_gui_block_pending = shift != 0.0f || hud.on;
    const u32 screen_was = touch_nodes ? rd32(ctx, cdraw + MH3U_CDRAW_SCREEN) : 0;
    if (touch_nodes) {
        const u32 screens = rd32(ctx, MH3U_SCREENS);
        if (Mh3uGuiNode(screens)) {
            wr32(ctx, cdraw + MH3U_CDRAW_SCREEN, (screen_was & ~0x3eu) | ((rd8(ctx, screens + 0x75) & 0x1fu) << 1));
        } else {
            touch_nodes = false;
        }
    }
    original(ctx);
    if (touch_nodes) wr32(ctx, cdraw + MH3U_CDRAW_SCREEN, screen_was);
    if (mh3u_gui_block_pending && cdraw) {  // (no instance drawn: the block as the function left it)
        Mh3uGuiPlaceBlock(ctx, cdraw, shift);
    }
    mh3u_gui_block_pending = false;
    mh3u_gui_shift = outer;
    mh3u_hud_place = outer_hud;
}

// The sign unit (uIDCockpitSign) draws the hunters' names on their plates over the bars and the area map's arrows and
// icons: on the top screen it is drawn twice, its name nodes with the bars' part and side (the others hidden), then the
// others with the map's (the names hidden), as its own draw (0x0034bed0) hides nodes by their + 0x70 bit 0 for its
// second pass. Its nodes: unit + 0x1f8, 36 pointers (0x1c to 0x1e are its second pass's, already hidden in this one).
constexpr u32 MH3U_SIGN_NODES = 0x1f8, MH3U_NODE_FLAGS = 0x70;
constexpr int MH3U_SIGN_NODE_COUNT = 36;
bool Mh3uNodePointer(u32 p) {
    return (p & 3) == 0 && p >= 0x08000000 && p < 0x20000000;
}
/// Hides the nodes of `mask` (bit 0 of + 0x70 cleared) and returns their bits as they were (bit i: node i visible).
std::uint64_t Mh3uSignHide(RCtx* ctx, u32 unit, std::uint64_t mask) {
    std::uint64_t was = 0;
    for (int i = 0; i < MH3U_SIGN_NODE_COUNT; ++i) {
        const u32 node = rd32(ctx, unit + MH3U_SIGN_NODES + 4 * i);
        if (!Mh3uNodePointer(node)) continue;
        const u32 flags = rd32(ctx, node + MH3U_NODE_FLAGS);
        if (flags & 1) was |= std::uint64_t{1} << i;
        if ((mask >> i) & 1) wr32(ctx, node + MH3U_NODE_FLAGS, flags & ~1u);
    }
    return was;
}
/// Puts back the bits of the nodes of `mask` as `was` had them.
void Mh3uSignShow(RCtx* ctx, u32 unit, std::uint64_t mask, std::uint64_t was) {
    for (int i = 0; i < MH3U_SIGN_NODE_COUNT; ++i) {
        if (!((mask >> i) & 1)) continue;
        const u32 node = rd32(ctx, unit + MH3U_SIGN_NODES + 4 * i);
        if (!Mh3uNodePointer(node)) continue;
        const u32 flags = rd32(ctx, node + MH3U_NODE_FLAGS);
        wr32(ctx, node + MH3U_NODE_FLAGS, ((was >> i) & 1) ? (flags | 1u) : (flags & ~1u));
    }
}
/// (research, mh3u_sign's dump) each node's pointer, flags and first 0x80 bytes, once
void Mh3uSignDump(RCtx* ctx, u32 unit) {
    for (int i = 0; i < MH3U_SIGN_NODE_COUNT; ++i) {
        const u32 node = rd32(ctx, unit + MH3U_SIGN_NODES + 4 * i);
        std::string bytes;
        if (Mh3uNodePointer(node)) {
            for (u32 k = 0; k < 0x80; k += 4) bytes += fmt::format(" {:08x}", rd32(ctx, node + k));
        }
        LogInfo("picture: sign node {} at {:08x}:{}", i, node, bytes);
    }
}

/// uGUI's draw (slot 24): the unit as Mh3uGuiDrawOne draws it; the sign unit's names and map signs apart on the top screen.
void Mh3uGuiDrawUnit(RCtx* ctx, RFunc original);
void Mh3uGuiDraw(RCtx* ctx, RFunc original) {
    const u32 unit = ctx->r[0];
    const u32 cdraw = rd32(ctx, ctx->r[1] + 4);
    Mh3u::OneScreenPart parts[4];
    int count = 0;
    if (cdraw) {
        const s32 y0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 4));
        const s32 y1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 12));
        const u32 rows = WideRows() ? WideRows() : TOP_ROWS;
        if (y1 - y0 == static_cast<s32>(rows)) count = Mh3u::OneScreenParts(unit, parts, 4);
    }
    if (count == 0) {
        Mh3uGuiDrawUnit(ctx, original);
        return;
    }
    u32 args[4], lr = ctx->r[14];
    std::copy(ctx->r, ctx->r + 4, args);
    Mh3uGuiDrawUnit(ctx, original);
    for (int i = 0; i < count; ++i) {  // (the unit's touch parts, over it: mh3u_onescreen.cpp)
        std::copy(args, args + 4, ctx->r);
        ctx->r[14] = lr;
        Mh3uGuiDrawTouchPart(ctx, original, unit, cdraw, parts[i]);
    }
}
/// uGUI's draw of one unit (the sign unit's names and map signs apart on the top screen)
void Mh3uGuiDrawUnit(RCtx* ctx, RFunc original) {
    const u32 unit = ctx->r[0];
    const u32 cdraw = rd32(ctx, ctx->r[1] + 4);
    if (rd32(ctx, unit) != MH3U_SIGN_VTABLE || !cdraw) {
        Mh3uGuiDrawOne(ctx, original);
        return;
    }
    const s32 y0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 4));
    const s32 y1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 12));
    const u32 rows = WideRows() ? WideRows() : TOP_ROWS;
    const bool top_pass = y1 - y0 == static_cast<s32>(rows);
    if (top_pass && Mh3u::SignDumpTake()) Mh3uSignDump(ctx, unit);
    const std::uint64_t all = (std::uint64_t{1} << MH3U_SIGN_NODE_COUNT) - 1;
    const std::uint64_t hide = top_pass ? Mh3u::SignHiddenNodes() & all : 0;
    const std::uint64_t names = top_pass ? Mh3u::SignNameNodes() & all & ~hide : 0;
    const std::uint64_t was = hide ? Mh3uSignHide(ctx, unit, hide) : 0;
    if (names == 0) {
        Mh3uGuiDrawOne(ctx, original);
    } else {
        u32 args[4], lr = ctx->r[14];
        std::copy(ctx->r, ctx->r + 4, args);
        const std::uint64_t named = Mh3uSignHide(ctx, unit, all & ~names);  // (the names alone: the bars' part and side)
        Mh3u::SetSignPass(1);
        Mh3uGuiDrawOne(ctx, original);
        Mh3uSignShow(ctx, unit, all & ~names, named);
        std::copy(args, args + 4, ctx->r);
        ctx->r[14] = lr;
        const std::uint64_t rest = Mh3uSignHide(ctx, unit, names);  // (the rest: the map's)
        Mh3u::SetSignPass(2);
        Mh3uGuiDrawOne(ctx, original);
        Mh3uSignShow(ctx, unit, names, rest);
        Mh3u::SetSignPass(0);
    }
    if (hide) Mh3uSignShow(ctx, unit, hide, was);
}

/// The unit's instance drawn (0x002957b4, uGUI's draw's tail call): uGUI's block is the last one written.
void Mh3uGuiInstanceDraw(RCtx* ctx, RFunc original) {
    if (mh3u_gui_block_pending) {
        mh3u_gui_block_pending = false;
        if (const u32 cdraw = rd32(ctx, ctx->r[1] + 4)) Mh3uGuiPlaceBlock(ctx, cdraw, mh3u_gui_shift);
    }
    original(ctx);
}

/// nGUI::Draw's begin (0x00539d48, an instance's animation drawing on its own): a block of its own, with the unit's shift.
void Mh3uGuiBegin(RCtx* ctx, RFunc original) {
    const u32 cdraw = rd32(ctx, ctx->r[0] + 4);
    const u32 before = cdraw ? rd32(ctx, cdraw + MH3U_CDRAW_GUI_CONSTANTS) : 0;
    original(ctx);
    if ((mh3u_gui_shift != 0.0f || mh3u_hud_place.on) && cdraw && rd32(ctx, cdraw + MH3U_CDRAW_GUI_CONSTANTS) != before) {
        Mh3uGuiPlaceBlock(ctx, cdraw, mh3u_gui_shift);
    }
}

// The HUD's sprites (uDrawIDSprite: the clock and the health, stamina and sharpness gauges). uDrawIDSprite's draw (slot
// 12, 0x002f6628) draws each of its elements (0x003e6b04), which works out their corners in clip space on the CPU with
// a fixed projection of a 400 x 240 space: x * (2/400) - 1, the 2/400 a float literal (0x003e7610, read from memory at
// run time: the recompiler doesn't fold VLDR literals). In a viewport W rows wide that stretches them by W/400 from the
// screen's left. The hook makes that literal 2/W while the unit draws on the wide top screen (2/400 otherwise), which
// keeps their pixel size at the left edge, and moves the unit's x (+0x90, one of the three it adds up: +0x70, +0x80,
// +0x90) by its side's shift for that draw alone.
constexpr u32 MH3U_SPRITE_X = 0x70;          // f32 x of the sprite unit: +0x70, +0x80 and +0x90 added (its draw)
constexpr u32 MH3U_SPRITE_X_MOVED = 0x90;    // the one the hook moves
constexpr u32 MH3U_SPRITE_SCALE_X = 0x003e7610;  // f32 2/400: 0x003e6b04's literal (vldr s28 at 0x003e729c)
constexpr float MH3U_SPRITE_SCALE_400 = 2.0f / 400.0f;
std::vector<u32> mh3u_sprites_traced;
bool mh3u_sprite_scale_unknown = false;  // the literal held something else: another version, left alone

/// Writes the sprites' x scale (2/rows), if the literal holds 2/400 or a scale this hook wrote.
bool Mh3uSpriteScale(RCtx* ctx, float rows) {
    if (mh3u_sprite_scale_unknown) return false;
    const float now = ReadF32(ctx, MH3U_SPRITE_SCALE_X);
    if (!(std::fabs(now * 400.0f - 2.0f) < 1e-4f) && !(now > 2.0f / 1100.0f && now < 2.0f / 399.0f)) {
        mh3u_sprite_scale_unknown = true;
        LogInfo("picture: the HUD sprites' scale at {:08x} holds {} (another version of the game): left alone",
                MH3U_SPRITE_SCALE_X, now);
        return false;
    }
    const float want = rows > 0.0f ? 2.0f / rows : MH3U_SPRITE_SCALE_400;
    if (now != want) WriteF32(ctx, MH3U_SPRITE_SCALE_X, want);
    return true;
}

// (mh3u-ui) The HUD's parts (hud_size, hud_<part>) for its sprites, the part told by the sprite's place (Mh3u::HudSprite:
// the bars' box or the map's): a sprite of a hidden part is not drawn; the others are drawn at their part's size and
// place by the same two things the wide top screen uses, for that draw alone: the projection's two literals (x 2/400 at
// 0x003e7610, or the wide one's 2/W; y 2/240 at 0x003e7614) times the size, which scales the sprite round the screen's
// top left, and the unit's place (+0x90, +0x94) moved so that the part's corner stays and the player's move is added.
// (A viewport of their own did nothing: their draws reach the GPU later, under the screen's own viewport.)
constexpr u32 MH3U_SPRITE_SCALE_Y = 0x003e7614;  // f32 2/240: 0x003e6b04's literal (vldr s27 at 0x003e72a0)
constexpr float MH3U_SPRITE_SCALE_240 = 2.0f / 240.0f;
struct Mh3uSpritePart {
    bool on = false, hidden = false;
    float size = 1.0f, dx = 0.0f, dy = 0.0f, ax = 0.0f, ay = 0.0f;
};
// The unit's place is the screen's corner for all the HUD's sprites (woods5's log): its elements (+0x44 of them, their
// pointers at +0x50; 0x003e6b04 draws one with flags +0x08 bits 0x20 and 0x01) have their own places (+0x18, +0x1c) and
// sizes (+0x28, +0x2c, whole pixels). The part is told by the box round the elements drawn; one as large as most of the
// screen (the darkening over the whole picture) is no part's.
constexpr u32 MH3U_SPRITE_ELEMENTS = 0x44, MH3U_SPRITE_ELEMENT_LIST = 0x50;
Mh3uSpritePart Mh3uSpritePartOf(RCtx* ctx, u32 unit) {
    Mh3uSpritePart p;
    const float sx = ReadF32(ctx, unit + MH3U_SPRITE_X) + ReadF32(ctx, unit + MH3U_SPRITE_X + 0x10) +
                     ReadF32(ctx, unit + MH3U_SPRITE_X + 0x20);
    const float sy = ReadF32(ctx, unit + MH3U_SPRITE_X + 4) + ReadF32(ctx, unit + MH3U_SPRITE_X + 0x14) +
                     ReadF32(ctx, unit + MH3U_SPRITE_X + 0x24);
    const u32 count = std::min<u32>(rd32(ctx, unit + MH3U_SPRITE_ELEMENTS), 256);
    const u32 list = rd32(ctx, unit + MH3U_SPRITE_ELEMENT_LIST);
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (u32 i = 0; list != 0 && i < count; ++i) {
        const u32 e = rd32(ctx, list + 4 * i);
        if (e == 0 || (rd32(ctx, e + 8) & 0x21) != 0x21) continue;
        const float ex = ReadF32(ctx, e + 0x18), ey = ReadF32(ctx, e + 0x1c);
        const float ew = static_cast<float>(static_cast<s32>(rd32(ctx, e + 0x28)));
        const float eh = static_cast<float>(static_cast<s32>(rd32(ctx, e + 0x2c)));
        if (!(std::fabs(ex) < 4096.0f && std::fabs(ey) < 4096.0f)) continue;
        x0 = std::min(x0, ex - std::fabs(ew) * 0.5f), x1 = std::max(x1, ex + std::fabs(ew) * 0.5f);
        y0 = std::min(y0, ey - std::fabs(eh) * 0.5f), y1 = std::max(y1, ey + std::fabs(eh) * 0.5f);
    }
    const bool placed = x1 >= x0;  // (else its elements drawn are its others, +0x58/+0x64: their place not read)
    const float cx = placed ? sx + (x0 + x1) * 0.5f : sx, cy = placed ? sy + (y0 + y1) * 0.5f : sy;
    // (a full-screen one: as wide AND as tall as most of the screen; the bars' sprite is 325 x 113 on the phones, mh3u-final)
    const bool full = placed && x1 - x0 > TOP_ROWS * 0.9f && y1 - y0 > LCD_COLUMNS * 0.9f;
    if (!full) p.on = Mh3u::HudSprite(placed, cx, cy, &p.size, &p.dx, &p.dy, &p.ax, &p.ay, &p.hidden);
    Mh3u::HudSpriteSeen(unit, placed, cx, cy, placed ? x1 - x0 : 0.0f, placed ? y1 - y0 : 0.0f, count, p.on);
    return p;
}
/// Draws a sprite unit of the top screen with the main view's scale taken off (Mh3uProjMatrix, mh3u_view_sx / sy): the
/// two literals over it and the unit's place moved by (scale - 1) / literal, so that the scaled clip coordinates are the
/// ones the game meant (x * literal - 1, and y's alike, scaled round the picture's middle). In a quest, where the
/// HUD is (woods4: its sprites are the bars'); elsewhere as the game draws them. mh3u_hud_unscale = off: as before
void Mh3uSpriteUnscaled(RCtx* ctx, RFunc original, u32 unit) {
    const float sx = mh3u_view_sx, sy = mh3u_view_sy;
    const float kx = ReadF32(ctx, MH3U_SPRITE_SCALE_X), ky = ReadF32(ctx, MH3U_SPRITE_SCALE_Y);
    if (!options.mh3u_hud_unscale || !Mh3u::HudInQuest() || mh3u_sprite_scale_unknown || (sx == 1.0f && sy == 1.0f) ||
        !(sx > 0.0f) || !(sy > 0.0f) || !(kx > 0.0f) || !(ky > 0.0f)) {
        original(ctx);
        return;
    }
    const float x = ReadF32(ctx, unit + MH3U_SPRITE_X_MOVED), y = ReadF32(ctx, unit + MH3U_SPRITE_X_MOVED + 4);
    WriteF32(ctx, MH3U_SPRITE_SCALE_X, kx / sx);
    WriteF32(ctx, MH3U_SPRITE_SCALE_Y, ky / sy);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED, x + (sx - 1.0f) / kx);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED + 4, y + (sy - 1.0f) / ky);
    original(ctx);
    WriteF32(ctx, MH3U_SPRITE_SCALE_X, kx);
    WriteF32(ctx, MH3U_SPRITE_SCALE_Y, ky);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED, x);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED + 4, y);
}
/// Out of a quest, a sprite unit drawn with its elements moved by `shift` rows and, on a top screen narrower than the
/// console's (4:3: the main view's projection narrowed, mh3u_view_sx / sy below 1), with that scale taken off as
/// Mh3uSpriteUnscaled takes it off the HUD's, but by the elements' places (+ 0x18, + 0x1c) instead of the unit's: the
/// text a menu draws with a sprite takes its place from the unit's own (the Smithy's and the Armory's weapon panels:
/// Attack, Sharpness, Slots, Affinity, their values, the weapon's name), so the unit's place moved took the text out of
/// its panel, over the list (the Moto at 20:9, the RG406V at 4:3, mh3u-menus2).
void Mh3uSpriteElementsDraw(RCtx* ctx, RFunc original, u32 unit, float shift, bool narrow) {
    const float sx = mh3u_view_sx, sy = mh3u_view_sy;
    const float kx = ReadF32(ctx, MH3U_SPRITE_SCALE_X), ky = ReadF32(ctx, MH3U_SPRITE_SCALE_Y);
    const bool unscale = narrow && options.mh3u_hud_unscale && !mh3u_sprite_scale_unknown && !(sx == 1.0f && sy == 1.0f) &&
                         sx > 0.0f && sy > 0.0f && kx > 0.0f && ky > 0.0f;
    const float dx = shift + (unscale ? (sx - 1.0f) / kx : 0.0f), dy = unscale ? (sy - 1.0f) / ky : 0.0f;
    if (dx == 0.0f && dy == 0.0f && !unscale) {
        original(ctx);
        return;
    }
    const u32 count = std::min<u32>(rd32(ctx, unit + MH3U_SPRITE_ELEMENTS), 256);
    const u32 list = rd32(ctx, unit + MH3U_SPRITE_ELEMENT_LIST);
    u32 moved[256];
    float was_x[256], was_y[256];
    u32 n = 0;
    for (u32 i = 0; list != 0 && i < count; ++i) {
        const u32 e = rd32(ctx, list + 4 * i);
        if (e == 0) continue;
        was_x[n] = ReadF32(ctx, e + 0x18), was_y[n] = ReadF32(ctx, e + 0x1c);
        WriteF32(ctx, e + 0x18, was_x[n] + dx);
        WriteF32(ctx, e + 0x1c, was_y[n] + dy);
        moved[n++] = e;
    }
    if (unscale) WriteF32(ctx, MH3U_SPRITE_SCALE_X, kx / sx), WriteF32(ctx, MH3U_SPRITE_SCALE_Y, ky / sy);
    original(ctx);
    if (unscale) WriteF32(ctx, MH3U_SPRITE_SCALE_X, kx), WriteF32(ctx, MH3U_SPRITE_SCALE_Y, ky);
    for (u32 i = 0; i < n; ++i) WriteF32(ctx, moved[i] + 0x18, was_x[i]), WriteF32(ctx, moved[i] + 0x1c, was_y[i]);
}

/// Draws the sprite unit at its part's size and place; `shift` is the wide top screen's move of it (its rows), which
/// the caller has already added to its x.
void Mh3uSpritePartDraw(RCtx* ctx, RFunc original, u32 unit, const Mh3uSpritePart& p, float shift) {
    const float s = p.size;
    const float kx = ReadF32(ctx, MH3U_SPRITE_SCALE_X), ky = ReadF32(ctx, MH3U_SPRITE_SCALE_Y);
    if (!(s > 0.0f) || mh3u_sprite_scale_unknown || !(std::fabs(ky - MH3U_SPRITE_SCALE_240) < 1e-6f)) {
        original(ctx);  // (another version's literals: as the game draws it)
        return;
    }
    const float x = ReadF32(ctx, unit + MH3U_SPRITE_X_MOVED), y = ReadF32(ctx, unit + MH3U_SPRITE_X_MOVED + 4);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED, x + ((shift + p.ax) * (1.0f - s) + p.dx) / s);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED + 4, y + (p.ay * (1.0f - s) + p.dy) / s);
    WriteF32(ctx, MH3U_SPRITE_SCALE_X, kx * s);
    WriteF32(ctx, MH3U_SPRITE_SCALE_Y, ky * s);
    Mh3uSpriteUnscaled(ctx, original, unit);
    WriteF32(ctx, MH3U_SPRITE_SCALE_X, kx);
    WriteF32(ctx, MH3U_SPRITE_SCALE_Y, ky);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED, x);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED + 4, y);
}

void Mh3uSpriteDraw(RCtx* ctx, RFunc original) {
    const u32 unit = ctx->r[0];
    const u32 cdraw = ctx->r[1];
    const u32 rows = WideRows();
    bool wide = false, top = false;
    if (cdraw) {
        const s32 x0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT));
        const s32 y0 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 4));
        const s32 x1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 8));
        const s32 y1 = static_cast<s32>(rd32(ctx, cdraw + MH3U_CDRAW_VIEWPORT + 12));
        wide = rows && y1 - y0 == static_cast<s32>(rows);
        top = wide || (y1 - y0 == static_cast<s32>(TOP_ROWS) && x1 - x0 == static_cast<s32>(LCD_COLUMNS));
        if (options.trace_state && !top && mh3u_sprites_traced.size() < 64 &&
            std::find(mh3u_sprites_traced.begin(), mh3u_sprites_traced.end(), unit) == mh3u_sprites_traced.end()) {
            mh3u_sprites_traced.push_back(unit);  // (mh3u-ui, trace_state: a sprite drawn elsewhere than the top screen)
            LogInfo("picture: sprite {:08x} drawn in the viewport {},{} - {},{}", unit, x0, y0, x1, y1);
        }
    }
    // (mh3u-ui) the HUD's part of a sprite on the top screen
    const Mh3uSpritePart part = top ? Mh3uSpritePartOf(ctx, unit) : Mh3uSpritePart{};
    if (part.on && part.hidden) return;
    if (!Mh3uSpriteScale(ctx, wide ? static_cast<float>(rows) : 0.0f) || !wide) {
        if (part.on) Mh3uSpritePartDraw(ctx, original, unit, part, 0.0f);
        else if (top && !Mh3u::HudInQuest() && !mh3u_sprite_scale_unknown) Mh3uSpriteElementsDraw(ctx, original, unit, 0.0f, true);
        else if (top) Mh3uSpriteUnscaled(ctx, original, unit);
        else original(ctx);
        return;
    }
    const float x = ReadF32(ctx, unit + MH3U_SPRITE_X) + ReadF32(ctx, unit + MH3U_SPRITE_X + 0x10) +
                    ReadF32(ctx, unit + MH3U_SPRITE_X + 0x20);
    // (hud_edges: the hunting HUD's sprites at their side's edge. Only in a quest, where the HUD is: a town's menus draw
    // sprites of their own, the Smithy's and the Armory's weapon panels (the weapon's icon and name bar, the sharpness
    // bar, the slots' marks, the lines between the rows), which went to the picture's left edge with the HUD's left side,
    // out of their panel and over its labels: the Moto, mh3u-menus2)
    Mh3uHudSide side = Mh3uHudSide::Middle;
    const bool quest = Mh3u::HudInQuest();
    if (options.hud_edges && quest) side = x < TOP_ROWS / 2.0f ? Mh3uHudSide::Left : Mh3uHudSide::Right;
    float shift = static_cast<float>(rows - TOP_ROWS) * 0.5f;
    if (side != Mh3uHudSide::Middle) {
        const float edge = static_cast<float>(EdgeRows() ? EdgeRows() : rows);
        const float margin = (static_cast<float>(rows) - edge) * 0.5f;
        const bool low = (side == Mh3uHudSide::Left) == (MH3U_GUI_LEFT > 0.0f);
        shift = low ? margin : margin + (edge - static_cast<float>(TOP_ROWS));
    }
    if (options.trace_layout && mh3u_sprites_traced.size() < 64 &&
        std::find(mh3u_sprites_traced.begin(), mh3u_sprites_traced.end(), unit) == mh3u_sprites_traced.end()) {
        mh3u_sprites_traced.push_back(unit);
        LogInfo("picture: HUD sprite {:08x} at x {:.0f} on the wide top screen, moved {:.0f} rows{}{}", unit, x, shift,
                side == Mh3uHudSide::Left ? " (left)" : side == Mh3uHudSide::Right ? " (right)" : "",
                quest ? "" : " (its elements: out of a quest)");
    }
    // Out of a quest the 400 is centred by moving the sprite's elements (their x, + 0x18) for the draw, not the unit's own
    // x: the menus' text drawn with a sprite (the weapon panels' Attack, Sharpness, Slots and Affinity, their values, the
    // weapon's name) takes its place from the unit's x and is centred by the GUI's own shift, so with the unit's x moved
    // it came 72 rows right of its panel at 20:9, over the list; the elements moved, the pictures are centred and the text
    // stays (the Moto, the Moga Outfitter's and Port Tanzia's Armory and Smithy lists, mh3u-menus2)
    if (!quest) {
        Mh3uSpriteElementsDraw(ctx, original, unit, shift, false);
        return;
    }
    const float before = ReadF32(ctx, unit + MH3U_SPRITE_X_MOVED);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED, before + shift);
    if (part.on) Mh3uSpritePartDraw(ctx, original, unit, part, shift);  // (mh3u-ui)
    else Mh3uSpriteUnscaled(ctx, original, unit);
    WriteF32(ctx, unit + MH3U_SPRITE_X_MOVED, before);
}

constexpr Hook MH3U_GFX_HOOKS[] = {
    {0x002f6628, "mh3u:uDrawIDSprite::draw", Mh3uSpriteDraw},
    {0x00549748, "mh3u:uGUI::draw", Mh3uGuiDraw},
    {0x002957b4, "mh3u:cGUIInstance::draw", Mh3uGuiInstanceDraw},
    {0x00539d48, "mh3u:nGUI::Draw::begin", Mh3uGuiBegin},
    {0x003320fc, "uAppCameraBase::getProjMatrix", Mh3uProjMatrix},
    {0x00265c54, "mh3u:uFreeCamera::getProjMatrix", Mh3uProjMatrix},
    {0x00309d6c, "mh3u:uMotionCamera::getProjMatrix", Mh3uProjMatrix},
    {0x00573a60, "mh3u:nDraw::Scene::Scene", Mh3uSceneCtor},
    {0x0056e3a8, "mh3u:nDraw::DisplayBuffer::init", Mh3uDisplayBufferInit},
    {0x00881478, "mh3u:sCamera::Viewport::updateRegion", Mh3uUpdateRegion},
};

/// InstallMh3u's part for the picture: the widescreen's shape and the code's 400s, the log lines of the options that
/// change the picture, and the hook points above.
void Mh3uGfxInstall(const RCodeInfo& info, std::vector<std::string>& log) {
    mh3u_owner_regions.clear();
    mh3u_gui_traced.clear();
    mh3u_sprites_traced.clear();
    mh3u_sprite_scale_unknown = false;
    mh3u_gui_shift = 0.0f;
    mh3u_gui_block_pending = false;
    mh3u_hud_place = {};
    mh3u_filters_applied = 0xFFFFFFFF;
    mh3u_effects_applied = -1;
    mh3u_slots_unknown = false;
    mh3u_flags_logged[0] = mh3u_flags_logged[1] = 0xFFFFFFFF;
    mh3u_flags_logged_at = -10.0;
    if (options.widescreen > 0) {
        log.push_back(WideRows() ? fmt::format("widescreen: top screen rendered {}x{}", WideRows(), LCD_COLUMNS)
                                 : fmt::format("widescreen: 3D view {} to aspect ratio {:.4f}",
                                               options.widescreen > NATIVE_ASPECT ? "widened" : "narrowed",
                                               options.widescreen));
        SetScreenAspect(options.widescreen, log);
        if (WideRows()) {
            const u32 set = SetTunables(info, "top_rows", WideRows());
            log.push_back(fmt::format("widescreen: {} of the code's top-screen sizes set to {}", set, WideRows()));
        }
        relayout_for_overlay = true;  // (as Install: the layout worked out again on the first frame)
        relayout_portrait = P().IsPortrait();
    }
    if (!options.dither) log.push_back("dithering off");
    if (!options.stereo_3d) log.push_back("stereoscopic 3D off: the top screen drawn once");
    if (options.fov != 1.0f) log.push_back(fmt::format("field of view x{:.2f}", options.fov));
    if (!options.effects) log.push_back("the game's particle effects off");
    if (!SLTRENDER_INSTANCE.Known() || !SCAMERA_INSTANCE.Known() || !EMPTY_RENDER.Known()) {
        log.push_back("picture: this library has no MH3U addresses for sLtRender, sCamera or the empty render "
                      "(config/port_map.txt): rebuild it");
    }
    for (const Hook& hook : MH3U_GFX_HOOKS) {
        const u32 addr = HookAddress(info, hook);
        if (!info.set_hook(addr, hook.fn)) {
            log.push_back(fmt::format("no hook point for {} at {:08X} (rebuild the code library)", hook.name, addr));
            ++missing_hook_points;
        }
    }
}
