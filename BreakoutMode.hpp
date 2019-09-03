#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <vector>
#include <deque>
#include <utility>

#define FROM_HEX( HX ) (glm::u8vec4((HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff, 0xff))

typedef glm::u8vec4 color_t;
typedef std::pair<color_t, color_t> color_pair;
typedef std::vector<color_pair>::const_iterator color_iter;

/*
 * BreakoutMode is a game mode that implements Brick Breaker Colors:
 * Brick Breaker, except the color of the ball must match the color of the brick
 * to break it. More details in README
 */

struct BreakoutMode : Mode {
	BreakoutMode();
	virtual ~BreakoutMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

    const color_t RED    = FROM_HEX(0xec3160);
    const color_t YELLOW = FROM_HEX(0xf3f439);
    const color_t BLUE   = FROM_HEX(0x1c8bc0);
    const color_t GREEN  = FROM_HEX(0x12b65f);
    const color_t PURPLE = FROM_HEX(0xce5bf6);
    const color_t ORANGE = FROM_HEX(0xf48f12);

    std::vector<color_pair> COLORS;

	glm::vec2 court_radius = glm::vec2(7.0f, 5.0f);
	glm::vec2 paddle_radius = glm::vec2(1.0f, 0.2f);
	glm::vec2 ball_radius = glm::vec2(0.1f, 0.1f);
    glm::vec2 brick_radius = glm::vec2(0.5f, 0.3f);
    glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);

    glm::vec2 paddle = glm::vec2(0.0f, -court_radius.y + 0.5);
    color_iter paddle_color;

	glm::vec2 ball = glm::vec2(0.0f, 0.0f);
	glm::vec2 ball_velocity = glm::vec2(0.0f, 0.0f);
    color_iter ball_color;

    struct Brick {
        Brick(glm::vec2 const &Position_, glm::u8vec4 const &Color_) :
            Position(Position_), Color(Color_) { }
        glm::vec2 Position;
        glm::u8vec4 Color;
    };

    std::vector<Brick> bricks;

    bool ball_reset = true;

    int score = 0;


	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "BreakoutMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
