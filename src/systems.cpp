#include <iostream>
#include <sciuter/systems.hpp>

void update_timers(const float dt, entt::registry& registry)
{
    auto view = registry.view<
        components::timer>();

    for(auto entity: view) {
        auto &timer = view.get<components::timer>(entity);
	timer.update(dt);
    }
}

void handle_gamepad(
    const SDL_Rect& boundaries,
    entt::registry& registry)
{
    auto view = registry.view<
        components::position,
        components::velocity,
        components::timer,
        components::gamepad>();

    for(auto entity: view) {
        auto &position = view.get<components::position>(entity);
        auto &velocity = view.get<components::velocity>(entity);
        auto &timer = view.get<components::timer>(entity);
        auto &gamepad = view.get<components::gamepad>(entity);

        gamepad.update();

        if(gamepad.down("move_left"))
        {
            velocity.dx = -1;
        }
        else if(gamepad.down("move_right"))
        {
            velocity.dx = 1;
        }
        else
        {
            velocity.dx = 0;
        }

        if(gamepad.down("move_up"))
        {
            velocity.dy = -1;
        }
        else if(gamepad.down("move_down"))
        {
            velocity.dy = 1;
        }
        else
        {
            velocity.dy = 0;
        }

        velocity.normalize();

        if(gamepad.down("fire") && timer.timed_out())
        {
            spawn_bullet(
                    position.x, position.y,
		    150.f, COLLISION_MASK_ENEMIES,
                    boundaries, registry);
        }
    }
}

SDL_Rect center_position(const int x, const int y, const SDL_Rect& frame_rect)
{
    SDL_Rect position =
    {
        x - frame_rect.w / 2, y - frame_rect.h / 2,
        frame_rect.w, frame_rect.h
    };
    return position;
}

void update_animations(const float dt, entt::registry &registry)
{
    auto view = registry.view<
        components::animation,
        components::source_rect>();

    for(auto entity: view) {
        auto &animation = view.get<components::animation>(entity);
        auto &frame_rect = view.get<components::source_rect>(entity);
        animation.update(dt);
        frame_rect.rect = animation.get_current_frame();
    }
}

void update_linear_velocity(const float dt, entt::registry& registry)
{
    auto view = registry.view<
        components::position,
        components::velocity>();

    for(auto entity: view) {
        auto &position = view.get<components::position>(entity);
        auto &velocity = view.get<components::velocity>(entity);

        position.x += velocity.dx * velocity.speed * dt;
        position.y += velocity.dy * velocity.speed * dt;
    }
}

void update_destination_rect(entt::registry& registry)
{
    auto view = registry.view<
        components::position,
        components::source_rect,
        components::destination_rect>();

    for(auto entity: view) {
        auto &position = view.get<components::position>(entity);
        auto &frame_rect = view.get<components::source_rect>(entity);
        auto &dest = view.get<components::destination_rect>(entity);

        dest.rect = center_position(position.x, position.y, frame_rect.rect);
    }
}

void update_shot_to_target_behaviour(
    const SDL_Rect& boundaries,
    entt::registry& registry)
{
    auto view = registry.view<
        components::timer,
        components::target,
        components::destination_rect>();

    for(auto entity: view) {
        auto &timer = view.get<components::timer>(entity);
        auto &dest = view.get<components::destination_rect>(entity);
        auto &target = view.get<components::target>(entity);
	auto &target_pos = registry.get<components::destination_rect>(target.entity);

	if(timer.timed_out() &&
	   target_pos.rect.x < dest.rect.x + dest.rect.w &&
	   target_pos.rect.x + target_pos.rect.w > dest.rect.x)
	{
	    const float x = dest.rect.x + dest.rect.w / 2;
	    const float y = dest.rect.y + dest.rect.h;
	    spawn_bullet(x, y, -100.f, COLLISION_MASK_PLAYER,
			 boundaries, registry);
	}
    }
}

void check_boundaries(entt::registry& registry)
{
    auto view = registry.view<
        components::destination_rect,
        components::screen_boundaries>();

    for(auto entity: view) {
        auto &dest_rect = view.get<components::destination_rect>(entity);
        auto &boundaries = view.get<components::screen_boundaries>(entity);

        if(!SDL_HasIntersection(&dest_rect.rect, &boundaries.rect))
        {
            registry.destroy(entity);
        }
    }
}

void resolve_collisions(entt::registry& registry)
{
    auto view_bullets = registry.view<
        components::destination_rect,
        components::collision_mask,
        components::damage>();
    auto view_targets = registry.view<
        components::destination_rect,
        components::collision_mask,
        components::energy>();

    for(auto bullet: view_bullets) {
        auto &bullet_mask = view_bullets.get<components::collision_mask>(bullet);
        auto &bullet_rect = view_bullets.get<components::destination_rect>(bullet);
        auto &damage = view_bullets.get<components::damage>(bullet);

        for(auto target: view_targets) {
	    auto &target_mask = view_targets.get<components::collision_mask>(target);
	    auto &target_rect = view_targets.get<components::destination_rect>(target);
            auto &energy = view_targets.get<components::energy>(target);

            if((bullet_mask.value & target_mask.value) != 0 &&
	       SDL_HasIntersection(&bullet_rect.rect, &target_rect.rect))
            {
                registry.destroy(bullet);
                energy.value -= damage.value;

                if(energy.value <= 0)
                {
                    registry.destroy(target);
                }
            }
        }
    }
}

void render_sprites(SDL_Renderer* renderer, entt::registry& registry)
{
    auto view = registry.view<
        components::image,
        components::source_rect,
        components::destination_rect>();

    for(auto entity: view) {
        auto &image = view.get<components::image>(entity);
        auto &frame_rect = view.get<components::source_rect>(entity);
        auto &dest_rect = view.get<components::destination_rect>(entity);

        SDL_RenderCopy(
	    renderer, image.texture,
	    &frame_rect.rect, &dest_rect.rect);
    }
}

entt::entity spawn_bullet(
    const float x, const float y,
    const float speed,
    const unsigned int collision_mask,
    const SDL_Rect& boundaries,
    entt::registry& registry)
{
    auto bullet = registry.create();
    auto texture = Resources::get("resources/images/bullet.png");
    registry.assign<components::position>(bullet, x, y);
    registry.assign<components::source_rect>(
            bullet,
            components::source_rect::from_texture(texture));
    registry.assign<components::destination_rect>(bullet);
    registry.assign<components::velocity>(bullet, 0.f, -1.f, speed);
    registry.assign<components::screen_boundaries>(bullet, boundaries);
    registry.assign<components::damage>(bullet, 10);
    registry.assign<components::collision_mask>(bullet, collision_mask);
    registry.assign<components::image>(
            bullet,
            texture);
    return bullet;
}
