#include "raytracer_renderer.h"

#include "utils/resource_utils.h"


void cg::renderer::ray_tracing_renderer::init()
{
	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(
		settings->width, settings->height);

	raytracer =
		std::make_shared<cg::renderer::raytracer<cg::vertex, cg::unsigned_color>>();
	raytracer->set_render_target(render_target);
	raytracer->set_viewport(settings->width, settings->height);
	shadow_raytracer =
		std::make_shared<cg::renderer::raytracer<cg::vertex, cg::unsigned_color>>();

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

	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	raytracer->set_per_shape_vertex_buffer(model->get_per_shape_buffer());
	lights.push_back({ float3{ 0.f, 1.58f, -0.03f }, float3{ 0.78f, 0.78f, 0.78f } });
	//lights.push_back({ float3{ 0.05f, 1.58f, 0.02f }, float3{ 0.78f, 0.78f, 0.78f } });
	//lights.push_back({ float3{ -0.05f, 1.58f, -0.03f }, float3{ 0.78f, 0.78f, 0.78f } });
	//lights.push_back({ float3{ 0.f, 1.58f, 0.02f }, float3{ 0.78f, 0.78f, 0.78f } });
	//lights.push_back({ float3{ 0.f, 1.58f, -0.08f }, float3{ 0.78f, 0.78f, 0.78f } });
	lights.push_back({ float3{ -0.2f, 0.6f, 1.f }, float3{ 0.5f, 0.5f, 0.5f } });
}

void cg::renderer::ray_tracing_renderer::destroy() {}

void cg::renderer::ray_tracing_renderer::update() {}

void cg::renderer::ray_tracing_renderer::render()
{
	raytracer->clear_render_target({ 0, 0, 0 });
	raytracer->miss_shader = [](const ray& ray) {
		payload payload;
		payload.color = { 0.f, 0.f, 0.f };
		payload.color = {0.f, 0.f, (ray.direction.y + 1.0f) / 2};
		//payload.color = { ray.direction.x / 0.5f + 0.5f, ray.direction.y / 0.5f + 0.5f, ray.direction.z / 0.5f + 0.5f };
		return payload;
	};
	raytracer->closest_hit_shader = [&](const ray& ray, payload& payload,
									   const triangle<vertex>& triangle) {
		float3 result_color = triangle.emissive;
		float3 position = ray.position + ray.direction * payload.t;
		float3 normal = payload.bary.x * triangle.na +
						payload.bary.y * triangle.nb + payload.bary.z * triangle.nc;
		for (auto& light : lights)
		{
			cg::renderer::ray to_light(position, light.position - position);
			float len = length(light.position - position);
			auto shadow_payload =
				shadow_raytracer->trace_ray(to_light, 1, len);
			if (shadow_payload.t == -1.f)
			{
				float m = std::min(1.5f, 1.5f / len);
				result_color += triangle.diffuse * light.color *
								std::max(dot(normal, to_light.direction), 0.f) * m;
			}
		}
		payload.color = cg::color::from_float3(result_color);
		return payload;
	};
	shadow_raytracer->miss_shader = [](const ray& ray) {
		payload payload;
		payload.t = -1.f;
		return payload;
	};
	shadow_raytracer->any_hit_shader = [](const ray& ray, payload& payload,
										   const triangle<vertex>& triangle) {
		return payload;
	};
	raytracer->build_acceleration_structure();
	shadow_raytracer->acceleration_structures = raytracer->acceleration_structures;
	for (unsigned frame_id = 0; frame_id < settings->accumulation_num; frame_id++)
	{
		raytracer->ray_generation(
			camera->get_position(), camera->get_direction(),
			camera->get_right(), camera->get_up(), 1.f / static_cast<float>(frame_id + 1));
	}
	cg::utils::save_resource(*render_target, settings->result_path);
}