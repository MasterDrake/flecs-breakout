#include <SDL.h>

#include "flecs.h"
#include "components.h"
#include "sdl_render.h"
#include "vmath.h"
#include "bitfont.h"

int score = 0;

uint32_t build_brick(flecs::world &world, Vec2f location, int type)
{
	std::string sprite;
	int score = 1;
	switch (type)
	{
	case 0:
		sprite = "../../assets/sprites/element_yellow_rectangle.png";
		score = 2;
		break;
	case 1:
		sprite = "../../assets/sprites/element_red_rectangle.png";
		score = 5;
		break;
	case 2:
		sprite = "../../assets/sprites/element_purple_rectangle.png";
		score = 3;
		break;

	case 3:
		sprite = "../../assets/sprites/element_grey_rectangle.png";
		score = 1;
		break;
	default:

		sprite = "../../assets/sprites/element_blue_rectangle.png";
		score = 4;
		break;
	}

	//ball
	auto brick = world.entity();

	brick.add<SDL_RenderSprite>();
	brick.add<Brick>();
	brick.get_mut<Brick>()->score_value = score;
	brick.set<SpriteLocation>({ location });
	load_sprite(sprite, brick.get_mut<SDL_RenderSprite>());
	return brick;
}

void registerComponentsReflection(flecs::world& world) {

	world.component<Vec2f>().member<float>("x").member<float>("y");
	//world.component<SDL_RenderSprite>();
	world.component<PlayerInputComponent>().member<Vec2f>("movement_speed");
	world.component<SpriteLocation>().member<Vec2f>("location");
	world.component<MovementComponent>().member<Vec2f>("acceleration").member<Vec2f>("velocity").member<float>("max_speed");
	world.component<Brick>().member<int>("hits_left").member<int>("score_value");
}

int main(int argc, char *argv[])
{
	flecs::world world;
	world.set<flecs::Rest>({});
	world.import<flecs::units>();
	world.import<flecs::stats>(); // Collect statistics periodically

#if _DEBUG
	registerComponentsReflection(world);
#endif

	if (!initialize_sdl())
		std::abort();

	//initialize player
	auto player_entity = world.entity();
	player_entity.add<SDL_RenderSprite>();
	player_entity.add<PlayerInputComponent>();
	player_entity.set<SpriteLocation>({0.0f,0.0f});
	player_entity.add<MovementComponent>();

	load_sprite("../../assets/sprites/paddleBlu.png", player_entity.get_mut<SDL_RenderSprite>());

	//ball
	auto ball_entity = world.entity();
	ball_entity.add<SDL_RenderSprite>();
	ball_entity.add<Ball>();
	ball_entity.set<SpriteLocation>({ 0.0f, 100.0f });
	ball_entity.add<MovementComponent>();
	load_sprite("../../assets/sprites/ballGrey.png", ball_entity.get_mut<SDL_RenderSprite>());
	ball_entity.get_mut<MovementComponent>()->velocity = random_vector() * 400;

	BitFont kenney_font;
	load_font(kenney_font,"../../assets/font/kenney_numbers.png", "../assets/font/kenney_numbers.fnt");

	//borders
	const float bricks_min_x = -WINDOW_WIDTH / 2.f +50;
	const float bricks_max_x = WINDOW_WIDTH / 2.f -50;

	//borders
	const float bricks_min_y = 300;
	const float bricks_max_y = WINDOW_HEIGHT - 150;
	 
	const int ny = 10;
	const int nx = 8;

	for (int y = 0; y <= ny; y++)
	{
		for (int x = 0; x <= nx; x++)
		{
			const float fx = x / float(nx);
			const float fy = y /float( ny);
			Vec2f loc;
			loc.x = bricks_min_x + ((bricks_max_x - bricks_min_x) *fx);
			loc.y = bricks_min_y + ((bricks_max_y - bricks_min_y) *fy);

			const int type = (x + y) % 4;
			build_brick(world, loc, type);
		}
	}

	flecs::system s = world.system<const PlayerInputComponent,  MovementComponent>("PlayerMovement")
		.kind(flecs::OnUpdate)
		.each([](flecs::entity e, const PlayerInputComponent& input,  MovementComponent& movement)
			{
				movement.velocity.x = input.movement_input.x * 300;
			  //movement.velocity += movement.acceleration * deltaSeconds;
			});

	flecs::system move_objects = world.system<SpriteLocation, MovementComponent>("MoveObjects")
		.kind(flecs::OnUpdate)
		.each([](flecs::entity e, SpriteLocation& location, MovementComponent& movement)
			{			
						const float dT = e.world().delta_time();
						movement.velocity += movement.acceleration * dT;
						if (movement.velocity.x != 0.0f || movement.velocity.y != 0.0f)
						{
							auto newLoc = location.location + movement.velocity * dT;
							//TODO: get_mut???
							e.set<SpriteLocation>({ newLoc });
						}
			});


	flecs::system process_ball_collisions = world.system<Ball, SpriteLocation, SDL_RenderSprite, MovementComponent>("ProcessBallCollisions")
		.kind(flecs::OnUpdate)
		.each([&world](flecs::entity e, Ball& ball, SpriteLocation& location, SDL_RenderSprite& sprite, MovementComponent& movement)
		{
				//grab data from location and sprite info
				const float xloc = location.location.x;
				const float xextent = sprite.width / 2.f;
				const float yloc = location.location.y;
				const float yextent = sprite.height / 2.f;


				const float ball_max_x = xloc + xextent;
				const float ball_min_x = xloc - xextent;

				const float ball_max_y = yloc + yextent;
				const float ball_min_y = yloc - yextent;

				//collide against bricks
				auto brickview = world.query_builder<Brick, const SpriteLocation, const SDL_RenderSprite>().build();
				brickview.each([ball_max_y, ball_min_y, ball_max_x, ball_min_x, &location, &movement](flecs::entity e, Brick& brick, const SpriteLocation& brick_location, const SDL_RenderSprite& brick_sprite)
					{

						float brick_min_x = brick_location.location.x - brick_sprite.width / 2;
						float brick_max_x = brick_location.location.x + brick_sprite.width / 2;

						float brick_min_y = brick_location.location.y - brick_sprite.height / 2;
						float brick_max_y = brick_location.location.y + brick_sprite.height / 2;

						bool collides = true;
						// Collision tests
						if (ball_max_x < brick_min_x || ball_min_x > brick_max_x) collides = false;
						if (ball_max_y < brick_min_y || ball_min_y > brick_max_y) collides = false;

						if (collides)
						{

							Vec2f diff = brick_location.location - location.location;

							float factor = (float)brick_sprite.height / (float)brick_sprite.width;
							diff.x *= factor;
							if (abs(diff.x) < abs(diff.y))
							{
								movement.velocity.y *= -1;
							}
							else {
								movement.velocity.x *= -1;
							}

							score += e.get<Brick>()->score_value;
							e.destruct();

						}

					});

				//collide against player paddle
				auto playerview = world.query_builder<const PlayerInputComponent, const SpriteLocation, const SDL_RenderSprite>().build();
				playerview.each([ball_max_y, ball_min_y, ball_max_x, ball_min_x, &location, &movement](flecs::entity player, const PlayerInputComponent& input, const SpriteLocation& brick_location, const SDL_RenderSprite& brick_sprite)
					{

						float brick_min_x = brick_location.location.x - brick_sprite.width / 2;
						float brick_max_x = brick_location.location.x + brick_sprite.width / 2;

						float brick_min_y = brick_location.location.y - brick_sprite.height / 2;
						float brick_max_y = brick_location.location.y + brick_sprite.height / 2;

						bool collides = true;
						// Collision tests
						if (ball_max_x < brick_min_x || ball_min_x > brick_max_x) collides = false;
						if (ball_max_y < brick_min_y || ball_min_y > brick_max_y) collides = false;

						if (collides)
						{
							float deltax = location.location.x - brick_location.location.x;

							deltax /= ((float)brick_sprite.width);

							movement.velocity.x = sin(deltax) * 300;
							movement.velocity.y = cos(deltax) * 300;

							printf("deltax: %f", deltax);
						}
					});
		});

	//borders
	float min_x = -WINDOW_WIDTH / 2.f;
	float max_x = WINDOW_WIDTH / 2.f;

	//borders
	float min_y = -100;
	float max_y = WINDOW_HEIGHT - 100;

	flecs::system player_border_collisions = world.system<PlayerInputComponent, SpriteLocation, SDL_RenderSprite>("PlayerBorderCollisions")
		.kind(flecs::OnUpdate)
		.each([min_x, max_x](flecs::entity e, PlayerInputComponent& input, SpriteLocation& location, SDL_RenderSprite& sprite)
			{
				//grab data from location and sprite info
				float xloc = (float)location.location.x;
				float xextent = sprite.width / 2.f;

				//left edge
				if (xloc - xextent < min_x)
					location.location.x = min_x + xextent;
				else if (xloc + xextent > max_x)
					location.location.x = max_x - xextent;
			});


	//bounce ball ------------------
	auto ball_border_collisions = world.system<Ball, SpriteLocation, SDL_RenderSprite, MovementComponent>("BallBorderCollisions")
		.kind(flecs::OnUpdate)
		.each([min_x, max_x, max_y, min_y](flecs::entity e, Ball& ball, SpriteLocation& location, SDL_RenderSprite& sprite, MovementComponent& movement)
			{
				//grab data from location and sprite info
				float xloc = location.location.x;
				float xextent = sprite.width / 2.f;
				float yloc = location.location.y;
				float yextent = sprite.height / 2.f;
				//left edge
				if (xloc - xextent < min_x)
				{
					location.location.x = min_x + xextent;
					movement.velocity.x *= -1;
				}
				//right edge
				else if (xloc + xextent > max_x)
				{
					location.location.x = max_x - xextent;
					movement.velocity.x *= -1;
				}
				//top edge		
				else if (yloc + yextent > max_y)
				{
					location.location.y = max_y - yextent;
					movement.velocity.y *= -1;
				}
				//down edge
				else if (yloc - yextent < min_y)
				{
					location.location.y = min_y + yextent;
					movement.velocity.y *= -1;
				}
			});

	flecs::system transform_sprites = world.system<SpriteLocation, SDL_RenderSprite>("TransformSprites")
		.kind(flecs::OnUpdate)
		.each([&world](flecs::entity e, SpriteLocation& location, SDL_RenderSprite& sprite)
			{ 
				Vec2i screenspace = game_space_to_screen_space(location.location);
				sprite.location = screenspace;
			});

	flecs::system draw_sprites_sdl = world.system<SDL_RenderSprite>("DrawSprites")
		.kind(flecs::OnUpdate)
		.each([](flecs::entity e, SDL_RenderSprite& sprite)
		{
			draw_sprite(sprite, gRenderer);
		});

	bool quit = false;
	SDL_Event e;
	//While application is running
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			else
			{
				//If a key was pressed
				if (e.type == SDL_KEYDOWN)
				{
					//Adjust the velocity
					switch (e.key.keysym.sym)
					{
					case SDLK_UP:
						player_entity.get_mut<PlayerInputComponent>()->movement_input.y = 1;
						break;													
					case SDLK_DOWN:												
						player_entity.get_mut<PlayerInputComponent>()->movement_input.y = -1;
						break;													
					case SDLK_LEFT:												
						player_entity.get_mut<PlayerInputComponent>()->movement_input.x = -1;
						break;													
					case SDLK_RIGHT:										
						player_entity.get_mut<PlayerInputComponent>()->movement_input.x = 1;
						break;
					}
				}
				else if (e.type == SDL_KEYUP)
				{
					//Adjust the velocity
					switch (e.key.keysym.sym)
					{
					case SDLK_UP:
						player_entity.get_mut<PlayerInputComponent>()->movement_input.y = 0;
						break;
					case SDLK_DOWN:
						player_entity.get_mut<PlayerInputComponent>()->movement_input.y = 0;
						break;
					case SDLK_LEFT:
						player_entity.get_mut<PlayerInputComponent>()->movement_input.x = 0;
						break;
					case SDLK_RIGHT:
						player_entity.get_mut<PlayerInputComponent>()->movement_input.x = 0;
						break;
					}
				}
			}
		}

		start_frame();

		world.progress(1.0f / 60.0f);
		draw_string(kenney_font, "score:" + std::to_string( score), Vec2i{10,750});
		
		end_frame();		
	}

	destroy_sdl();
	

	return 0;
}