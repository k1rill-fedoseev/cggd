#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"


void cg::renderer::rasterization_renderer::init()
{
	// load model
	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	// init render target
	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);

	// init depth buffer
	depth_buffer = std::make_shared<cg::resource<float>>(settings->width, settings->height);

	// create camera
	camera = std::make_shared<cg::world::camera>();
	camera->set_height(static_cast<float>(settings->height));
	camera->set_width(static_cast<float>(settings->width));
	camera->set_position(float3{ settings->camera_position[0],
								 settings->camera_position[1],
								 settings->camera_position[2] });
	camera->set_theta(settings->camera_theta);
	camera->set_phi(settings->camera_phi);
	camera->set_angle_of_view(settings->camera_angle_of_view);
	camera->set_z_far(settings->camera_z_far);
	camera->set_z_near(settings->camera_z_near);

	// create rasterizer
	rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();
	rasterizer->set_render_target(render_target, depth_buffer);
	rasterizer->set_viewport(settings->width, settings->height);

	rasterizer->set_vertex_buffer(model->get_vertex_buffer());
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}

void cg::renderer::rasterization_renderer::render()
{
	rasterizer->clear_render_target({0, 0, 0});

	float4x4 matrix = mul(camera->get_projection_matrix(), camera->get_view_matrix(), model->get_world_matrix());
	rasterizer->vertex_shader = [&](float4 vertex, cg::vertex vertex_data) {
		return std::make_pair(mul(matrix, vertex), vertex_data);
	};
	rasterizer->pixel_shader = [&](cg::vertex vertex_data, float z) {
		return cg::color{vertex_data.ambient_r, vertex_data.ambient_g, vertex_data.ambient_b};
	};
	rasterizer->draw(model->get_vertex_buffer()->get_number_of_elements(), 0);

	cg::utils::save_resource(*render_target, settings->result_path);
}