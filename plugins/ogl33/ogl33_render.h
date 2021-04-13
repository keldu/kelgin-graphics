#pragma once

#include "render/render.h"

#include <queue>
#include <set>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <cassert>

#include <iostream>

#include "ogl33_bindings.h"

#include <kelgin/window/gl/gl_context.h>
#include <kelgin/common.h>

#include "common/math.h"

namespace gin {
class Ogl33Render;
class Ogl33RenderProperty;
class Ogl33RenderObject;

class Ogl33Camera {
private:
	Matrix<float, 3, 3> projection_matrix;
	Matrix<float, 3, 3> view_matrix;
public:
	Ogl33Camera();

	void setViewPosition(float x, float y);
	void setViewRotation(float angle);

	void setOrtho(float left, float right, float top, float bot);

	const Matrix<float, 3,3>& view() const;
	const Matrix<float, 3,3>& projection() const;
};

class Ogl33Viewport {
private:
	float x;
	float y;
	float width;
	float height;
public:
	Ogl33Viewport(float x, float y,float w, float h);

	void use();
};

/// @todo Implement usage of VertexArrayObjects
class Ogl33Mesh {
private:
	std::array<GLuint,3> ids;
	size_t indices;
	
	// GLuint vao;
public:
	Ogl33Mesh();
	Ogl33Mesh(std::array<GLuint,3>&&, size_t);
	// Ogl33Mesh(std::array<GLuint,3>&&, size_t, GLuint);
	~Ogl33Mesh();
	Ogl33Mesh(Ogl33Mesh&&);

	// void bindAttribute() const;

	void bindVertex() const;
	void bindUV() const;
	void bindIndex() const;

	void setData(const MeshData& data);

	size_t indexCount() const;
};

class Ogl33Mesh3d {
private:
	GLuint vao;
	std::array<GLuint, 2> ids;
	size_t indices;

public:
	Ogl33Mesh3d();
	Ogl33Mesh3d(GLuint, std::array<GLuint, 2>&&, size_t);
	~Ogl33Mesh3d();
	Ogl33Mesh3d(Ogl33Mesh3d&&);

	void bindAttribute() const;
	void bindIndex() const;

	void setData(const Mesh3dData& data);

	size_t indexCount() const;
};

class Ogl33Camera3d {
private:
	Matrix<float, 4, 4> projection_matrix;
	Matrix<float, 4, 4> view_matrix;
public:
	Ogl33Camera3d();

	void setOrtho(float left, float right, float top, float bottom, float near, float far);
	void setViewPosition(float x, float y, float z);
	void setViewRotation(float alpha, float beta, float gamma);

	const Matrix<float, 4, 4>& projection() const;
	const Matrix<float, 4, 4>& view() const;
};

class Ogl33Texture {
private:
	GLuint tex_id;
public:
	Ogl33Texture();
	Ogl33Texture(GLuint tex_id);
	~Ogl33Texture();
	Ogl33Texture(Ogl33Texture&&);

	void bind() const;
};

class Ogl33Program3d {
private:
	GLuint program_id;

	GLuint texture_uniform;
	GLuint mvp_uniform;
public:
	Ogl33Program3d();
	Ogl33Program3d(GLuint, GLuint, GLuint);
	~Ogl33Program3d();

	void setTexture(const Ogl33Texture&);
	void setMvp(const Matrix<float, 4, 4>&);
	void setMesh(const Ogl33Mesh3d&);
};

class Ogl33Program {
private:
	GLuint program_id;

	GLuint texture_uniform;
	GLuint mvp_uniform;
	GLuint layer_uniform;
public:
	Ogl33Program();
	Ogl33Program(GLuint, GLuint, GLuint, GLuint);
	~Ogl33Program();

	Ogl33Program(Ogl33Program&&);

	void setTexture(const Ogl33Texture&);
	void setMvp(const Matrix<float,3,3>&);
	void setMesh(const Ogl33Mesh&);
	void setLayer(float);
	void setLayer(int16_t);

	void use();
};

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

class Ogl33Scene {
public:
	struct RenderObject {
		RenderPropertyId id = 0;
		float x = 0.f;
		float y = 0.f;
		float angle = 0.f;
		float layer = 0.f;
		bool visible = true;
	};
private:
	std::unordered_map<RenderObjectId, RenderObject> objects;
public:

	RenderObjectId createObject(const RenderPropertyId&);
	void destroyObject(const RenderObjectId&);
	void setObjectPosition(const RenderObjectId&, float, float);
	void setObjectRotation(const RenderObjectId&, float);
	void setObjectVisibility(const RenderObjectId&, bool);

	void visit(const Ogl33Camera&, std::vector<RenderObject*>&);
};

class Ogl33Scene3d {
public:
	struct RenderObject {
		RenderProperty3dId id = 0;
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;
		float alpha = 0.f;
		float beta = 0.f;
		float gamma = 0.f;
		bool visible = true;
	};
private:
	std::unordered_map<RenderObject3dId, RenderObject> objects;
public:

	RenderObject3dId createObject(const RenderProperty3dId&);
	void destroyObject(const RenderObject3dId&);

	void setObjectPosition(const RenderObject3dId&, float, float, float);
	void setObjectVisibility(const RenderObject3dId&, bool);
};

class Ogl33RenderStage {
private:
	void renderOne(Ogl33Program& program, Ogl33RenderProperty& property, Ogl33Scene::RenderObject& object, Ogl33Mesh& mesh, Ogl33Texture&, Matrix<float, 3, 3>& vp);
public:
	RenderTargetId target_id;
	RenderSceneId scene_id;
	RenderCameraId camera_id;
	ProgramId program_id;

	void render(Ogl33Render& render);
};

class Ogl33RenderStage3d {
private:
	void renderOne();
public:
	RenderTargetId target_id;
	RenderScene3dId scene_id;
	RenderCamera3dId camera_id;
	Program3dId program_id;

	void render();
};

class Ogl33Render final : public LowLevelRender {
private:
	Own<GlContext> context;

	Ogl33RenderTargetStorage render_targets;
	bool loaded_glad = false;
	GLuint vao = 0;

	// General Resource Storage
	std::unordered_map<TextureId, Ogl33Texture> textures;
	std::unordered_map<RenderViewportId, Ogl33Viewport> viewports;

	// 2D Resource Storage
	std::unordered_map<MeshId, Ogl33Mesh> meshes;
	std::unordered_map<ProgramId, Ogl33Program> programs;
	std::unordered_map<RenderCameraId, Ogl33Camera> cameras;
	std::unordered_map<RenderPropertyId, Ogl33RenderProperty> render_properties;
	std::unordered_map<RenderSceneId, Own<Ogl33Scene>> scenes;
	std::unordered_map<RenderStageId, Ogl33RenderStage> render_stages;

	// Stages listening  to RenderTarget changes
	std::unordered_multimap<RenderTargetId, RenderStageId> render_target_stages;

	// 3D Resource Storage
	std::unordered_map<Mesh3dId, Ogl33Mesh3d> meshes_3d;
	std::unordered_map<Program3dId, Ogl33Program3d> programs_3d;
	std::unordered_map<RenderCamera3dId, Ogl33Camera3d> cameras_3d;
	std::unordered_map<RenderProperty3dId, Ogl33RenderProperty3d> render_properties_3d;
	std::unordered_map<RenderScene3dId, Ogl33Scene3d> scenes_3d;
	std::unordered_map<RenderStage3dId, Ogl33RenderStage3d> render_stages_3d;

	// Stages listening  to RenderTarget changes
	std::unordered_multimap<RenderTargetId, RenderStage3dId> render_target_stages_3d;

	struct RenderTargetUpdate {
		std::chrono::steady_clock::duration seconds_per_frame;
		std::chrono::steady_clock::time_point next_update;
	};
	std::unordered_map<RenderTargetId, RenderTargetUpdate> render_target_times;	

	void stepRenderTargetTimes(const std::chrono::steady_clock::time_point&);

	std::queue<RenderTargetId> render_target_draw_tasks;
public:
	Ogl33Render(Own<GlContext>&&);
	~Ogl33Render();

	Ogl33Scene* getScene(const RenderSceneId&) noexcept;
	Ogl33Camera* getCamera(const RenderCameraId&) noexcept;
	Ogl33Program* getProgram(const ProgramId&) noexcept;
	Ogl33RenderProperty* getProperty(const RenderPropertyId&) noexcept;
	Ogl33Mesh* getMesh(const MeshId&) noexcept;
	Ogl33Texture* getTexture(const TextureId&) noexcept;

	ErrorOr<MeshId> createMesh(const MeshData&) noexcept override;
	Error setMeshData(const MeshId&, const MeshData&) noexcept override;
	Error destroyMesh(const MeshId&) noexcept override;

	ErrorOr<TextureId> createTexture(const Image&) noexcept override;
	Error destroyTexture(const TextureId&) noexcept override;

	ErrorOr<RenderWindowId> createWindow(const RenderVideoMode&, const std::string& title) noexcept override;
	Error setWindowDesiredFPS(const RenderWindowId&, float fps) noexcept override;
	Error setWindowVisibility(const RenderWindowId& id, bool show) noexcept override;
	Error destroyWindow(const RenderWindowId& id) noexcept override;

	Conveyor<RenderEvent::Events> listenToWindowEvents(const RenderWindowId&) noexcept override;

	ErrorOr<ProgramId> createProgram(const std::string& vertex_src, const std::string& fragment_src) noexcept override;
	ErrorOr<ProgramId> createProgram() noexcept override;
	Error destroyProgram(const ProgramId&) noexcept override;

	ErrorOr<RenderCameraId> createCamera() noexcept override;
	Error setCameraPosition(const RenderCameraId&, float x, float y) noexcept override;
	Error setCameraRotation(const RenderCameraId&, float alpha) noexcept override;
	Error setCameraOrthographic(const RenderCameraId&, float, float, float, float) noexcept override;
	Error destroyCamera(const RenderCameraId&) noexcept override;
	
	ErrorOr<RenderStageId> createStage(const RenderTargetId& id, const RenderSceneId&, const RenderCameraId&, const ProgramId&) noexcept override;
	Error destroyStage(const RenderStageId&) noexcept override;

	ErrorOr<RenderViewportId> createViewport() noexcept override;
	Error setViewportRect(const RenderViewportId&, float, float, float, float) noexcept override;
	Error destroyViewport(const RenderViewportId&) noexcept override;

	ErrorOr<RenderPropertyId> createProperty(const MeshId&, const TextureId&) noexcept override;
	Error setPropertyMesh(const RenderPropertyId&, const MeshId& id) noexcept override;
	Error setPropertyTexture(const RenderPropertyId&, const TextureId& id) noexcept override;
	Error destroyProperty(const RenderPropertyId&) noexcept override;

	ErrorOr<RenderSceneId> createScene() noexcept override;
	ErrorOr<RenderObjectId> createObject(const RenderSceneId&, const RenderPropertyId&) noexcept override;
	Error setObjectPosition(const RenderSceneId&, const RenderObjectId&, float, float) noexcept override;
	Error setObjectRotation(const RenderSceneId&, const RenderObjectId&, float) noexcept override;
	Error setObjectVisibility(const RenderSceneId&, const RenderObjectId&, bool) noexcept override;
	Error destroyObject(const RenderSceneId&, const RenderObjectId&) noexcept override;
	Error destroyScene(const RenderSceneId&) noexcept override;

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

	ErrorOr<RenderStage3dId> createStage3d(const RenderTargetId&, const RenderScene3dId&, const RenderCamera3dId&, const Program3dId&) noexcept override;
	Error destroyStage3d(const RenderStage3dId&) noexcept override;

	void step(const std::chrono::steady_clock::time_point&) noexcept override;
	void flush() noexcept override;

	void updateTime(const std::chrono::steady_clock::time_point& ) noexcept override;
};
}
