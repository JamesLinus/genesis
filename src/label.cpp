#include "label.hpp"
#include "gui.hpp"
#include "debug.hpp"

#include <epoxy/gl.h>
#include <epoxy/glx.h>


static void ft_ok(FT_Error err) {
    if (err)
        panic("freetype error");
}

Label::Label(Gui *gui) :
    _gui(gui),
    _width(0),
    _height(0),
    _text("Label"),
    _font_size(16),
    _render_slice_start(-1),
    _render_slice_end(-1)
{

    glGenTextures(1, &_texture_id);
    glBindTexture(GL_TEXTURE_2D, _texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenVertexArrays(1, &_vertex_array);
    glBindVertexArray(_vertex_array);

    glGenBuffers(1, &_vertex_buffer);
    glGenBuffers(1, &_tex_coord_buffer);


    // send dummy vertex data - real data happens at update()
    GLfloat vertexes[4][3] = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
    };
    glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(GLfloat), vertexes, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(_gui->_text_attrib_position);
    glVertexAttribPointer(_gui->_text_attrib_position, 3, GL_FLOAT, GL_FALSE, 0, NULL);


    GLfloat coords[4][2] = {
        {0, 0},
        {0, 1},
        {1, 0},
        {1, 1},
    };
    glBindBuffer(GL_ARRAY_BUFFER, _tex_coord_buffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), coords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(_gui->_text_attrib_tex_coord);
    glVertexAttribPointer(_gui->_text_attrib_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    assert_no_gl_error();

    glGenVertexArrays(1, &_slice_vertex_array);
    glBindVertexArray(_slice_vertex_array);

    glGenBuffers(1, &_slice_vertex_buffer);
    glGenBuffers(1, &_slice_tex_coord_buffer);

    // send dummy vertex data for the slice
    glBindBuffer(GL_ARRAY_BUFFER, _slice_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(GLfloat), vertexes, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(_gui->_text_attrib_position);
    glVertexAttribPointer(_gui->_text_attrib_position, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    // dummy tex coord data for the slice
    glBindBuffer(GL_ARRAY_BUFFER, _slice_tex_coord_buffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), coords, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(_gui->_text_attrib_tex_coord);
    glVertexAttribPointer(_gui->_text_attrib_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, NULL);


    update();
}

Label::~Label() {
    glDeleteBuffers(1, &_slice_tex_coord_buffer);
    glDeleteBuffers(1, &_slice_vertex_buffer);
    glDeleteVertexArrays(1, &_slice_vertex_array);

    glDeleteBuffers(1, &_tex_coord_buffer);
    glDeleteBuffers(1, &_vertex_buffer);
    glDeleteVertexArrays(1, &_vertex_array);
    glDeleteTextures(1, &_texture_id);
}

void Label::draw(const glm::mat4 &mvp, const glm::vec4 &color) {
    _gui->_text_shader_program.bind();

    _gui->_text_shader_program.set_uniform(_gui->_text_uniform_color, color);
    _gui->_text_shader_program.set_uniform(_gui->_text_uniform_tex, 0);
    _gui->_text_shader_program.set_uniform(_gui->_text_uniform_mvp, mvp);

    glBindVertexArray(_vertex_array);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture_id);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Label::draw_slice(const glm::mat4 &mvp, const glm::vec4 &color) {
    _gui->_text_shader_program.bind();

    _gui->_text_shader_program.set_uniform(_gui->_text_uniform_color, color);
    _gui->_text_shader_program.set_uniform(_gui->_text_uniform_tex, 0);
    _gui->_text_shader_program.set_uniform(_gui->_text_uniform_mvp, mvp);

    glBindVertexArray(_slice_vertex_array);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture_id);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void copy_freetype_bitmap(FT_Bitmap source, ByteBuffer &dest,
        int left, int top, int dest_width)
{
    int pitch = source.pitch;
    if (pitch < 0)
        panic("flow up unsupported");

    if (source.pixel_mode != FT_PIXEL_MODE_GRAY)
        panic("only 8-bit grayscale fonts supported");

    for (int y = 0; y < source.rows; y += 1) {
        for (int x = 0; x < source.width; x += 1) {
            unsigned char alpha = source.buffer[y * pitch + x];
            int dest_index = 4 * ((top + y) * dest_width + x + left) + 3;
            dest.at(dest_index) = alpha;
        }
    }
}

void Label::update() {
    // one pass to determine width and height
    // pen_x and pen_y are on the baseline. the char can go lower than it
    float pen_x = 0.0f;
    float pen_y = 0.0f;
    int previous_glyph_index = 0;
    float above_size = 0.0f; // pixel count above the baseline
    float below_size = 0.0f; // pixel count below the baseline
    float bounding_width = 0.0f;
    float prev_right = 0.0f;
    _letters.clear();
    for (int i = 0; i < _text.length(); i += 1) {
        uint32_t ch = _text.at(i);
        FontCacheKey key = {_font_size, ch};
        FontCacheValue entry = _gui->font_cache_entry(key);
        if (_letters.length() > 0) {
            FT_Face face = _gui->_default_font_face;
            FT_Vector kerning;
            ft_ok(FT_Get_Kerning(face, previous_glyph_index, entry.glyph_index,
                        FT_KERNING_DEFAULT, &kerning));
            float kerning_x = ((float)kerning.x) / 64.0f;
            pen_x += kerning_x;
        }

        float bmp_start_left = (float)entry.bitmap_glyph->left;
        float bmp_start_top = (float)entry.bitmap_glyph->top;
        FT_Bitmap bitmap = entry.bitmap_glyph->bitmap;
        float bmp_width = bitmap.width;
        float bmp_height = bitmap.rows;
        float left = pen_x + bmp_start_left;
        float right = left + bmp_width;
        float this_above_size = pen_y + bmp_start_top;
        float this_below_size = bmp_height - this_above_size;
        above_size = (this_above_size > above_size) ? this_above_size : above_size;
        below_size = (this_below_size > below_size) ? this_below_size : below_size;
        bounding_width = ceilf(right);

        int halfway_left = floorf((prev_right + left) / 2.0f);
        if (_letters.length() > 0) {
            Letter *prev_letter = &_letters.at(_letters.length() - 1);
            prev_letter->full_width = halfway_left - prev_letter->left;
        }

        _letters.append(Letter {
            ch,

            halfway_left,
            (int)(left - halfway_left),
            (int)bmp_width,
            (int)(right - halfway_left),

            (int)ceilf(this_above_size),
            (int)ceilf(this_below_size),
            entry.bitmap_glyph->top,
        });

        previous_glyph_index = entry.glyph_index;
        prev_right = right;
        pen_x += ((float)entry.glyph->advance.x) / 65536.0f;
        pen_y += ((float)entry.glyph->advance.y) / 65536.0f;
    }

    float bounding_height = ceilf(above_size + below_size);
    _width = bounding_width;
    _height = bounding_height;
    _above_size = above_size;
    _below_size = below_size;

    int img_buf_size =  4 * bounding_width * bounding_height;
    if (img_buf_size > _img_buffer.length())
        _img_buffer.resize(img_buf_size);

    glBindVertexArray(_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
    GLfloat vertexes[4][3] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, bounding_height, 0.0f},
        {bounding_width, 0.0f, 0.0f},
        {bounding_width, bounding_height, 0.0f},
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * 4 * sizeof(GLfloat), vertexes);

    assert_no_gl_error();

    update_render_slice();

    _img_buffer.fill(0);
    // second pass to render bitmap
    for (int i = 0; i < _letters.length(); i += 1) {
        Letter *letter = &_letters.at(i);
        FontCacheKey key = {_font_size, letter->codepoint};
        FontCacheValue entry = _gui->font_cache_entry(key);
        FT_Bitmap bitmap = entry.bitmap_glyph->bitmap;
        copy_freetype_bitmap(bitmap, _img_buffer,
                letter->left + letter->bitmap_left, _above_size - letter->bitmap_top, _width);
    }

    // send bitmap to GPU
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bounding_width, bounding_height,
            0, GL_BGRA, GL_UNSIGNED_BYTE, _img_buffer.raw());

    assert_no_gl_error();

}

int Label::cursor_at_pos(int x, int y) const {
    if (x < 0)
        return 0;
    for (int i = 0; i < _letters.length(); i += 1) {
        const Letter *letter = &_letters.at(i);

        if (x < letter->left + letter->full_width / 2)
            return i;
    }
    return _letters.length();
}

void Label::pos_at_cursor(int index, int &x, int &y) const {
    y = _above_size;
    if (index < 0) {
        x = 0;
        return;
    }
    if (index >= _letters.length()) {
        const Letter *letter = &_letters.at(_letters.length() - 1);
        x = letter->left + letter->full_width;
        return;
    }
    const Letter *letter = &_letters.at(index);
    x = letter->left;
}

void Label::get_slice_dimensions(int start, int end, int &start_x, int &end_x) const {
    if (end >= _letters.length()) {
        const Letter *end_letter = &_letters.at(_letters.length() - 1);
        end_x = end_letter->left + end_letter->full_width;
    } else {
        const Letter *end_letter = &_letters.at(end);
        end_x = end_letter->left;
    }
    const Letter *start_letter = &_letters.at(start);
    start_x = start_letter->left;
}

void Label::set_slice(int start, int end) {
    _render_slice_start = start;
    _render_slice_end = end;
    update_render_slice();
}

void Label::update_render_slice() {
    int start, end;
    if (_render_slice_start == -1) {
        start = 0;
        end = _text.length() + 1;
    } else {
        start = _render_slice_start;
        end = _render_slice_end;
    }
    int start_x, start_y;
    int end_x, end_y;
    pos_at_cursor(start, start_x, start_y);
    pos_at_cursor(end, end_x, end_y);
    float width = end_x - start_x;
    float tex_start_x = ((float)start_x) / ((float)_width);
    float tex_end_x = tex_start_x + width / ((float)_width);

    glBindVertexArray(_slice_vertex_array);

    glBindBuffer(GL_ARRAY_BUFFER, _slice_vertex_buffer);
    GLfloat vertexes[4][3] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, (float)_height, 0.0f},
        {width, 0.0f, 0.0f},
        {width, (float)_height, 0.0f},
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * 4 * sizeof(GLfloat), vertexes);

    glBindBuffer(GL_ARRAY_BUFFER, _slice_tex_coord_buffer);
    GLfloat coords[4][2] = {
        {tex_start_x, 0.0f},
        {tex_start_x, 1.0f},
        {tex_end_x, 0.0f},
        {tex_end_x, 1.0f},
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * 4 * sizeof(GLfloat), coords);
}

void Label::replace_text(int start, int end, String text) {
    _text.replace(start, end, text);
}