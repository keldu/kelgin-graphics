#pragma once

#include "render/render.h"

#include <queue>
#include <set>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <cassert>
#include <complex>

#include <iostream>

#include "ogl33_bindings.h"

#include <kelgin/window/gl/gl_context.h>
#include <kelgin/common.h>

#include "common/math.h"
#include "common/shapes.h"

#include "ogl33_mesh.h"
#include "ogl33_texture.h"
#include "ogl33_program.h"
#include "ogl33_scene.h"
#include "ogl33_camera.h"

namespace gin {
class Ogl33Render;
class Ogl33RenderProperty;
class Ogl33RenderObject;


class Ogl33RenderTarget {
protected:
	~Ogl33RenderTarget() = default;

	std::array<float, 4> clear_colour = {0.f, 0.f, 0.f, 1.f};
public:
	virtual void beginRender() = 0;
	virtual void endRender() = 0;

	void setClearColour(const std::array<float, 4>& colour);

	virtual void bind() = 0;

	virtual size_t width() const = 0;
	virtual size_t height() const = 0;
};

class Ogl33Window final : public Ogl33RenderTarget {
private:
	Own<GlWindow> window;
public:
	Ogl33Window(Own<GlWindow>&&);

	void show();
	void hide();

	Conveyor<RenderEvent::Events> listenToWindowEvents();

	void beginRender() override;
	void endRender() override;

	/**
	* This binds the window as the current 0 framebuffer in its context
	* This is different to glBindFramebuffer(0);
	*/
	void bindAsMain();

	/**
	*
	*/
	void bind() override;

	size_t width() const override;
	size_t height() const override;
};

class Ogl33RenderTexture final : public Ogl33RenderTarget {
private:
public:
	~Ogl33RenderTexture();

	void beginRender() override;
	void endRender() override;

	void bind() override;

	size_t width() const override;
	size_t height() const override;
};

/// @todo this storage kinda feels hacky
/// An idea would be to use evenly numbered IDs for RenderWindows
/// and odd numbered IDs for RenderTextures
class Ogl33RenderTargetStorage {
private:
	std::map<RenderTargetId, Ogl33RenderTexture> render_textures;
	std::map<RenderTargetId, Ogl33Window> windows;

	RenderTargetId max_free_id = 1;
	std::priority_queue<RenderTargetId, std::vector<RenderTargetId>, std::greater<RenderTargetId>> free_ids;
public:
	RenderTextureId insert(Ogl33RenderTexture&& render_texture);
	RenderWindowId insert(Ogl33Window&& render_window);
	void erase(const RenderTargetId& id);

	bool exists(const RenderTargetId& id) const;
	Ogl33RenderTarget* operator[](const RenderTargetId& id);
	const Ogl33RenderTarget* operator[](const RenderTargetId& id) const;

	Ogl33Window* getWindow(const RenderWindowId&);
	Ogl33RenderTexture* getRenderTexture(const RenderTextureId&);
};

class Ogl33RenderProperty {
public:
	MeshId mesh_id;
	TextureId texture_id;
};

class Ogl33RenderProperty3d {
public:
	Mesh3dId mesh_id;
	TextureId texture_id;
};

class Ogl33RenderStage {
private:
	void renderOne(Ogl33Program& program, Ogl33RenderProperty& property, Ogl33Scene::RenderObject& object, Ogl33Mesh& mesh, Ogl33Texture&, Matrix<float, 3, 3>& vp, float time_interval);
public:
	RenderTargetId target_id;
	RenderViewportId viewport_id;
	RenderSceneId scene_id;
	RenderCameraId camera_id;
	ProgramId program_id;

	void render(Ogl33Render& render, float time_interval);
};

class Ogl33RenderStage3d {
private:
	void renderOne(Ogl33Program3d& program, Ogl33RenderProperty3d& property, Ogl33Scene3d::RenderObject& object, Ogl33Mesh3d& mesh, Ogl33Texture& texture, Matrix<float, 4, 4>& vp);
public:
	RenderTargetId target_id;
	RenderViewportId viewport_id;
	RenderScene3dId scene_id;
	RenderCamera3dId camera_id;
	Program3dId program_id;

	void render(Ogl33Render& render);
};

class Ogl33Resources {
public:
	// Render Targets
	Ogl33RenderTargetStorage render_targets;
	// General Resource Storage
	std::unordered_map<TextureId, Ogl33Texture> textures;
	std::unordered_map<RenderViewportId, Ogl33Viewport> viewports;

	struct RenderTargetUpdate {
		std::chrono::steady_clock::duration seconds_per_frame;
		std::chrono::steady_clock::time_point next_update;
	};
	std::unordered_map<RenderTargetId, RenderTargetUpdate> render_target_times;	

	std::queue<RenderTargetId> render_target_draw_tasks;
};

class Ogl33Resources2D {
public:
	Ogl33Resources* res;

	// 2D Resource Storage
	std::unordered_map<MeshId, Ogl33Mesh> meshes;
	std::unordered_map<ProgramId, Ogl33Program> programs;
	std::unordered_map<RenderCameraId, Ogl33Camera> cameras;
	std::unordered_map<RenderPropertyId, Ogl33RenderProperty> render_properties;
	std::unordered_map<RenderSceneId, Ogl33Scene> scenes;
	std::unordered_map<RenderStageId, Ogl33RenderStage> render_stages;

	// Stages listening  to RenderTarget changes
	std::unordered_multimap<RenderTargetId, RenderStageId> render_target_stages;

public:
	Ogl33Resources2D(Ogl33Resources& resources):res{&resources}{}
};

class Ogl33Render;
class Ogl33Render2D final : public LowLevelRender2D {
private:
	Ogl33Resources2D resources;
	Ogl33Render* render;

public:
	Ogl33Render2D(Ogl33Render& r);

	Ogl33Resources2D& getResources(){
		return resources;
	}

	// 2D
	ErrorOr<MeshId> createMesh(const MeshData&) noexcept override;
	Error setMeshData(const MeshId&, const MeshData&) noexcept override;
	Error destroyMesh(const MeshId&) noexcept override;

	ErrorOr<ProgramId> createProgram(const std::string& vertex_src, const std::string& fragment_src) noexcept override;
	ErrorOr<ProgramId> createProgram() noexcept override;
	Error destroyProgram(const ProgramId&) noexcept override;

	ErrorOr<RenderCameraId> createCamera() noexcept override;
	Error setCameraPosition(const RenderCameraId&, float x, float y) noexcept override;
	Error setCameraRotation(const RenderCameraId&, float alpha) noexcept override;
	Error setCameraOrthographic(const RenderCameraId&, float, float, float, float) noexcept override;
	Error destroyCamera(const RenderCameraId&) noexcept override;
	
	ErrorOr<RenderStageId> createStage(const RenderTargetId& id, const RenderViewportId&, const RenderSceneId&, const RenderCameraId&, const ProgramId&) noexcept override;
	Error destroyStage(const RenderStageId&) noexcept override;

	ErrorOr<RenderPropertyId> createProperty(const MeshId&, const TextureId&) noexcept override;
	Error setPropertyMesh(const RenderPropertyId&, const MeshId& id) noexcept override;
	Error setPropertyTexture(const RenderPropertyId&, const TextureId& id) noexcept override;
	Error destroyProperty(const RenderPropertyId&) noexcept override;

	ErrorOr<RenderSceneId> createScene() noexcept override;
	ErrorOr<RenderObjectId> createObject(const RenderSceneId&, const RenderPropertyId&) noexcept override;
	Error setObjectPosition(const RenderSceneId&, const RenderObjectId&, float, float, bool interpolate = true) noexcept override;
	Error setObjectRotation(const RenderSceneId&, const RenderObjectId&, float, bool interpolate = true) noexcept override;
	Error setObjectVisibility(const RenderSceneId&, const RenderObjectId&, bool) noexcept override;
	Error setObjectLayer(const RenderSceneId& id, const RenderObjectId&, float) noexcept override;
	Error setObjectProperty(const RenderSceneId& id, const RenderObjectId&, const RenderPropertyId&) noexcept override;
	Error destroyObject(const RenderSceneId&, const RenderObjectId&) noexcept override;
	Error destroyScene(const RenderSceneId&) noexcept override;
};

class Ogl33Resources3D {
public:
	Ogl33Resources* res;

	// 3D Resource Storage
	std::unordered_map<Mesh3dId, Ogl33Mesh3d> meshes_3d;
	std::unordered_map<Program3dId, Ogl33Program3d> programs_3d;
	std::unordered_map<RenderCamera3dId, Ogl33Camera3d> cameras_3d;
	std::unordered_map<RenderProperty3dId, Ogl33RenderProperty3d> render_properties_3d;
	std::unordered_map<RenderScene3dId, Ogl33Scene3d> scenes_3d;
	std::unordered_map<RenderStage3dId, Ogl33RenderStage3d> render_stages_3d;

	// Stages listening  to RenderTarget changes
	std::unordered_multimap<RenderTargetId, RenderStage3dId> render_target_stages_3d;
public:
	Ogl33Resources3D(Ogl33Resources& resources):res{&resources}{}
};

/*
class Ogl33Render3D final : public LowLevelRender3D {
private:
	Ogl33Resources2D resources;
	Ogl33Render* render;

public:
	Ogl33Render3D(Ogl33Render& r):resources_3d{r.getResources()}, render{&r}{}


	// 3D
	ErrorOr<Mesh3dId> createMesh3d(const Mesh3dData&) noexcept override;
	Error destroyMesh3d(const Mesh3dId&) noexcept override;

	/// @todo layout api ideas
	ErrorOr<RenderProperty3dId> createProperty3d(const Mesh3dId&, const TextureId&) noexcept override;
	Error destroyProperty3d(const RenderProperty3dId&) noexcept override;

	ErrorOr<Program3dId> createProgram3d(const std::string& vertex_src, const std::string& fragment_src) noexcept override;
	ErrorOr<Program3dId> createProgram3d() noexcept override;
	Error destroyProgram3d(const Program3dId&) noexcept override;

	ErrorOr<RenderScene3dId> createScene3d() noexcept override;
	ErrorOr<RenderObject3dId> createObject3d(const RenderScene3dId&, const RenderProperty3dId&) noexcept override;
	Error destroyObject3d(const RenderScene3dId&, const RenderObject3dId&) noexcept override;
	Error destroyScene3d(const RenderScene3dId&) noexcept override;

	ErrorOr<RenderCamera3dId> createCamera3d() noexcept override;
	Error setCamera3dPosition(const RenderCamera3dId&, float, float, float) noexcept override;
	Error setCamera3dOrthographic(const RenderCamera3dId&, float, float, float, float, float, float) noexcept override;
	Error destroyCamera3d(const RenderCamera3dId&) noexcept override;

	ErrorOr<RenderStage3dId> createStage3d(const RenderTargetId&, const RenderViewportId&, const RenderScene3dId&, const RenderCamera3dId&, const Program3dId&) noexcept override;
	Error destroyStage3d(const RenderStage3dId&) noexcept override;
};
*/

class Ogl33Render final : public LowLevelRender {
private:
	Own<GlContext> context;

	bool loaded_glad = false;

	Ogl33Resources resources;

	Ogl33Render2D render_2d;
	//Ogl33Render3D render_3d;

	void stepRenderTargetTimes(const std::chrono::steady_clock::time_point&);

	std::chrono::steady_clock::time_point old_time_point;
	std::chrono::steady_clock::time_point time_point;
public:
	Ogl33Render(Own<GlContext>&&);
	~Ogl33Render();

	Ogl33Scene* getScene(const RenderSceneId&) noexcept;
	Ogl33Camera* getCamera(const RenderCameraId&) noexcept;
	Ogl33Program* getProgram(const ProgramId&) noexcept;
	Ogl33RenderProperty* getProperty(const RenderPropertyId&) noexcept;
	Ogl33Mesh* getMesh(const MeshId&) noexcept;
	Ogl33Texture* getTexture(const TextureId&) noexcept;

	
	Ogl33Scene3d* getScene3d(const RenderScene3dId&) noexcept;
	Ogl33Camera3d* getCamera3d(const RenderCamera3dId&) noexcept;
	Ogl33Program3d* getProgram3d(const Program3dId&) noexcept;
	Ogl33RenderProperty3d* getRenderProperty3d(const RenderProperty3dId&) noexcept;
	Ogl33Mesh3d* getMesh3d(const Mesh3dId& ) noexcept;
	

	Ogl33Resources& getResources() noexcept {
		return resources;
	}

	LowLevelRender2D* interface2D() noexcept override {return &render_2d;}
	LowLevelRender3D* interface3D() noexcept override {return nullptr;}

	ErrorOr<TextureId> createTexture(const Image&) noexcept override;
	Error destroyTexture(const TextureId&) noexcept override;

	ErrorOr<RenderWindowId> createWindow(const RenderVideoMode&, const std::string& title) noexcept override;
	Error setWindowDesiredFPS(const RenderWindowId&, float fps) noexcept override;
	Error setWindowVisibility(const RenderWindowId& id, bool show) noexcept override;
	Error destroyWindow(const RenderWindowId& id) noexcept override;

	Conveyor<RenderEvent::Events> listenToWindowEvents(const RenderWindowId&) noexcept override;

	ErrorOr<RenderViewportId> createViewport() noexcept override;
	Error setViewportRect(const RenderViewportId&, float, float, float, float) noexcept override;
	Error destroyViewport(const RenderViewportId&) noexcept override;


	void step(const std::chrono::steady_clock::time_point&) noexcept override;
	void flush() noexcept override;

	void updateTime(const std::chrono::steady_clock::time_point& new_old_time_point, const std::chrono::steady_clock::time_point& new_time_point) noexcept override;
};
}
