#include "BreakoutMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <algorithm>
#include <math.h>


BreakoutMode::BreakoutMode() {

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of BreakoutMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

    // assign complementary colors
    COLORS.emplace_back(std::make_pair(RED, GREEN));
    COLORS.emplace_back(std::make_pair(YELLOW, PURPLE));
    COLORS.emplace_back(std::make_pair(BLUE, ORANGE));
    COLORS.emplace_back(std::make_pair(GREEN, RED));
    COLORS.emplace_back(std::make_pair(PURPLE, YELLOW));
    COLORS.emplace_back(std::make_pair(ORANGE, BLUE));

    // and assign paddle/ball to a color
    paddle_color = COLORS.begin();
    ball_color = COLORS.begin();

    // make a bunch of bricks
    std::vector<color_t> rows = { BLUE, ORANGE, GREEN, RED, YELLOW, PURPLE };
    int c = 0;
    float offset = 0.1f;
    float delta_x = 2*brick_radius.x + offset;
    float delta_y = 2*brick_radius.y + offset;

    // hardcoded brick placements bc i'm only making one level
    for (float j = -delta_y; j < 5*(delta_y); j += delta_y) {
        for (float i = -3.5f*(delta_x); i < 3*(delta_x); i += delta_x) {

            // leave a 2x3 hold in the middle
            if (j > 0 && j < 3*delta_y && i > -2*delta_x && i < delta_x) continue;
            bricks.emplace_back(glm::vec2(i+(delta_x/2),j+(delta_y/2)), rows[c]);
        }
        c++;
    }
}

BreakoutMode::~BreakoutMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool BreakoutMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_MOUSEMOTION) {
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
		);

        paddle.x = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).x;
	}

    if (evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT) {
        if (ball_reset) {
            ball_reset = false;
            ball_velocity = glm::vec2(0.0f, 6.0f);
        }
    }

    if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_SPACE) {
        paddle_color = paddle_color + 1 == COLORS.end() ? COLORS.begin() : paddle_color + 1;
    }

	return false;
}

void BreakoutMode::update(float elapsed) {

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//----- paddle update -----

    paddle.x = std::max(paddle.x, -court_radius.x + paddle_radius.x);
    paddle.x = std::min(paddle.x,  court_radius.x - paddle_radius.x);

	//----- ball update -----

    if (ball_reset) {
        ball = paddle + glm::vec2(0.0f, ball_radius.y);
    } else {
	    ball += elapsed * ball_velocity;
    }


	//---- collision handling ----

	//paddles:
	auto paddle_vs_ball = [this](glm::vec2 const &paddle) {
		//compute area of overlap:
		glm::vec2 min = glm::max(paddle - paddle_radius, ball - ball_radius);
		glm::vec2 max = glm::min(paddle + paddle_radius, ball + ball_radius);

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y) return;

		if (max.x - min.x > max.y - min.y) {
			//wider overlap in x => bounce in y direction:
			if (ball.y > paddle.y) {
				ball.y = paddle.y + paddle_radius.y + ball_radius.y;
				ball_velocity.y = std::abs(ball_velocity.y);
			} else {
				ball.y = paddle.y - paddle_radius.y - ball_radius.y;
				ball_velocity.y = -std::abs(ball_velocity.y);
			}
            // warp x velocity based on offset from paddle center
            float vel = (ball.x - paddle.x) / (paddle_radius.x + ball_radius.x);
            ball_velocity.x = 4.0f * vel;
		} else {
			//wider overlap in y => bounce in x direction:
			if (ball.x > paddle.x) {
				ball.x = paddle.x + paddle_radius.x + ball_radius.x;
				ball_velocity.x = std::abs(ball_velocity.x);
			} else {
				ball.x = paddle.x - paddle_radius.x - ball_radius.x;
				ball_velocity.x = -std::abs(ball_velocity.x);
			}
		}

        if (ball_color != paddle_color) ball_color = paddle_color;
	};
	paddle_vs_ball(paddle);

    auto ball_vs_brick = [this]() {
        for (int i = 0; i < bricks.size(); i++) {
            Brick b = bricks[i];

            // compute area of overlap:
            glm::vec2 min = glm::max(b.Position - brick_radius, ball - ball_radius);
            glm::vec2 max = glm::min(b.Position + brick_radius, ball + ball_radius);

            // if no overlap, no collision
            // additionally, ball passes through bricks of complementary color
            if (min.x > max.x || min.y > max.y || ball_color->second == b.Color) continue;
            if (max.x - min.x > max.y - min.y) {
                // wider overlap in x => bounce in y direction:
                if (ball.y > b.Position.y) {
                    ball.y = b.Position.y + brick_radius.y + ball_radius.y;
                    ball_velocity.y = std::abs(ball_velocity.y);
                } else {
                    ball.y = b.Position.y - brick_radius.y - ball_radius.y;
                    ball_velocity.y = -std::abs(ball_velocity.y);
                }
            } else {
                // wider overlap in y => bounce in x direction:
                if (ball.x > b.Position.x) {
                    ball.x = b.Position.x + brick_radius.x + ball_radius.x;
                    ball_velocity.x = std::abs(ball_velocity.x);
                } else {
                    ball.x = b.Position.x - brick_radius.x - ball_radius.x;
                    ball_velocity.x = -std::abs(ball_velocity.x);
                }
            }

            if (ball_color->first == b.Color) {
                // break brick
                auto it = bricks.cbegin() + i;
                bricks.erase(it);
                score++;
            }
            // can't hit more than one brick per frame
            return;
        }
    };
    ball_vs_brick();

	//court walls:
	if (ball.y > court_radius.y - ball_radius.y) {
		ball.y = court_radius.y - ball_radius.y;
		if (ball_velocity.y > 0.0f) {
			ball_velocity.y = -ball_velocity.y;
		}
	}
	if (ball.y < -court_radius.y + ball_radius.y) {
		ball.y = -court_radius.y + ball_radius.y;
        ball_velocity = glm::vec2(0.0f, 0.0f);
        ball_reset = true;
	}

	if (ball.x > court_radius.x - ball_radius.x) {
		ball.x = court_radius.x - ball_radius.x;
		if (ball_velocity.x > 0.0f) {
			ball_velocity.x = -ball_velocity.x;
		}
	}
	if (ball.x < -court_radius.x + ball_radius.x) {
		ball.x = -court_radius.x + ball_radius.x;
		if (ball_velocity.x < 0.0f) {
			ball_velocity.x = -ball_velocity.x;
		}
	}
}

void BreakoutMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
    #define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0xf3ffc6ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0x000000ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xa5df40ff);
	const std::vector< glm::u8vec4 > rainbow_colors = {
		HEX_TO_U8VEC4(0xe2ff70ff), HEX_TO_U8VEC4(0xcbff70ff), HEX_TO_U8VEC4(0xaeff5dff),
		HEX_TO_U8VEC4(0x88ff52ff), HEX_TO_U8VEC4(0x6cff47ff), HEX_TO_U8VEC4(0x3aff37ff),
		HEX_TO_U8VEC4(0x2eff94ff), HEX_TO_U8VEC4(0x2effa5ff), HEX_TO_U8VEC4(0x17ffc1ff),
		HEX_TO_U8VEC4(0x00f4e7ff), HEX_TO_U8VEC4(0x00cbe4ff), HEX_TO_U8VEC4(0x00b0d8ff),
		HEX_TO_U8VEC4(0x00a5d1ff), HEX_TO_U8VEC4(0x0098cfd8), HEX_TO_U8VEC4(0x0098cf54),
		HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54),
		HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54), HEX_TO_U8VEC4(0x0098cf54),
		HEX_TO_U8VEC4(0x0098cf54)
	};
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//split rectangle into two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(0.0f,-shadow_offset);

	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius)+s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius)+s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(paddle+s, paddle_radius, shadow_color);
	draw_rectangle(ball+s, ball_radius, shadow_color);


	//solid objects:
    for (Brick b : bricks) {
        draw_rectangle(b.Position+s, brick_radius, shadow_color); // shadow
        draw_rectangle(b.Position, brick_radius, b.Color); // brick
    }

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

    //paddle:
    draw_rectangle(paddle, paddle_radius, paddle_color->first);

	//ball:
	draw_rectangle(ball, ball_radius, ball_color->first);

	//scores:



	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so this matrix is actually transposed from how it appears.

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);


	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
